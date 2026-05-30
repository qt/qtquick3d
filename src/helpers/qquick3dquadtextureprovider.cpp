// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qquick3dquadtextureprovider_p.h"

#include <QtQuick3DRuntimeRender/private/qssgshadermaterialadapter_p.h>

#include <ssg/qssgrenderextensions.h>
#include <ssg/qssgrenderhelpers.h>
#include <ssg/qssgrendercontextcore.h>
#include <ssg/qquick3dextensionhelpers.h>


/*!
    \qmltype QuadTextureProvider
    \nativetype QQuick3DQuadTextureProvider
    \inqmlmodule QtQuick3D.Helpers
    \inherits TextureProviderExtension
    \since 6.12
    \brief Used to render a quad texture using a custom fragment shader.

    This type is used to render quad textures using custom shader code, enabling a convenient way
    to get programmable textures. By providing a fragment shader and wanted properties, a pass is created
    and a shader pipeline is built based on the provided data, passing the properties in as uniform values
    to the fragment shader.

    Built-ins provided:

    \table
    \header
    \li Keyword
    \li Type
    \li Description
    \row
    \li MAIN
    \li
    \li void MAIN() is the entry point. This function must always be present in the fragment shader provided.
    \row
    \li INPUT_UV
    \li vec2
    \li UV coordinates for current fragment. Top-right: [1, 1] and bottom-left: [0, 0].
    \row
    \li OUTPUT_SIZE
    \li vec2
    \li Size of the output texture.
    \row
    \endtable

    Custom properties gets mapped to uniforms. Any time the values change, the updated
    value will become visible in the shader. This concept may already be familiar from \l ShaderEffect.

    The name of the QML property and the GLSL variable must match. There is no separate
    declaration in the shader code for the individual uniforms. Rather, the QML property name
    can be used as-is.

    The following table lists how the types are mapped:

    \table
    \header
    \li QML Type
    \li Shader Type
    \li Notes
    \row
    \li real, int, bool
    \li float, int, bool
    \li
    \row
    \li color
    \li vec4
    \li sRGB to linear conversion is performed implicitly
    \row
    \li vector2d
    \li vec2
    \li
    \row
    \li vector3d
    \li vec3
    \li
    \row
    \li vector4d
    \li vec4
    \li
    \row
    \li matrix4x4
    \li mat4
    \li
    \row
    \li quaternion
    \li vec4
    \li scalar value is \c w
    \row
    \li rect
    \li vec4
    \li
    \row
    \li point, size
    \li vec2
    \li
    \row
    \li TextureInput
    \li sampler2D
    \li
    \endtable

    An example of outputting a simple red texture could be done the following way:
    \badcode
    Texture {
        textureProvider: QuadTextureProvider {
            width: 128
            height: 128
            fragmentShaderCode: `
                void MAIN() {
                    FRAGCOLOR = vec4(1.0, 0.0, 0.0, 1.0);
                }
            `
        }
    }
    \endcode

    Another example sampling from a \l Texture property and mixing with UV colors:
    \badcode
    Texture {
        textureProvider: QuadTextureProvider {
            fragmentShaderCode: `
            void MAIN() {
                vec2 uv = INPUT_UV;
                vec4 c = texture(checkers, uv);
                FRAGCOLOR = mix(c, vec4(uv, 1, 1), 0.5);
            }`

            property Texture checkers : Texture {
                source: "../shared/maps/checkers2.png"
            }
        }
    }
    \endcode

    The result is the following:
    \image quadtextureprovider_checkers.webp
           {2x2 grid of gradient-filled cells in teal, white, blue, and
           magenta}

    \note Providing a vertex shader is not supported, only a fragment shader.

    \note If \l Texture properties are provided it will not render until the dependent textures are available.

    \note There is currently no support for adding / removing properties at runtime, just modifying the original ones.

    \sa ShaderEffect
 */

/*!
    \qmlproperty url QuadTextureProvider::fragmentShader
    \since 6.12

    Specifies the file with the snippet of custom fragment shader code.

    The value is a URL and must either be a local file or use the qrc scheme to
    access files embedded via the Qt resource system. Relative file paths
    (without a scheme) are also accepted, in which case the file is treated as
    relative to the component (the \c{.qml} file).

    \warning Shader snippets are assumed to be trusted content. Application
    developers are advised to carefully consider the potential implications
    before allowing the loading of user-provided content that is not part of the
    application.

    \note If set, fragmentShaderCode will take precedence over fragmentShader.

    \sa fragmentShaderCode
*/

