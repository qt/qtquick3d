// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qssgrendercontextcore.h"
#include <QtQuick3DRuntimeRender/private/qssgrenderskymaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererimplshaders_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhieffectsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgshadermaterialadapter_p.h>

#include <QtCore/qhash.h>
#include <QtCore/qspan.h>
#include <QtGui/qvector4d.h>
#include <rhi/qshaderbaker.h>

#include <algorithm>

using namespace Qt::StringLiterals;

QT_BEGIN_NAMESPACE

static const char *vertexShaderStr = R"(
void main()
{
    vec4 qt_vertPosition = vec4(attr_pos, 1.0);
    qt_eyeDir = qt_vertPosition.xyz;
    gl_Position = qt_projectionMatrix * qt_viewMatrix * vec4(qt_eyeDir, 1.0);
}

)";

// Screen-space variant: the sky shader is evaluated directly on a fullscreen quad
// for the visible background (no cubemap round-trip). The per-pixel world-space view
// direction is reconstructed exactly like skybox.vert: inverse-project the NDC position
// into view space, then rotate into world space. qt_viewMatrix here carries the combined
// orientation * world-rotation (translation-free), so a single matrix suffices.
static const char *vertexShaderStrScreen = R"(
void main()
{
    gl_Position = vec4(attr_pos, 1.0);
#if QSHADER_VIEW_COUNT >= 2
    vec3 qt_unprojected = (qt_inverseProjection[gl_ViewIndex] * gl_Position).xyz;
    qt_eyeDir = (qt_viewMatrix[gl_ViewIndex] * vec4(qt_unprojected, 0.0)).xyz;
#else
    vec3 qt_unprojected = (qt_inverseProjection * gl_Position).xyz;
    qt_eyeDir = (qt_viewMatrix * vec4(qt_unprojected, 0.0)).xyz;
#endif
    gl_Position.y *= qt_adjustY;
}

)";

static const char *mainFragmentSnippet = R"(

void main()
{
    qt_customMain();
}

)";

// Screen-space (background) fragment main: the user shader writes linear HDR into
// FRAGCOLOR, so we apply exposure + tonemapping here, exactly as skybox.frag does when
// it samples the IBL cube. qt_exposure / qt_tonemap are no-ops unless a tonemapping
// feature is compiled in (TonemapMode::None/Custom -> passthrough), matching the skybox.
static const char *mainFragmentSnippetScreen = R"(

void main()
{
    qt_customMain();
    FRAGCOLOR = vec4(qt_tonemap(qt_exposure(FRAGCOLOR.rgb, qt_skyExposure)), 1.0);
}

)";

static const char *debugFragStr = R"(
// Helper: outer square border of a face
float line(vec2 uv, float width)
{
    vec2 d = abs(uv - 0.5);
    return step(max(d.x, d.y), 0.5) - step(max(d.x, d.y), 0.5 - width);
}

// Simple plus/minus symbol
float drawPlus(vec2 uv)
{
    float h = step(abs(uv.y - 0.5), 0.05) * step(abs(uv.x - 0.5), 0.2);
    float v = step(abs(uv.x - 0.5), 0.05) * step(abs(uv.y - 0.5), 0.2);
    return max(h, v);
}

float drawMinus(vec2 uv)
{
    return step(abs(uv.y - 0.5), 0.05) * step(abs(uv.x - 0.5), 0.2);
}

void MAIN()
{
    vec3 d = normalize(qt_eyeDir);
    vec3 ad = abs(d);

    vec2 uv;
    vec3 faceColor;
    float label = 0.0;

    // Determine dominant axis (cubemap face)
    if (ad.x > ad.y && ad.x > ad.z)
    {
        uv = d.zy / ad.x * 0.5 + 0.5;
        if (d.x > 0.0)
        {
            faceColor = vec3(1.0, 0.0, 0.0); // +X red
            label = drawPlus(uv);
        }
        else
        {
            faceColor = vec3(0.5, 0.0, 0.0); // -X dark red
            label = drawMinus(uv);
        }
    }
    else if (ad.y > ad.x && ad.y > ad.z)
    {
        uv = d.xz / ad.y * 0.5 + 0.5;
        if (d.y > 0.0)
        {
            faceColor = vec3(0.0, 1.0, 0.0); // +Y green
            label = drawPlus(uv);
        }
        else
        {
            faceColor = vec3(0.0, 0.5, 0.0); // -Y dark green
            label = drawMinus(uv);
        }
    }
    else
    {
        uv = d.xy / ad.z * 0.5 + 0.5;
        if (d.z > 0.0)
        {
            faceColor = vec3(0.0, 0.0, 1.0); // +Z blue
            label = drawPlus(uv);
        }
        else
        {
            faceColor = vec3(0.0, 0.0, 0.5); // -Z dark blue
            label = drawMinus(uv);
        }
    }

    // Outer border
    float border = line(uv, 0.02);

    // Combine
    vec3 color = faceColor;
    color = mix(color, vec3(1.0), border); // white border
    color = mix(color, vec3(1.0), label);  // white + or - label

    FRAGCOLOR = vec4(color, 1.0);
}
)";

