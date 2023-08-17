// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuick3DRuntimeRender/private/qssgrhieffectsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhiquadrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <qtquick3d_tracepoints_p.h>

#include <QtQuick3DUtils/private/qssgassert_p.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcEffectSystem);
Q_LOGGING_CATEGORY(lcEffectSystem, "qt.quick3d.effects");

struct QSSGRhiEffectTexture
{
    QRhiTexture *texture = nullptr;
    QRhiRenderPassDescriptor *renderPassDescriptor = nullptr;
    QRhiTextureRenderTarget *renderTarget = nullptr;
    QByteArray name;

    QSSGRhiSamplerDescription desc;
    QSSGAllocateBufferFlags flags;

    ~QSSGRhiEffectTexture()
    {
        delete texture;
        delete renderPassDescriptor;
        delete renderTarget;
    }
    QSSGRhiEffectTexture &operator=(const QSSGRhiEffectTexture &) = delete;
};

QSSGRhiEffectSystem::QSSGRhiEffectSystem(const std::shared_ptr<QSSGRenderContextInterface> &sgContext)
    : m_sgContext(sgContext)
{
}

QSSGRhiEffectSystem::~QSSGRhiEffectSystem()
{
    releaseResources();
}

void QSSGRhiEffectSystem::setup(QSize outputSize)
{
    if (outputSize.isEmpty()) {
        releaseResources();
        return;
    }
    m_outSize = outputSize;
}

QSSGRhiEffectTexture *QSSGRhiEffectSystem::findTexture(const QByteArray &bufferName)
{
    auto findTexture = [bufferName](const QSSGRhiEffectTexture *rt){ return rt->name == bufferName; };
    const auto foundIt = std::find_if(m_textures.cbegin(), m_textures.cend(), findTexture);
    QSSGRhiEffectTexture *result = foundIt == m_textures.cend() ? nullptr : *foundIt;
    return result;
}

QSSGRhiEffectTexture *QSSGRhiEffectSystem::getTexture(const QByteArray &bufferName,
                                                      const QSize &size,
                                                      QRhiTexture::Format format,
                                                      bool isFinalOutput,
                                                      const QSSGRenderEffect *inEffect)
{
    QSSGRhiEffectTexture *result = findTexture(bufferName);
    const bool gotMatch = result != nullptr;

    // If not found, look for an unused texture
    if (!result) {
        // ### This could be enhanced to try to find a texture with the right
        // size/format/flags first. It is not essential because the texture will be
        // recreated below if the size or format does not match, but it would
        // be more optimal (for Effects with Buffers with sizeMultipliers on
        // them, or ones that use different formats) to look for a matching
        // size/format too instead of picking the first unused texture.
        auto findUnused = [](const QSSGRhiEffectTexture *rt){ return rt->name.isEmpty(); };
        const auto found = std::find_if(m_textures.cbegin(), m_textures.cend(), findUnused);
        if (found != m_textures.cend()) {
            result = *found;
            result->desc = {};
        }
    }

    if (!result) {
        result = new QSSGRhiEffectTexture {};
        m_textures.append(result);
    }

    QRhi *rhi = m_sgContext->rhi();
    const bool formatChanged = result->texture && result->texture->format() != format;
    const bool needsRebuild = result->texture && (result->texture->pixelSize() != size || formatChanged);

    QRhiTexture::Flags flags = QRhiTexture::RenderTarget;
    if (isFinalOutput) // play nice with progressive/temporal AA
        flags |= QRhiTexture::UsedAsTransferSource;

    if (!result->texture) {
        result->texture = rhi->newTexture(format, size, 1, flags);
        result->texture->create();
    } else if (needsRebuild) {
        result->texture->setFlags(flags);
        result->texture->setPixelSize(size);
        result->texture->setFormat(format);
        result->texture->create();
    }

    if (!result->renderTarget) {
        result->renderTarget = rhi->newTextureRenderTarget({ result->texture });
        result->renderPassDescriptor = result->renderTarget->newCompatibleRenderPassDescriptor();
        result->renderTarget->setRenderPassDescriptor(result->renderPassDescriptor);
        result->renderTarget->create();
        m_pendingClears.insert(result->renderTarget);
    } else if (needsRebuild) {
        if (formatChanged) {
            delete result->renderPassDescriptor;
            result->renderPassDescriptor = result->renderTarget->newCompatibleRenderPassDescriptor();
            result->renderTarget->setRenderPassDescriptor(result->renderPassDescriptor);
        }
        result->renderTarget->create();
        m_pendingClears.insert(result->renderTarget);
    }

    if (!gotMatch) {
        QByteArray rtName = inEffect->debugObjectName.toLatin1();
        rtName += QByteArrayLiteral(" effect pass ");
        rtName += bufferName;
        result->renderTarget->setName(rtName);
    }

    result->name = bufferName;
    return result;
}