/*!
    \qmlproperty string QuadTextureProvider::fragmentShaderCode
    \since 6.12

    Specifies a snippet of custom fragment shader code.

    Used as a way to inline shader code as a string instead of providing a file.

    \note If set, this property will take precedence over \l fragmentShader.

    \sa fragmentShader
*/

/*!
    \qmlproperty int QuadTextureProvider::width
    \since 6.12
    \default 128

    Specifies the width in pixels of the output texture.

    \sa height
*/

/*!
    \qmlproperty int QuadTextureProvider::height
    \since 6.12
    \default 128

    Specifies the height in pixels of the output texture.

    \sa width
*/

/*!
    \qmlproperty enumeration QuadTextureProvider::format
    \since 6.12
    \default TexureData.RGBA16F

    This property holds the format of the output texture.

    \value TexureData.RGBA8 The color format is considered as 8-bit integer in R, G, B and alpha channels.
    \value TexureData.RGBA16F The color format is considered as 16-bit float in R,G,B and alpha channels.
    \value TexureData.RGBA32F The color format is considered as 32-bit float in R, G, B and alpha channels.
    \value TexureData.RGBE8 The color format is considered as 8-bit mantissa in the R, G, and B channels and 8-bit shared exponent.
    \value TexureData.R8 The color format is considered as 8-bit integer in R channel.
    \value TexureData.R16 The color format is considered as 16-bit integer in R channel.
    \value TexureData.R16F The color format is considered as 16-bit float in R channel.
    \value TexureData.R32F The color format is considered as 32-bit float R channel.

    \note With the exception of \c TexureData.RGBA8, not every format is supported at runtime as this
    depends on which backend is being used as well which hardware is being used.
*/

QT_BEGIN_NAMESPACE

static constexpr float g_vertexData[] = { -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
                                          1.0f,  1.0f,  0.0f, 1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 0.0f, 1.0f };

static constexpr uint16_t g_indexData[] = { 0, 1, 2, 0, 2, 3 };

static const QByteArray mainVertexSnippet = R"(
void main()
{
    qt_inputUV = attr_uv;
    gl_Position = vec4(attr_pos, 0.0, 1.0);
}
)";

static const QByteArray mainVertexSnippetFlipped = R"(
void main()
{
    qt_inputUV = attr_uv;
    qt_inputUV.y = 1.0 - qt_inputUV.y;
    gl_Position = vec4(attr_pos, 0.0, 1.0);
}
)";

static const QByteArray mainFragmentSnippet = R"(
void main()
{
    qt_customMain();
}
)";

static const QByteArray fallbackFragmentShaderStr = R"(
void MAIN()
{
    FRAGCOLOR = vec4(1.0, 0.0, 1.0, 1.0);
}
)";

static inline void insertVertexMainArgs(QByteArray &snippet)
{
    static const char *argKey = "/*%QT_ARGS_MAIN%*/";
    const int argKeyLen = int(strlen(argKey));
    const int argKeyPos = snippet.indexOf(argKey);
    if (argKeyPos >= 0)
        snippet = snippet.left(argKeyPos) + QByteArrayLiteral("inout vec3 VERTEX") + snippet.mid(argKeyPos + argKeyLen);
}