QSSGRenderSkyMaterial::QSSGRenderSkyMaterial() : QSSGRenderGraphObject(QSSGRenderGraphObject::Type::SkyMaterial) { }

QSSGRenderSkyMaterial::~QSSGRenderSkyMaterial() = default;

// Returns false if any custom-property texture is not ready yet (compilation must wait
// until next frame, since the GLSL sampler type depends on the actual texture: textures
// not created yet are assumed to be sampler2DArray, which could cause compile failures).
static bool skyShaderTexturesReady(const QList<QSSGBaseTypeProperty> &propertyUniforms, QSSGBufferManager &bufferManager)
{
    for (const auto &u : std::as_const(propertyUniforms)) {
        if (u.shaderDataType == QSSGRenderShaderValue::Texture) {
            QSSGRenderImage *image = u.value.value<QSSGRenderImage *>();
            const QSSGRenderImageTexture texture = image ? bufferManager.loadRenderImage(image) : QSSGRenderImageTexture { };
            if (!image || !texture.m_texture)
                return false;
        }
    }
    return true;
}

// Appends the user shader's custom-property texture bindings to an SRB list. Shared by the
// cube (updateUniforms) and screen-space (updateBackgroundUniforms) paths.
static void appendCustomTextureBindings(QSSGRhiShaderPipeline *pipeline,
                                        QSSGRhiShaderResourceBindingList &bindings,
                                        QSSGRhiContext *rhiCtx)
{
    int maxSamplerBinding = -1;
    QVector<QShaderDescription::InOutVariable> samplerVars =
            pipeline->fragmentStage()->shader().description().combinedImageSamplers();
    for (const QShaderDescription::InOutVariable &var :
         pipeline->vertexStage()->shader().description().combinedImageSamplers()) {
        auto it = std::find_if(samplerVars.cbegin(), samplerVars.cend(), [&var](const QShaderDescription::InOutVariable &v) {
            return var.binding == v.binding;
        });
        if (it == samplerVars.cend())
            samplerVars.append(var);
    }
    for (const QShaderDescription::InOutVariable &var : std::as_const(samplerVars))
        maxSamplerBinding = qMax(maxSamplerBinding, var.binding);

    if (maxSamplerBinding < 0)
        return;

    const int customTexCount = pipeline->extraTextureCount();
    for (int i = 0; i < customTexCount; ++i) {
        const QSSGRhiTexture &t(pipeline->extraTextureAt(i));
        const int samplerBinding = pipeline->bindingForTexture(t.name);
        if (samplerBinding >= 0) {
            QRhiSampler *sampler = rhiCtx->sampler(t.samplerDesc);
            bindings.addTexture(samplerBinding, QRhiShaderResourceBinding::FragmentStage, t.texture, sampler);
        }
    }
}