void QSSGRhiEffectSystem::releaseTexture(QSSGRhiEffectTexture *texture)
{
    // Mark as unused by setting the name to empty, unless the Buffer had scene
    // lifetime on it (then it needs to live on for ever).
    if (!texture->flags.isSceneLifetime())
        texture->name = {};
}

void QSSGRhiEffectSystem::releaseTextures()
{
    for (auto *t : std::as_const(m_textures))
        releaseTexture(t);
}

QRhiTexture *QSSGRhiEffectSystem::process(const QSSGRenderEffect &firstEffect,
                                          QRhiTexture *inTexture,
                                          QRhiTexture *inDepthTexture,
                                          QVector2D cameraClipRange)
{
    QSSG_ASSERT(m_sgContext != nullptr, return inTexture);
    const auto &rhiContext = m_sgContext->rhiContext();
    const auto &renderer = m_sgContext->renderer();
    QSSG_ASSERT(rhiContext && renderer, return inTexture);

    m_depthTexture = inDepthTexture;
    m_cameraClipRange = cameraClipRange;

    m_currentUbufIndex = 0;
    auto *currentEffect = &firstEffect;
    QSSGRhiEffectTexture firstTex{ inTexture, nullptr, nullptr, {}, {}, {} };
    auto *latestOutput = doRenderEffect(currentEffect, &firstTex);
    firstTex.texture = nullptr; // make sure we don't delete inTexture when we go out of scope

    while ((currentEffect = currentEffect->m_nextEffect)) {
        auto *effectOut = doRenderEffect(currentEffect, latestOutput);
        releaseTexture(latestOutput);
        latestOutput = effectOut;
    }

    releaseTextures();
    return latestOutput ? latestOutput->texture : nullptr;
}

void QSSGRhiEffectSystem::releaseResources()
{
    qDeleteAll(m_textures);
    m_textures.clear();

    m_shaderPipelines.clear();
}

QSSGRenderTextureFormat::Format QSSGRhiEffectSystem::overriddenOutputFormat(const QSSGRenderEffect *inEffect)
{
    QSSGRenderTextureFormat::Format format = QSSGRenderTextureFormat::Unknown;
    for (const QSSGRenderEffect::Command &c : inEffect->commands) {
        QSSGCommand *cmd = c.command;
        if (cmd->m_type == CommandType::BindTarget) {
            QSSGBindTarget *targetCmd = static_cast<QSSGBindTarget *>(cmd);
            format = targetCmd->m_outputFormat == QSSGRenderTextureFormat::Unknown
                    ? inEffect->outputFormat : targetCmd->m_outputFormat.format;
        }
    }
    return format;
}