namespace {

QRhiTexture::Format rhiTextureFormatFromTextureDataFormat(QQuick3DTextureData::Format format)
{
    switch (format) {
    case QQuick3DTextureData::RGBA8:
        return QRhiTexture::Format::RGBA8;
    case QQuick3DTextureData::RGBA16F:
        return QRhiTexture::Format::RGBA16F;
    case QQuick3DTextureData::RGBA32F:
        return QRhiTexture::Format::RGBA32F;
    case QQuick3DTextureData::RGBE8:
        return QRhiTexture::Format::RGBA8;
    case QQuick3DTextureData::R8:
        return QRhiTexture::Format::R8;
    case QQuick3DTextureData::R16:
        return QRhiTexture::Format::R16;
    case QQuick3DTextureData::R16F:
        return QRhiTexture::Format::R16F;
    case QQuick3DTextureData::R32F:
        return QRhiTexture::Format::R32F;
    default:
        return QRhiTexture::Format::RGBA8;
    }
    Q_UNREACHABLE_RETURN(QRhiTexture::Format::RGBA8);
}

QByteArray generateFinalVertexShaderCode(QRhi *rhi,
                           QSSGShaderCustomMaterialAdapter::StringPairList uniforms,
                           QSSGShaderCustomMaterialAdapter::StringPairList varyings)
{
    // Handle d3d flipped textures
    const bool doFlip = rhi->isYUpInNDC() && !rhi->isYUpInFramebuffer();
    QByteArray vertexShader = doFlip ? mainVertexSnippetFlipped : mainVertexSnippet;

    QSSGShaderCustomMaterialAdapter::ShaderCodeAndMetaData result;
    QByteArray buf;
    QSSGShaderCustomMaterialAdapter::CustomShaderPrepWorkData scratch;
    QSSGShaderCustomMaterialAdapter::beginPrepareCustomShader(&scratch, &result, vertexShader, QSSGShaderCache::ShaderType::Vertex, false);
    QSSGShaderCustomMaterialAdapter::finishPrepareCustomShader(&buf,
                                                               scratch,
                                                               result,
                                                               QSSGShaderCache::ShaderType::Vertex,
                                                               false,
                                                               uniforms,
                                                               { },
                                                               varyings,
                                                               { },
                                                               { });
    insertVertexMainArgs(result.first);
    return result.first + buf;
}

QByteArray generateFinalFragmentShaderCode(const QByteArray &baseFragmentShaderCode,
                                           QSSGShaderCustomMaterialAdapter::StringPairList uniforms,
                                           QSSGShaderCustomMaterialAdapter::StringPairList varyings)
{
    QSSGShaderCustomMaterialAdapter::ShaderCodeAndMetaData result;
    QByteArray buf;
    QSSGShaderCustomMaterialAdapter::CustomShaderPrepWorkData scratch;
    QSSGShaderCustomMaterialAdapter::beginPrepareCustomShader(&scratch, &result, baseFragmentShaderCode, QSSGShaderCache::ShaderType::Fragment, false);
    QSSGShaderCustomMaterialAdapter::finishPrepareCustomShader(&buf,
                                                               scratch,
                                                               result,
                                                               QSSGShaderCache::ShaderType::Fragment,
                                                               false,
                                                               uniforms,
                                                               varyings,
                                                               { },
                                                               { },
                                                               { });
    return result.first + buf;
}

QSSGRhiShaderPipelinePtr compileShader(QSSGRenderContextInterface *sgContext,
                                       const QByteArray &shaderPathKey,
                                       const QByteArray &vertexShader,
                                       const QByteArray &fragmentShader)
{
    QSSGProgramGenerator *generator = sgContext->shaderProgramGenerator().get();
    QSSGShaderLibraryManager *shaderLib = sgContext->shaderLibraryManager().get();
    QSSGShaderCache *shaderCache = sgContext->shaderCache().get();

    generator->beginProgram();
    auto vertex = generator->getStage(QSSGShaderGeneratorStage::Vertex);
    vertex->addIncoming("attr_pos", "vec2");
    vertex->addIncoming("attr_uv", "vec2");
    vertex->append(vertexShader);

    generator->getStage(QSSGShaderGeneratorStage::Fragment)->append(fragmentShader);

    QSSGShaderFeatures features;
    const QByteArray key = shaderPathKey + ':'
            + QCryptographicHash::hash(QByteArray(vertexShader + fragmentShader), QCryptographicHash::Algorithm::Sha1)
                      .toHex();
    return generator->compileGeneratedRhiShader(key, features, *shaderLib, *shaderCache, QSSGRhiShaderPipeline::UsedWithoutIa, { }, 1, false);
}

} // namespace

class QSSGQuadTextureProvider : public QSSGRenderTextureProviderExtension
{
public:
    explicit QSSGQuadTextureProvider(QQuick3DQuadTextureProvider *ext);
    ~QSSGQuadTextureProvider() override;
    bool prepareData(QSSGFrameData &data) override;
    void prepareRender(QSSGFrameData &data) override;
    void render(QSSGFrameData &data) override;
    void resetForFrame() override;

    QQuick3DQuadTextureProvider::DirtyT dirtyFlag = 0;