// Compiles a sky pipeline from the shared user fragment shader and a given vertex
// variant. The user fragment is identical for the cube (IBL) and screen (background)
// paths since it only reads qt_eyeDir; the stages differ in the vertex code and uniforms
// and in the fragment main snippet (the screen path tonemaps the linear output for
// display). cacheKeyTag keeps the variants from aliasing in the shader cache.
QSSGRhiShaderPipelinePtr QSSGRenderSkyMaterial::buildPipeline(const QSSGRenderContextInterface &sgContext,
                                                             QByteArray vertexShader,
                                                             const QSSGShaderCustomMaterialAdapter::StringPairList &vertexViewDependentUniforms,
                                                             const QSSGShaderCustomMaterialAdapter::StringPairList &vertexUniforms,
                                                             const QByteArray &fragmentMainSnippet,
                                                             const QSSGShaderCustomMaterialAdapter::StringPairList &fragmentUniforms,
                                                             const QSSGShaderFeatures &features,
                                                             int viewCount,
                                                             const QByteArray &cacheKeyTag)
{
    const bool multiViewCompatible = viewCount >= 2;

    QByteArray fragmentShader = (!fragmentShaderSource.isEmpty() ? fragmentShaderSource : QByteArray(debugFragStr)) + fragmentMainSnippet;

    QSSGShaderCustomMaterialAdapter::StringPairList propertyBaseUniforms;
    for (const auto &u : std::as_const(propertyUniforms))
        propertyBaseUniforms.append({ u.typeName, u.name });

    QSSGShaderCustomMaterialAdapter::StringPairList inputOutputs;
    inputOutputs.append({ "vec3", "qt_eyeDir"_ba });

    {
        QSSGShaderCustomMaterialAdapter::StringPairList vertexBaseUniforms = propertyBaseUniforms;
        vertexBaseUniforms.append(vertexUniforms.constData(), vertexUniforms.size());

        QSSGShaderCustomMaterialAdapter::ShaderCodeAndMetaData result;
        QByteArray buf;
        QSSGShaderCustomMaterialAdapter::CustomShaderPrepWorkData scratch;
        QSSGShaderCustomMaterialAdapter::beginPrepareCustomShader(&scratch, &result, vertexShader, QSSGShaderCache::ShaderType::Vertex, multiViewCompatible);
        QSSGShaderCustomMaterialAdapter::finishPrepareCustomShader(&buf,
                                                                   scratch,
                                                                   result,
                                                                   QSSGShaderCache::ShaderType::Vertex,
                                                                   multiViewCompatible,
                                                                   vertexBaseUniforms,
                                                                   { },
                                                                   inputOutputs,
                                                                   { },
                                                                   vertexViewDependentUniforms);
        vertexShader = result.first;
        vertexShader.append(buf);
    }

    {
        // Fragment uniforms (e.g. qt_skyExposure) are view-independent.
        QSSGShaderCustomMaterialAdapter::StringPairList fragmentBaseUniforms = propertyBaseUniforms;
        fragmentBaseUniforms.append(fragmentUniforms.constData(), fragmentUniforms.size());

        QSSGShaderCustomMaterialAdapter::ShaderCodeAndMetaData result;
        QByteArray buf;
        QSSGShaderCustomMaterialAdapter::CustomShaderPrepWorkData scratch;
        QSSGShaderCustomMaterialAdapter::beginPrepareCustomShader(&scratch, &result, fragmentShader, QSSGShaderCache::ShaderType::Fragment, multiViewCompatible);
        QSSGShaderCustomMaterialAdapter::finishPrepareCustomShader(&buf,
                                                                   scratch,
                                                                   result,
                                                                   QSSGShaderCache::ShaderType::Fragment,
                                                                   multiViewCompatible,
                                                                   fragmentBaseUniforms,
                                                                   inputOutputs,
                                                                   { },
                                                                   { },
                                                                   { });
        fragmentShader = result.first;
        fragmentShader.append(buf);
    }

    auto generator = sgContext.shaderProgramGenerator().get();
    auto shaderLib = sgContext.shaderLibraryManager().get();
    auto shaderCache = sgContext.shaderCache().get();

    generator->beginProgram();
    auto vertex = generator->getStage(QSSGShaderGeneratorStage::Vertex);
    vertex->addIncoming("attr_pos"_ba, "vec3"_ba);
    vertex->append(vertexShader);

    auto fragment = generator->getStage(QSSGShaderGeneratorStage::Fragment);
    fragment->addInclude("tonemapping.glsllib"_ba);
    fragment->append(fragmentShader);

    const QByteArray key = shaderPathKey + cacheKeyTag + ':'
            + QCryptographicHash::hash(QByteArray(vertexShader + fragmentShader), QCryptographicHash::Algorithm::Sha1).toHex();
    return generator->compileGeneratedRhiShader(key, features, *shaderLib, *shaderCache, QSSGRhiShaderPipeline::UsedWithoutIa, { }, viewCount, false);
}