QSSGRhiEffectTexture *QSSGRhiEffectSystem::doRenderEffect(const QSSGRenderEffect *inEffect,
                                                          QSSGRhiEffectTexture *inTexture)
{
    // Run through the effect commands and render the effect.
    qCDebug(lcEffectSystem) << "START effect " << inEffect->className;
    QSSGRhiEffectTexture *finalOutputTexture = nullptr;
    QSSGRhiEffectTexture *currentOutput = nullptr;
    QSSGRhiEffectTexture *currentInput = inTexture;
    for (const QSSGRenderEffect::Command &c : inEffect->commands) {
        QSSGCommand *theCommand = c.command;
        qCDebug(lcEffectSystem).noquote() << "    >" << theCommand->typeAsString() << "--" << theCommand->debugString();

        switch (theCommand->m_type) {
        case CommandType::AllocateBuffer:
            allocateBufferCmd(static_cast<QSSGAllocateBuffer *>(theCommand), inTexture, inEffect);
            break;

        case CommandType::ApplyBufferValue: {
            auto *applyCommand = static_cast<QSSGApplyBufferValue *>(theCommand);

            /*
                BufferInput { buffer: buf }
                  -> INPUT (qt_inputTexture) in the shader samples the texture for Buffer buf in the pass
                BufferInput { sampler: "ttt" }
                  -> ttt in the shader samples the input texture for the pass
                     (ttt also needs to be a TextureInput with a Texture{} to get the sampler declared in the shader code,
                      beware that without the BufferInput the behavior would change: ttt would then sample a dummy texture)
                BufferInput { buffer: buf; sampler: "ttt" }
                  -> ttt in the shader samples the texture for Buffer buf in the pass
            */

            auto *buffer = applyCommand->m_bufferName.isEmpty() ? inTexture : findTexture(applyCommand->m_bufferName);
            if (applyCommand->m_samplerName.isEmpty())
                currentInput = buffer;
            else
                addTextureToShaderPipeline(applyCommand->m_samplerName, buffer->texture, buffer->desc);
            break;
        }

        case CommandType::ApplyInstanceValue:
            applyInstanceValueCmd(static_cast<QSSGApplyInstanceValue *>(theCommand), inEffect);
            break;

        case CommandType::ApplyValue:
            applyValueCmd(static_cast<QSSGApplyValue *>(theCommand), inEffect);
            break;

        case CommandType::BindBuffer: {
            auto *bindCmd = static_cast<QSSGBindBuffer *>(theCommand);
            currentOutput = findTexture(bindCmd->m_bufferName);
            break;
        }

        case CommandType::BindShader:
            bindShaderCmd(static_cast<QSSGBindShader *>(theCommand), inEffect);
            break;

        case CommandType::BindTarget: {
            auto targetCmd = static_cast<QSSGBindTarget*>(theCommand);
            // matches overriddenOutputFormat()
            QSSGRenderTextureFormat::Format f = targetCmd->m_outputFormat == QSSGRenderTextureFormat::Unknown ?
                        inEffect->outputFormat : targetCmd->m_outputFormat.format;
            // f is now either Unknown (common case), or if the effect overrides the output format, then that
            QRhiTexture::Format rhiFormat = f == QSSGRenderTextureFormat::Unknown ?
                        currentInput->texture->format() : QSSGBufferManager::toRhiFormat(f);
            qCDebug(lcEffectSystem) << "      Target format override" << QSSGBaseTypeHelpers::toString(f) << "Effective RHI format" << rhiFormat;
            // Make sure we use different names for each effect inside one frame
            QByteArray tmpName = QByteArrayLiteral("__output_").append(QByteArray::number(m_currentUbufIndex));
            currentOutput = getTexture(tmpName, m_outSize, rhiFormat, true, inEffect);
            finalOutputTexture = currentOutput;
            break;
        }

        case CommandType::Render:
            renderCmd(currentInput, currentOutput);
            currentInput = inTexture; // default input for each new pass is defined to be original input
            break;

        default:
            qWarning() << "Effect command" << theCommand->typeAsString() << "not implemented";
            break;
        }
    }
    // TODO: release textures used by this effect now, instead of after processing all the effects
    qCDebug(lcEffectSystem) << "END effect " << inEffect->className;
    return finalOutputTexture;
}

void QSSGRhiEffectSystem::allocateBufferCmd(const QSSGAllocateBuffer *inCmd, QSSGRhiEffectTexture *inTexture, const QSSGRenderEffect *inEffect)
{
    // Note: Allocate is used both to allocate new, and refer to buffer created earlier
    QSize bufferSize(m_outSize * qreal(inCmd->m_sizeMultiplier));

    QSSGRenderTextureFormat f = inCmd->m_format;
    QRhiTexture::Format rhiFormat = (f == QSSGRenderTextureFormat::Unknown) ? inTexture->texture->format()
                                                                            : QSSGBufferManager::toRhiFormat(f);

    QSSGRhiEffectTexture *buf = getTexture(inCmd->m_name, bufferSize, rhiFormat, false, inEffect);
    auto filter = toRhi(inCmd->m_filterOp);
    auto tiling = toRhi(inCmd->m_texCoordOp);
    buf->desc = { filter, filter, QRhiSampler::None, tiling, tiling, QRhiSampler::Repeat };
    buf->flags = inCmd->m_bufferFlags;
}