    QByteArray fragmentShaderSource;
    QList<QSSGBaseTypeProperty> propertyUniforms;
    QQuick3DTextureData::Format format = QQuick3DTextureData::Format::RGBA16F;
    int width = 128;
    int height = 128;

    QByteArray shaderPathKey = "quad texture provider --";

private:
    QPointer<QQuick3DQuadTextureProvider> m_ext;

    QSSGRhiShaderResourceBindingList m_srbBindings;
    QSSGRhiShaderPipelinePtr m_shaderPipeline;

    std::unique_ptr<QRhiBuffer> m_vertexBuffer;
    std::unique_ptr<QRhiBuffer> m_indexBuffer;

    std::unique_ptr<QRhiTexture> m_outputTexture;
    std::unique_ptr<QRhiTexture> m_outputTextureOld;
    std::unique_ptr<QRhiTextureRenderTarget> m_renderTarget;
    std::unique_ptr<QRhiRenderPassDescriptor> m_renderPassDesc;

    std::unique_ptr<QRhiGraphicsPipeline> m_graphicsPipeline;
    QRhiShaderResourceBindings* m_srb = nullptr; // not owned
};

QSSGQuadTextureProvider::QSSGQuadTextureProvider(QQuick3DQuadTextureProvider *ext) : m_ext(ext) { }

QSSGQuadTextureProvider::~QSSGQuadTextureProvider() { }

bool QSSGQuadTextureProvider::prepareData(QSSGFrameData &data)
{
    QSSGRenderContextInterface *ctxIfx = data.contextInterface();
    auto bufferManager = ctxIfx->bufferManager().get();
    QSSGRhiContext *rhiCtx = ctxIfx->rhiContext().get();
    if (!rhiCtx)
        return false;

    QSSGExtensionId extensionId = m_ext ? QQuick3DExtensionHelpers::getExtensionId(*m_ext) : QSSGExtensionId { };
    if (QQuick3DExtensionHelpers::isNull(extensionId))
        return false;

    // TODO: should know if new properties are added or removed
    bool needsRebuild = false;
    if (dirtyFlag & QQuick3DQuadTextureProvider::Dirty::Dimensions)
        needsRebuild = true;
    if (dirtyFlag & QQuick3DQuadTextureProvider::Dirty::Format)
        needsRebuild = true;
    if (dirtyFlag & QQuick3DQuadTextureProvider::Dirty::FragmentShader)
        needsRebuild = true;
    if (m_shaderPipeline && !needsRebuild)
        return dirtyFlag & QQuick3DQuadTextureProvider::Dirty::TrackedProperty;

    if (!m_shaderPipeline || dirtyFlag & QQuick3DQuadTextureProvider::Dirty::FragmentShader) {
        // Run through all inputs and if any texture is not created yet wait until next frame.
        // The problem with textures not created yet is that they are assumed to be sampler2DArray in glsl but
        // that could cause shader compilation failures so we need to know the actual type before trying to compile.
        for (const auto &u : std::as_const(propertyUniforms)) {
            if (u.shaderDataType == QSSGRenderShaderValue::Texture) {
                QSSGRenderImage *image = u.value.value<QSSGRenderImage *>();
                const QSSGRenderImageTexture texture = image ? bufferManager->loadRenderImage(image) : QSSGRenderImageTexture { };
                if (!image || !texture.m_texture) {
                    return false;
                }
            }
        }

        QByteArray fragmentShaderCode = (!fragmentShaderSource.isEmpty() ? fragmentShaderSource : fallbackFragmentShaderStr)
                            + mainFragmentSnippet;

        QSSGShaderCustomMaterialAdapter::StringPairList baseUniforms;
        for (const auto &u : std::as_const(propertyUniforms))
            baseUniforms.append({ u.typeName, u.name });

        baseUniforms.append({ "vec2", "qt_outputSize" });

        QSSGShaderCustomMaterialAdapter::StringPairList baseInputOutputs;
        baseInputOutputs.append({ "vec2", "qt_inputUV" });

        QByteArray vertexShader = generateFinalVertexShaderCode(rhiCtx->rhi(), baseUniforms, baseInputOutputs);
        QByteArray fragmentShader = generateFinalFragmentShaderCode(fragmentShaderCode, baseUniforms, baseInputOutputs);

        QSSGRenderContextInterface *sgContext = data.contextInterface();

        m_shaderPipeline = compileShader(sgContext, shaderPathKey, vertexShader, fragmentShader);
        // If there's an issue compiling try using the fallback fragment shader. Might be something wrong with the
        // provided fragment shader, e.g. syntax error. This avoids asserting at runtime.
        if (!m_shaderPipeline) {
            fragmentShaderCode = fallbackFragmentShaderStr + mainFragmentSnippet;
            fragmentShader = generateFinalFragmentShaderCode(fragmentShaderCode, baseUniforms, baseInputOutputs);
            m_shaderPipeline = compileShader(sgContext, shaderPathKey, vertexShader, fragmentShader);
        }
    }

    QRhi *rhi = rhiCtx->rhi();

    if (!m_outputTexture || dirtyFlag & QQuick3DQuadTextureProvider::Dirty::Dimensions
        || dirtyFlag & QQuick3DQuadTextureProvider::Dirty::Format) {
        // Workaround: Preserve the previous texture until the end of the frame.
        //
        // QuadTextureProviders are rendered sequentially. If recreating the output
        // texture destroys the old QRhiTexture immediately, consumers that have not
        // rendered yet may still hold a pointer to the previous texture, resulting in
        // a dangling reference during the current frame.
        //
        // By the next frame, surfaceChanged() will have propagated and consumers are
        // expected to have updated to the new texture. The old texture is then released
        // in resetForFrame().
        if (m_outputTexture)
            m_outputTextureOld = std::move(m_outputTexture);
        m_outputTexture.reset(rhi->newTexture(rhiTextureFormatFromTextureDataFormat(format), QSize(width, height), 1, QRhiTexture::RenderTarget));
        m_outputTexture->create();

        m_renderTarget.reset(rhi->newTextureRenderTarget({ m_outputTexture.get() }));
        m_renderPassDesc.reset(m_renderTarget->newCompatibleRenderPassDescriptor());
        m_renderTarget->setRenderPassDescriptor(m_renderPassDesc.get());
        m_renderTarget->create();

        QSSGRenderExtensionHelpers::registerRenderResult(data, extensionId, m_outputTexture.get());

        m_graphicsPipeline.reset();
    }

    return true;
}

