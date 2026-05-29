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

static const char *mainFragmentSnippet = R"(

void main()
{
    qt_customMain();
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

QSSGRhiShaderPipelinePtr QSSGRenderSkyMaterial::ensurePipeline(const QSSGRenderContextInterface &sgContext)
{
    if (iblPassPipeline && !isFragmentShaderDirty)
        return iblPassPipeline;

    auto &bufferManager = *sgContext.bufferManager().get();

    // Run through all inputs and if any texture is not created yet wait until next frame.
    // The problem with textures not created yet is that they are assumed to be sampler2DArray in glsl but
    // that could cause shader compilation failures so we need to know the actual type before trying to compile.
    for (const auto &u : std::as_const(propertyUniforms)) {
        if (u.shaderDataType == QSSGRenderShaderValue::Texture) {
            QSSGRenderImage *image = u.value.value<QSSGRenderImage *>();
            const QSSGRenderImageTexture texture = image ? bufferManager.loadRenderImage(image) : QSSGRenderImageTexture { };
            if (!image || !texture.m_texture) {
                return nullptr;
            }
        }
    }

    QByteArray vertexShader = vertexShaderStr;
    QByteArray fragmentShader = (!fragmentShaderSource.isEmpty() ? fragmentShaderSource : QByteArray(debugFragStr)) + mainFragmentSnippet;

    QSSGShaderCustomMaterialAdapter::StringPairList baseUniforms;
    for (const auto &u : std::as_const(propertyUniforms))
        baseUniforms.append({ u.typeName, u.name });

    QSSGShaderCustomMaterialAdapter::StringPairList inputOutputs;
    inputOutputs.append({ "vec3", "qt_eyeDir"_ba });

    {
        QSSGShaderCustomMaterialAdapter::StringPairList vertexUniforms;
        vertexUniforms.append({ "mat4"_ba, "qt_projectionMatrix"_ba });
        vertexUniforms.append({ "mat4"_ba, "qt_viewMatrix"_ba });

        QSSGShaderCustomMaterialAdapter::ShaderCodeAndMetaData result;
        QByteArray buf;
        QSSGShaderCustomMaterialAdapter::CustomShaderPrepWorkData scratch;
        QSSGShaderCustomMaterialAdapter::beginPrepareCustomShader(&scratch, &result, vertexShader, QSSGShaderCache::ShaderType::Vertex, false);
        QSSGShaderCustomMaterialAdapter::finishPrepareCustomShader(&buf,
                                                                   scratch,
                                                                   result,
                                                                   QSSGShaderCache::ShaderType::Vertex,
                                                                   false,
                                                                   baseUniforms,
                                                                   { },
                                                                   inputOutputs,
                                                                   { },
                                                                   vertexUniforms);
        vertexShader = result.first;
        vertexShader.append(buf);
    }

    {
        QSSGShaderCustomMaterialAdapter::ShaderCodeAndMetaData result;
        QByteArray buf;
        QSSGShaderCustomMaterialAdapter::CustomShaderPrepWorkData scratch;
        QSSGShaderCustomMaterialAdapter::beginPrepareCustomShader(&scratch, &result, fragmentShader, QSSGShaderCache::ShaderType::Fragment, false);
        QSSGShaderCustomMaterialAdapter::finishPrepareCustomShader(&buf,
                                                                   scratch,
                                                                   result,
                                                                   QSSGShaderCache::ShaderType::Fragment,
                                                                   false,
                                                                   baseUniforms,
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

    QSSGShaderFeatures features;
    features.set(QSSGShaderFeatures::Feature::AcesTonemapping, true);
    const QByteArray key = shaderPathKey + ':'
            + QCryptographicHash::hash(QByteArray(vertexShader + fragmentShader), QCryptographicHash::Algorithm::Sha1).toHex();
    iblPassPipeline = generator->compileGeneratedRhiShader(key, features, *shaderLib, *shaderCache, QSSGRhiShaderPipeline::UsedWithoutIa, { }, 1, false);

    isFragmentShaderDirty = false;

    return iblPassPipeline;
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

    {
        bindings = QSSGRhiShaderResourceBindingList();

        bindings.addUniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, dcd->ubuf, 0, uniformStride, true);

        // Add textures (mostly copy pasta from addOpaqueDepthPrePassBindings())
        int maxSamplerBinding = -1;

        QVector<QShaderDescription::InOutVariable>
                samplerVars = iblPassPipeline->fragmentStage()->shader().description().combinedImageSamplers();
        for (const QShaderDescription::InOutVariable &var :
             iblPassPipeline->vertexStage()->shader().description().combinedImageSamplers()) {
            auto it = std::find_if(samplerVars.cbegin(), samplerVars.cend(), [&var](const QShaderDescription::InOutVariable &v) {
                return var.binding == v.binding;
            });
            if (it == samplerVars.cend())
                samplerVars.append(var);
        }
        for (const QShaderDescription::InOutVariable &var : std::as_const(samplerVars))
            maxSamplerBinding = qMax(maxSamplerBinding, var.binding);

        if (maxSamplerBinding >= 0) {
            // custom property textures
            int customTexCount = iblPassPipeline->extraTextureCount();
            for (int i = 0; i < customTexCount; ++i) {
                const QSSGRhiTexture &t(iblPassPipeline->extraTextureAt(i));
                const int samplerBinding = iblPassPipeline->bindingForTexture(t.name);
                if (samplerBinding >= 0) {
                    QRhiSampler *sampler = rhiCtx->sampler(t.samplerDesc);
                    bindings.addTexture(samplerBinding, QRhiShaderResourceBinding::FragmentStage, t.texture, sampler);
                }
            }
        }
    }

    return uniformStride;
}

QT_END_NAMESPACE