void QSSGRhiEffectSystem::applyInstanceValueCmd(const QSSGApplyInstanceValue *inCmd, const QSSGRenderEffect *inEffect)
{
    if (!m_currentShaderPipeline)
        return;

    const bool setAll = inCmd->m_propertyName.isEmpty();
    for (const QSSGRenderEffect::Property &property : std::as_const(inEffect->properties)) {
        if (setAll || property.name == inCmd->m_propertyName) {
            m_currentShaderPipeline->setUniformValue(m_currentUBufData, property.name, property.value, property.shaderDataType);
            //qCDebug(lcEffectSystem) << "setUniformValue" << property.name << toString(property.shaderDataType) << "to" << property.value;
        }
    }
    for (const QSSGRenderEffect::TextureProperty &textureProperty : std::as_const(inEffect->textureProperties)) {
        if (setAll || textureProperty.name == inCmd->m_propertyName) {
            bool texAdded = false;
            QSSGRenderImage *image = textureProperty.texImage;
            if (image) {
                const auto &theBufferManager(m_sgContext->bufferManager());
                const QSSGRenderImageTexture texture = theBufferManager->loadRenderImage(image);
                if (texture.m_texture) {
                    const QSSGRhiSamplerDescription desc{
                        toRhi(textureProperty.minFilterType),
                        toRhi(textureProperty.magFilterType),
                        textureProperty.mipFilterType != QSSGRenderTextureFilterOp::None ? toRhi(textureProperty.mipFilterType) : QRhiSampler::None,
                        toRhi(textureProperty.horizontalClampType),
                        toRhi(textureProperty.verticalClampType),
                        QRhiSampler::Repeat
                    };
                    addTextureToShaderPipeline(textureProperty.name, texture.m_texture, desc);
                    texAdded = true;
                }
            }
            if (!texAdded) {
                // Something went wrong, e.g. image file not found. Still need to add a dummy texture for the shader
                qCDebug(lcEffectSystem) << "Using dummy texture for property" << textureProperty.name;
                addTextureToShaderPipeline(textureProperty.name, nullptr, {});
            }
        }
    }
}

void QSSGRhiEffectSystem::applyValueCmd(const QSSGApplyValue *inCmd, const QSSGRenderEffect *inEffect)
{
    if (!m_currentShaderPipeline)
        return;

    const auto &properties = inEffect->properties;
    const auto foundIt = std::find_if(properties.cbegin(), properties.cend(), [inCmd](const QSSGRenderEffect::Property &prop) {
        return (prop.name == inCmd->m_propertyName);
    });

    if (foundIt != properties.cend())
        m_currentShaderPipeline->setUniformValue(m_currentUBufData, inCmd->m_propertyName, inCmd->m_value, foundIt->shaderDataType);
    else
        qWarning() << "Could not find effect property" << inCmd->m_propertyName;
}

static const char *effect_builtin_textureMapUV =
        "vec2 qt_effectTextureMapUV(vec2 uv)\n"
        "{\n"
        "    return uv;\n"
        "}\n";

static const char *effect_builtin_textureMapUVFlipped =
        "vec2 qt_effectTextureMapUV(vec2 uv)\n"
        "{\n"
        "    return vec2(uv.x, 1.0 - uv.y);\n"
        "}\n";

QSSGRhiShaderPipelinePtr QSSGRhiEffectSystem::buildShaderForEffect(const QSSGBindShader &inCmd,
                                                                   QSSGProgramGenerator &generator,
                                                                   QSSGShaderLibraryManager &shaderLib,
                                                                   QSSGShaderCache &shaderCache,
                                                                   bool isYUpInFramebuffer)
{
    const auto &key = inCmd.m_shaderPathKey;
    qCDebug(lcEffectSystem) << "    generating new shader pipeline for: " << key;

    generator.beginProgram();

    {
        const QByteArray src = shaderLib.getShaderSource(inCmd.m_shaderPathKey, QSSGShaderCache::ShaderType::Vertex);
        QSSGStageGeneratorBase *vStage = generator.getStage(QSSGShaderGeneratorStage::Vertex);
        // The variation based on isYUpInFramebuffer is captured in 'key' as
        // well, so it is safe to vary the source code here.
        vStage->append(isYUpInFramebuffer ? effect_builtin_textureMapUV : effect_builtin_textureMapUVFlipped);
        vStage->append(src);
    }
    {
        const QByteArray src = shaderLib.getShaderSource(inCmd.m_shaderPathKey, QSSGShaderCache::ShaderType::Fragment);
        QSSGStageGeneratorBase *fStage = generator.getStage(QSSGShaderGeneratorStage::Fragment);
        fStage->append(src);
    }

    return generator.compileGeneratedRhiShader(key,
                                               shaderLib.getShaderMetaData(inCmd.m_shaderPathKey, QSSGShaderCache::ShaderType::Fragment).features,
                                                shaderLib,
                                                shaderCache,
                                                QSSGRhiShaderPipeline::UsedWithoutIa);
}