void QSSGQuadTextureProvider::prepareRender(QSSGFrameData &data)
{
    QSSGRenderContextInterface *ctxIfx = data.contextInterface();
    QSSGRhiContext *rhiCtx = ctxIfx->rhiContext().get();
    if (!rhiCtx || !m_shaderPipeline)
        return;

    QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiCtx);
    QRhi *rhi = rhiCtx->rhi();

    QSSGRhiDrawCallData *dcd = &rhiCtxD->drawCallData({ (void *)this, nullptr, nullptr, 0 });
    if (!dcd->ubuf) {
        const int uniformStride = rhiCtx->rhi()->ubufAligned(m_shaderPipeline->ub0Size());
        dcd->ubuf = rhiCtx->rhi()->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, uniformStride);
        dcd->ubuf->create();
    }

    const bool trackedPropertyDirty = dirtyFlag & QQuick3DQuadTextureProvider::Dirty::TrackedProperty;
    const bool dimensionsDirty = dirtyFlag & QQuick3DQuadTextureProvider::Dirty::Dimensions;
    const bool pipelineDirty = !m_graphicsPipeline || (dirtyFlag & QQuick3DQuadTextureProvider::Dirty::Format)
            || (dirtyFlag & QQuick3DQuadTextureProvider::Dirty::FragmentShader);
    // SRB only needs rebuild when texture bindings may have changed
    const bool srbDirty = pipelineDirty || trackedPropertyDirty || !m_srb;

    // Update uniforms / textures
    if (trackedPropertyDirty || dimensionsDirty) {

        char *ubufData = dcd->ubuf->beginFullDynamicBufferUpdateForCurrentFrame();

        if (trackedPropertyDirty) {
            // rebuild transient texture list
            m_shaderPipeline->resetExtraTextures();

            QSSGBufferManager *bufferManager = ctxIfx->bufferManager().get();

            for (const auto &u : std::as_const(propertyUniforms)) {
                m_shaderPipeline->setShaderResources(ubufData, *bufferManager, u.name, u.value, u.shaderDataType);
            }
        }

        if (dimensionsDirty) {
            m_shaderPipeline->setUniformValue(ubufData, "qt_outputSize", QVector2D(width, height), QSSGRenderShaderValue::Vec2);
        }
        dcd->ubuf->endFullDynamicBufferUpdateForCurrentFrame();
    }

    // Static geometry (create once)
    if (!m_vertexBuffer && !m_indexBuffer) {
        // 1 quad (2 trianges), pos + uv.  4 vertices, 5 values each (x, y, z, u, v)
        m_vertexBuffer.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, 5 * 4 * sizeof(float)));
        m_vertexBuffer->create();

        // 6 indexes (2 triangles)
        m_indexBuffer.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, 6 * sizeof(uint16_t)));
        m_indexBuffer->create();
        QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();
        updates->uploadStaticBuffer(m_vertexBuffer.get(), g_vertexData);
        updates->uploadStaticBuffer(m_indexBuffer.get(), g_indexData);
        rhiCtx->commandBuffer()->resourceUpdate(updates);
    }

    // SRB rebuild
    if (srbDirty) {
        rhiCtxD->releaseCachedSrb(m_srbBindings);
        m_srbBindings = QSSGRhiShaderResourceBindingList();
        m_srbBindings.addUniformBuffer(0, QRhiShaderResourceBinding::FragmentStage, dcd->ubuf);

        // Add textures (mostly copy pasta from addOpaqueDepthPrePassBindings())
        int maxSamplerBinding = -1;
        QVector<QShaderDescription::InOutVariable> samplerVars = m_shaderPipeline->fragmentStage()->shader().description().combinedImageSamplers();
        for (const QShaderDescription::InOutVariable &var :
             m_shaderPipeline->vertexStage()->shader().description().combinedImageSamplers()) {
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
            int customTexCount = m_shaderPipeline->extraTextureCount();
            for (int i = 0; i < customTexCount; ++i) {
                const QSSGRhiTexture &t(m_shaderPipeline->extraTextureAt(i));
                const int samplerBinding = m_shaderPipeline->bindingForTexture(t.name);
                if (samplerBinding >= 0) {
                    QRhiSampler *sampler = rhiCtx->sampler(t.samplerDesc);
                    m_srbBindings.addTexture(samplerBinding, QRhiShaderResourceBinding::FragmentStage, t.texture, sampler);
                }
            }
        }

        m_srb = rhiCtxD->srb(m_srbBindings);
    }

    // Graphics pipeline rebuild
    if (pipelineDirty) {

        m_graphicsPipeline.reset(rhi->newGraphicsPipeline());
        m_graphicsPipeline->setShaderStages({ *m_shaderPipeline->vertexStage(), *m_shaderPipeline->fragmentStage() });

        // 2 Attributes, Position (vec3) + UV (vec2)
        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({ { 5 * sizeof(float) } });
        inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
                                    { 0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float) } });
        m_graphicsPipeline->setVertexInputLayout(inputLayout);
        m_graphicsPipeline->setShaderResourceBindings(m_srb);
        m_graphicsPipeline->setRenderPassDescriptor(m_renderPassDesc.get());

        m_graphicsPipeline->create();
    }

    dirtyFlag = 0;
}