QSSGRhiShaderPipelinePtr QSSGRenderSkyMaterial::ensurePipeline(const QSSGRenderContextInterface &sgContext)
{
    if (iblPassPipeline && !isFragmentShaderDirty)
        return iblPassPipeline;

    if (!skyShaderTexturesReady(propertyUniforms, *sgContext.bufferManager().get()))
        return nullptr;

    QSSGShaderCustomMaterialAdapter::StringPairList vertexViewDependentUniforms;
    vertexViewDependentUniforms.append({ "mat4"_ba, "qt_projectionMatrix"_ba });
    vertexViewDependentUniforms.append({ "mat4"_ba, "qt_viewMatrix"_ba });

    // The cube render stores linear radiance for IBL, so no tonemapping is applied
    // (AcesTonemapping is set only to satisfy the tonemapping.glsllib include; the cube
    // fragment never calls qt_tonemap).
    QSSGShaderFeatures features;
    features.set(QSSGShaderFeatures::Feature::AcesTonemapping, true);

    iblPassPipeline = buildPipeline(sgContext, vertexShaderStr, vertexViewDependentUniforms, { }, mainFragmentSnippet, { }, features, 1, QByteArrayLiteral(":cube"));

    isFragmentShaderDirty = false;

    return iblPassPipeline;
}

QSSGRhiShaderPipelinePtr QSSGRenderSkyMaterial::ensureBackgroundPipeline(const QSSGRenderContextInterface &sgContext,
                                                                        const QSSGShaderFeatures &tonemapFeatures,
                                                                        quint32 tonemapKey,
                                                                        int viewCount)
{
    if (backgroundPipeline && !isBackgroundShaderDirty && m_backgroundTonemapKey == tonemapKey
        && m_backgroundViewCount == viewCount) {
        return backgroundPipeline;
    }

    if (!skyShaderTexturesReady(propertyUniforms, *sgContext.bufferManager().get()))
        return nullptr;

    QSSGShaderCustomMaterialAdapter::StringPairList vertexViewDependentUniforms;
    vertexViewDependentUniforms.append({ "mat4"_ba, "qt_inverseProjection"_ba });
    vertexViewDependentUniforms.append({ "mat4"_ba, "qt_viewMatrix"_ba });

    QSSGShaderCustomMaterialAdapter::StringPairList vertexUniforms;
    vertexUniforms.append({ "float"_ba, "qt_adjustY"_ba });

    QSSGShaderCustomMaterialAdapter::StringPairList fragmentUniforms;
    fragmentUniforms.append({ "float"_ba, "qt_skyExposure"_ba });

    backgroundPipeline = buildPipeline(sgContext, vertexShaderStrScreen, vertexViewDependentUniforms, vertexUniforms,
                                       mainFragmentSnippetScreen, fragmentUniforms, tonemapFeatures, viewCount,
                                       QByteArrayLiteral(":screen:") + QByteArray::number(tonemapKey)
                                               + ':' + QByteArray::number(viewCount));

    m_backgroundTonemapKey = tonemapKey;
    m_backgroundViewCount = viewCount;
    isBackgroundShaderDirty = false;

    return backgroundPipeline;
}

quint32 QSSGRenderSkyMaterial::updateUniforms(const QSSGRenderContextInterface &sgContext,
                                              const QMatrix4x4 &mvp,
                                              const QVarLengthArray<QMatrix4x4, 6> views)
{
    constexpr int cMatrixSize = 64;

    if (!iblPassPipeline)
        return 0;

    QSSGRhiContext *rhiCtx = sgContext.rhiContext().get();
    QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiCtx);
    QSSGRhiDrawCallData *dcd = &rhiCtxD->drawCallData({ (void *)this, nullptr, nullptr, 0 });

    const int uniformStride = rhiCtx->rhi()->ubufAligned(iblPassPipeline->ub0Size());
    const int totalBufferSize = uniformStride * 6;

    if (!dcd->ubuf) {
        dcd->ubuf = rhiCtx->rhi()->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, totalBufferSize);
        dcd->ubuf->create();
    }

    char *ubufData = dcd->ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
    auto bufferManager = sgContext.bufferManager().get();

    iblPassPipeline->resetExtraTextures();

    for (const auto &u : std::as_const(propertyUniforms))
        iblPassPipeline->setShaderResources(ubufData, *bufferManager, u.name, u.value, u.shaderDataType);

    // same for all faces
    iblPassPipeline->setShaderResources(ubufData, *bufferManager, "qt_projectionMatrix"_ba, mvp, QSSGRenderShaderValue::Type::Matrix4x4);
    iblPassPipeline->setShaderResources(ubufData, *bufferManager, "qt_viewMatrix"_ba, views[0], QSSGRenderShaderValue::Type::Matrix4x4);

    // Kinda hacky way of filling out all the views
    // Copy first properties then clone these six times (each face)
    // view matrix is the only unique property per face
    const int viewMatrixOffset = iblPassPipeline->offsetOfUniform("qt_viewMatrix"_ba);

    Q_ASSERT(ubufData != nullptr);
    Q_ASSERT(totalBufferSize >= 6 * uniformStride); // buffer must hold all 6 faces
    Q_ASSERT(uniformStride >= cMatrixSize); // view matrix fits
    Q_ASSERT(viewMatrixOffset >= 0);
    Q_ASSERT(viewMatrixOffset + cMatrixSize <= uniformStride);

    auto buffer = QSpan<char>(ubufData, totalBufferSize);
    auto firstFace = buffer.first(uniformStride);

    for (int face = 1; face < 6; ++face) {
        auto dst = buffer.sliced(face * uniformStride, uniformStride);

        // Copy the entire first face block
        std::copy(firstFace.begin(), firstFace.end(), dst.begin());

        // Copy only the per-face view matrix
        std::copy_n(reinterpret_cast<const char *>(views[face].constData()), cMatrixSize, dst.data() + viewMatrixOffset);
    }
    dcd->ubuf->endFullDynamicBufferUpdateForCurrentFrame();

    rhiCtxD->releaseCachedSrb(bindings);
    bindings = QSSGRhiShaderResourceBindingList();
    bindings.addUniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, dcd->ubuf, 0, uniformStride, true);
    appendCustomTextureBindings(iblPassPipeline.get(), bindings, rhiCtx);

    return uniformStride;
}