void QSSGRhiEffectSystem::bindShaderCmd(const QSSGBindShader *inCmd, const QSSGRenderEffect *inEffect)
{
    QElapsedTimer timer;
    timer.start();

    m_currentTextures.clear();
    m_pendingClears.clear();
    m_currentShaderPipeline = nullptr;

    QRhi *rhi = m_sgContext->rhi();
    const auto &shaderLib = m_sgContext->shaderLibraryManager();
    const auto &shaderCache = m_sgContext->shaderCache();

    // Now we need a proper unique key (unique in the scene), the filenames are
    // not sufficient. This means that using the same shader source files in
    // multiple Effects in the same scene will work. It wouldn't if all those
    // Effects reused the same QSSGRhiShaderPipeline (i.e. if the only cache
    // key was the m_shaderPathKey).
    QSSGEffectSceneCacheKey cacheKey;
    cacheKey.m_shaderPathKey = inCmd->m_shaderPathKey;
    cacheKey.m_cmd = quintptr(inCmd);
    cacheKey.m_ubufIndex = m_currentUbufIndex;
    cacheKey.updateHashCode();

    // look for a runtime pipeline
    const auto it = m_shaderPipelines.constFind(cacheKey);
    if (it != m_shaderPipelines.cend())
        m_currentShaderPipeline = (*it).get();

    QByteArray qsbcKey;
    QSSGShaderFeatures features;
    if (!m_currentShaderPipeline) { // don't spend time if already got the pipeline
        features = shaderLib->getShaderMetaData(inCmd->m_shaderPathKey, QSSGShaderCache::ShaderType::Fragment).features;
        qsbcKey = QQsbCollection::EntryDesc::generateSha(inCmd->m_shaderPathKey, QQsbCollection::toFeatureSet(features));
    }

    // Check if there's a build-time generated entry for this effect
    if (!m_currentShaderPipeline && !shaderLib->m_preGeneratedShaderEntries.isEmpty()) {
        const QQsbCollection::EntryMap &pregenEntries = shaderLib->m_preGeneratedShaderEntries;
        const auto foundIt = pregenEntries.constFind(QQsbCollection::Entry(qsbcKey));
        if (foundIt != pregenEntries.cend()) {
            // The result here is always a new QSSGRhiShaderPipeline, which
            // fulfills the requirements of our local cache (cmd/ubufIndex in
            // cacheKey, not needed here since the result is a new object).
            const auto &shader = shaderCache->newPipelineFromPregenerated(inCmd->m_shaderPathKey,
                                                                          features,
                                                                          *foundIt,
                                                                          *inEffect,
                                                                          QSSGRhiShaderPipeline::UsedWithoutIa);
            m_shaderPipelines.insert(cacheKey, shader);
            m_currentShaderPipeline = shader.get();
        }
    }

    if (!m_currentShaderPipeline) {
        // Try the persistent (disk-based) cache then. The result here is
        // always a new QSSGRhiShaderPipeline, which fulfills the requirements
        // of our local cache (cmd/ubufIndex in cacheKey, not needed here since
        // the result is a new object). Alternatively, the result may be null
        // if there was no hit.
        const auto &shaderPipeline = shaderCache->tryNewPipelineFromPersistentCache(qsbcKey,
                                                                                    inCmd->m_shaderPathKey,
                                                                                    features,
                                                                                    QSSGRhiShaderPipeline::UsedWithoutIa);
        if (shaderPipeline) {
            m_shaderPipelines.insert(cacheKey, shaderPipeline);
            m_currentShaderPipeline = shaderPipeline.get();
        }
    }

    if (!m_currentShaderPipeline) {
        // Final option, generate the shader pipeline
        Q_TRACE_SCOPE(QSSG_generateShader);
        Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DGenerateShader);
        const auto &generator = m_sgContext->shaderProgramGenerator();
        if (auto stages = buildShaderForEffect(*inCmd, *generator, *shaderLib, *shaderCache, rhi->isYUpInFramebuffer())) {
            m_shaderPipelines.insert(cacheKey, stages);
            m_currentShaderPipeline = stages.get();
        }
        Q_QUICK3D_PROFILE_END_WITH_ID(QQuick3DProfiler::Quick3DGenerateShader, 0, inEffect->profilingId);
    }

    const auto &rhiContext = m_sgContext->rhiContext();

    if (m_currentShaderPipeline) {
        const void *cacheKey1 = reinterpret_cast<const void *>(this);
        const void *cacheKey2 = reinterpret_cast<const void *>(qintptr(m_currentUbufIndex));
        QSSGRhiDrawCallData &dcd = rhiContext->drawCallData({ cacheKey1, cacheKey2, nullptr, 0 });
        m_currentShaderPipeline->ensureCombinedMainLightsUniformBuffer(&dcd.ubuf);
        m_currentUBufData = dcd.ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
    } else {
        m_currentUBufData = nullptr;
    }

    rhiContext->stats().registerEffectShaderGenerationTime(timer.elapsed());
}