void QSSGQuadTextureProvider::render(QSSGFrameData &data)
{
    const auto &ctxIfx = data.contextInterface();
    const auto &rhiCtx = ctxIfx->rhiContext();
    if (!rhiCtx)
        return;

    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    cb->debugMarkBegin(QByteArrayLiteral("Quick3D Quad Texture Provider"));

    cb->beginPass(m_renderTarget.get(), Qt::transparent, { 1.0f, 0 }, nullptr, rhiCtx->commonPassFlags());
    cb->setViewport(QRhiViewport(0, 0, width, height));
    cb->setScissor(QRhiScissor(0, 0, width, height));

    cb->setGraphicsPipeline(m_graphicsPipeline.get());

    QSSGRhiContextPrivate *rhiCtxD = QSSGRhiContextPrivate::get(rhiCtx.get());
    auto srb = rhiCtxD->srb(m_srbBindings);
    cb->setShaderResources(srb);

    QRhiCommandBuffer::VertexInput vb(m_vertexBuffer.get(), 0);
    cb->setVertexInput(0, 1, &vb, m_indexBuffer.get(), QRhiCommandBuffer::IndexFormat::IndexUInt16);
    cb->drawIndexed(6);
    cb->endPass();

    cb->debugMarkEnd();

}

void QSSGQuadTextureProvider::resetForFrame() {
    m_outputTextureOld.reset();
}