void QSSGRenderSkyMaterial::updateBackgroundUniforms(const QSSGRenderContextInterface &sgContext,
                                                     const QVarLengthArray<QMatrix4x4, 2> &inverseProjections,
                                                     const QVarLengthArray<QMatrix4x4, 2> &viewRotations,
                                                     float adjustY,
                                                     float exposure)
{
    if (!backgroundPipeline)
        return;

    QSSGRhiContext *rhiCtx = sgContext.rhiContext().get();
    QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiCtx);
    // Distinct drawCallData key index (1) so the background UBO does not collide with
    // the per-face IBL UBO (index 0) held for the same QSSGRenderSkyMaterial.
    QSSGRhiDrawCallData *dcd = &rhiCtxD->drawCallData({ (void *)this, nullptr, nullptr, 1 });

    const int bufferSize = rhiCtx->rhi()->ubufAligned(backgroundPipeline->ub0Size());

    if (!dcd->ubuf || int(dcd->ubuf->size()) != bufferSize) {
        delete dcd->ubuf;
        dcd->ubuf = rhiCtx->rhi()->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, bufferSize);
        dcd->ubuf->create();
    }

    char *ubufData = dcd->ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
    auto bufferManager = sgContext.bufferManager().get();

    backgroundPipeline->resetExtraTextures();

    for (const auto &u : std::as_const(propertyUniforms))
        backgroundPipeline->setShaderResources(ubufData, *bufferManager, u.name, u.value, u.shaderDataType);

    backgroundPipeline->setShaderResources(ubufData, *bufferManager, "qt_adjustY"_ba, adjustY, QSSGRenderShaderValue::Type::Float);
    backgroundPipeline->setShaderResources(ubufData, *bufferManager, "qt_skyExposure"_ba, exposure, QSSGRenderShaderValue::Type::Float);

    constexpr int matSize = 64;
    const int invProjOffset = backgroundPipeline->offsetOfUniform("qt_inverseProjection"_ba);
    const int viewMatOffset = backgroundPipeline->offsetOfUniform("qt_viewMatrix"_ba);
    const int viewCount = int(inverseProjections.size());
    for (int v = 0; v < viewCount; ++v) {
        if (invProjOffset >= 0)
            std::copy_n(reinterpret_cast<const char *>(inverseProjections[v].constData()), matSize, ubufData + invProjOffset + v * matSize);
        if (viewMatOffset >= 0)
            std::copy_n(reinterpret_cast<const char *>(viewRotations[v].constData()), matSize, ubufData + viewMatOffset + v * matSize);
    }

    dcd->ubuf->endFullDynamicBufferUpdateForCurrentFrame();

    rhiCtxD->releaseCachedSrb(backgroundBindings);
    backgroundBindings = QSSGRhiShaderResourceBindingList();
    backgroundBindings.addUniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, dcd->ubuf);
    appendCustomTextureBindings(backgroundPipeline.get(), backgroundBindings, rhiCtx);
}

QT_END_NAMESPACE