void QSSGRhiEffectSystem::renderCmd(QSSGRhiEffectTexture *inTexture, QSSGRhiEffectTexture *target)
{
    if (!m_currentShaderPipeline)
        return;

    if (!target) {
        qWarning("No effect render target?");
        return;
    }

    addTextureToShaderPipeline(QByteArrayLiteral("qt_inputTexture"), inTexture->texture, inTexture->desc);

    const auto &rhiContext = m_sgContext->rhiContext();
    const auto &renderer = m_sgContext->renderer();

    QRhiCommandBuffer *cb = rhiContext->commandBuffer();
    cb->debugMarkBegin(QByteArrayLiteral("Post-processing effect"));
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DRenderPass);

    for (QRhiTextureRenderTarget *rt : m_pendingClears) {
        // Effects like motion blur use an accumulator texture that should
        // start out empty (and they are sampled in the first pass), so such
        // textures need an explicit clear. It is not applicable for the common
        // case of outputting into a texture because that will get a clear
        // anyway when rendering the quad.
        if (rt != target->renderTarget) {
            cb->beginPass(rt, Qt::transparent, { 1.0f, 0 }, nullptr, QSSGRhiContext::commonPassFlags());
            QSSGRHICTX_STAT(rhiContext, beginRenderPass(rt));
            cb->endPass();
            QSSGRHICTX_STAT(rhiContext, endRenderPass());
        }
    }
    m_pendingClears.clear();

    const QSize inputSize = inTexture->texture->pixelSize();
    const QSize outputSize = target->texture->pixelSize();
    addCommonEffectUniforms(inputSize, outputSize);

    const void *cacheKey1 = reinterpret_cast<const void *>(this);
    const void *cacheKey2 = reinterpret_cast<const void *>(qintptr(m_currentUbufIndex));
    QSSGRhiDrawCallData &dcd = rhiContext->drawCallData({ cacheKey1, cacheKey2, nullptr, 0 });
    dcd.ubuf->endFullDynamicBufferUpdateForCurrentFrame();
    m_currentUBufData = nullptr;

    QRhiResourceUpdateBatch *rub = rhiContext->rhi()->nextResourceUpdateBatch();
    renderer->rhiQuadRenderer()->prepareQuad(rhiContext.get(), rub);

    // do resource bindings
    const QRhiShaderResourceBinding::StageFlags VISIBILITY_ALL =
            QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;
    QSSGRhiShaderResourceBindingList bindings;
    for (const QSSGRhiTexture &rhiTex : m_currentTextures) {
        int binding = m_currentShaderPipeline->bindingForTexture(rhiTex.name);
        if (binding < 0) // may not be used in the shader (think qt_inputTexture, it's not given a shader samples INPUT)
            continue;
        qCDebug(lcEffectSystem) << "    -> texture binding" << binding << "for" << rhiTex.name;
        // Make sure to bind all samplers even if the texture is missing, otherwise we can get crash on some graphics APIs
        QRhiTexture *texture = rhiTex.texture ? rhiTex.texture : rhiContext->dummyTexture({}, rub);
        bindings.addTexture(binding,
                            QRhiShaderResourceBinding::FragmentStage,
                            texture,
                            rhiContext->sampler(rhiTex.samplerDesc));
    }
    bindings.addUniformBuffer(0, VISIBILITY_ALL, dcd.ubuf);
    QRhiShaderResourceBindings *srb = rhiContext->srb(bindings);

    QSSGRhiGraphicsPipelineState ps;
    ps.viewport = QRhiViewport(0, 0, float(outputSize.width()), float(outputSize.height()));
    ps.shaderPipeline = m_currentShaderPipeline;

    renderer->rhiQuadRenderer()->recordRenderQuadPass(rhiContext.get(), &ps, srb, target->renderTarget, QSSGRhiQuadRenderer::UvCoords);
    m_currentUbufIndex++;
    cb->debugMarkEnd();
    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DRenderPass, 0, QByteArrayLiteral("post_processing_effect"));
}