QQuick3DQuadTextureProvider::QQuick3DQuadTextureProvider(QQuick3DObject *parent)
    : QQuick3DTextureProviderExtension(parent)
    , QQuick3DPropertyChangedTracker(this, QQuick3DSuperClassInfo<QQuick3DQuadTextureProvider>())
{
}

QQuick3DQuadTextureProvider::~QQuick3DQuadTextureProvider() { }

QSSGRenderGraphObject *QQuick3DQuadTextureProvider::updateSpatialNode(QSSGRenderGraphObject *node)
{
    auto n = static_cast<QSSGQuadTextureProvider *>(node);

    if (!n) {
        n = new QSSGQuadTextureProvider(this);
    }

    if (m_dirtyFlag & Dirty::Dimensions) {
        n->width = m_width;
        n->height = m_height;
    }

    if (m_dirtyFlag & Dirty::Format)
        n->format = m_format;

    if (m_dirtyFlag & Dirty::FragmentShader) {
        const QQmlContext *context = qmlContext(this);
        n->fragmentShaderSource = !m_fragmentShaderCode.isEmpty() ? m_fragmentShaderCode.toUtf8()
                : !m_fragmentShader.isEmpty() ? QSSGShaderUtils::resolveShader(m_fragmentShader, context, n->shaderPathKey)
                                              : QByteArray();
    }

    if (m_dirtyFlag & Dirty::TrackedProperty) {
        n->propertyUniforms = extractProperties();
    }

    if (m_dirtyFlag != 0)
        emit surfaceChanged();

    n->dirtyFlag = m_dirtyFlag;
    m_dirtyFlag = 0;

    return n;
}

void QQuick3DQuadTextureProvider::markTrackedPropertyDirty(QMetaProperty property, DirtyPropertyHint hint)
{
    Q_UNUSED(property);
    Q_UNUSED(hint);
    markDirty(Dirty::TrackedProperty);
}

void QQuick3DQuadTextureProvider::markDirty(Dirty v)
{
    m_dirtyFlag |= v;
    update();
}

QUrl QQuick3DQuadTextureProvider::fragmentShader() const
{
    return m_fragmentShader;
}

void QQuick3DQuadTextureProvider::setFragmentShader(const QUrl &newFragmentShader)
{
    if (m_fragmentShader == newFragmentShader)
        return;
    m_fragmentShader = newFragmentShader;
    emit fragmentShaderChanged();
    markDirty(Dirty::FragmentShader);
}

QString QQuick3DQuadTextureProvider::fragmentShaderCode() const
{
    return m_fragmentShaderCode;
}

void QQuick3DQuadTextureProvider::setFragmentShaderCode(const QString &newFragmentShaderCode)
{
    if (m_fragmentShaderCode == newFragmentShaderCode)
        return;
    m_fragmentShaderCode = newFragmentShaderCode;
    emit fragmentShaderCodeChanged();
    markDirty(Dirty::FragmentShader);
}


int QQuick3DQuadTextureProvider::width() const
{
    return m_width;
}

void QQuick3DQuadTextureProvider::setWidth(int newWidth)
{
    newWidth = qMax(1, newWidth);
    if (m_width == newWidth)
        return;
    m_width = newWidth;
    emit widthChanged();
    markDirty(Dirty::Dimensions);
}

int QQuick3DQuadTextureProvider::height() const
{
    return m_height;
}

void QQuick3DQuadTextureProvider::setHeight(int newHeight)
{
    newHeight = qMax(1, newHeight);
    if (m_height == newHeight)
        return;
    m_height = newHeight;
    emit heightChanged();
    markDirty(Dirty::Dimensions);
}

QQuick3DTextureData::Format QQuick3DQuadTextureProvider::format() const
{
    return m_format;
}

void QQuick3DQuadTextureProvider::setFormat(QQuick3DTextureData::Format newFormat)
{
    if (m_format == newFormat)
        return;
    m_format = newFormat;
    emit formatChanged();
    markDirty(Dirty::Format);
}

QT_END_NAMESPACE