void QSSGRhiEffectSystem::addCommonEffectUniforms(const QSize &inputSize, const QSize &outputSize)
{
    const auto &rhiContext = m_sgContext->rhiContext();
    QRhi *rhi = rhiContext->rhi();

    QMatrix4x4 mvp;
    if (rhi->isYUpInFramebuffer() != rhi->isYUpInNDC())
        mvp.data()[5] = -1.0f;
    m_currentShaderPipeline->setUniformValue(m_currentUBufData, "qt_modelViewProjection", mvp, QSSGRenderShaderValue::Matrix4x4);

    QVector2D size(inputSize.width(), inputSize.height());
    m_currentShaderPipeline->setUniformValue(m_currentUBufData, "qt_inputSize", size, QSSGRenderShaderValue::Vec2);

    size = QVector2D(outputSize.width(), outputSize.height());
    m_currentShaderPipeline->setUniformValue(m_currentUBufData, "qt_outputSize", size, QSSGRenderShaderValue::Vec2);

    float fc = float(m_sgContext->frameCount());
    m_currentShaderPipeline->setUniformValue(m_currentUBufData, "qt_frame_num", fc, QSSGRenderShaderValue::Float);

    // Bames and values for uniforms that are also used by default and/or
    // custom materials must always match, effects must not deviate.

    m_currentShaderPipeline->setUniformValue(m_currentUBufData, "qt_cameraProperties", m_cameraClipRange, QSSGRenderShaderValue::Vec2);

    float vp = rhi->isYUpInFramebuffer() ? 1.0f : -1.0f;
    m_currentShaderPipeline->setUniformValue(m_currentUBufData, "qt_normalAdjustViewportFactor", vp, QSSGRenderShaderValue::Float);

    const float nearClip = rhi->isClipDepthZeroToOne() ? 0.0f : -1.0f;
    m_currentShaderPipeline->setUniformValue(m_currentUBufData, "qt_nearClipValue", nearClip, QSSGRenderShaderValue::Float);

    if (m_depthTexture) {
        static const QSSGRhiSamplerDescription depthSamplerDesc {
                    QRhiSampler::Nearest, QRhiSampler::Nearest,
                    QRhiSampler::None,
                    QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat
        };
        addTextureToShaderPipeline("qt_depthTexture", m_depthTexture, depthSamplerDesc);
    }
}

void QSSGRhiEffectSystem::addTextureToShaderPipeline(const QByteArray &name,
                                                     QRhiTexture *texture,
                                                     const QSSGRhiSamplerDescription &samplerDescription)
{
    if (!m_currentShaderPipeline)
        return;

    static const QSSGRhiSamplerDescription defaultDescription { QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                                QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat };
    bool validDescription = samplerDescription.magFilter != QRhiSampler::None;

    // This is a map for a reason: there can be multiple calls to this function
    // for the same 'name', with a different 'texture', take the last value
    // into account only.
    m_currentTextures.insert(name, { name, texture, validDescription ? samplerDescription : defaultDescription});
}

QT_END_NAMESPACE
