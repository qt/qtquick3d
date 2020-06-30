/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtQuick3DRuntimeRender/private/qssgrhieffectsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhiquadrenderer_p.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcEffectSystem);
Q_LOGGING_CATEGORY(lcEffectSystem, "qt.quick3d.effects");

struct QSSGRhiEffectTexture
{
    QRhiTexture *texture;
    QRhiRenderPassDescriptor *renderPassDescriptor;
    QRhiTextureRenderTarget *renderTarget;
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

QSSGRhiEffectSystem::QSSGRhiEffectSystem(const QSSGRef<QSSGRenderContextInterface> &sgContext)
    : m_sgContext(sgContext.data())
{
}

QSSGRhiEffectSystem::~QSSGRhiEffectSystem()
{
    releaseResources();
}

void QSSGRhiEffectSystem::setup(QRhi *, QSize outputSize, QSSGRenderEffect *firstEffect)
{
    if (!firstEffect || outputSize.isEmpty()) {
        releaseResources();
        return;
    }
    m_outSize = outputSize;
    m_firstEffect = firstEffect;
}

QSSGRhiEffectTexture *QSSGRhiEffectSystem::findTexture(const QByteArray &bufferName)
{
    auto findTexture = [bufferName](const QSSGRhiEffectTexture *rt){ return rt->name == bufferName; };
    const auto foundIt = std::find_if(m_textures.cbegin(), m_textures.cend(), findTexture);
    QSSGRhiEffectTexture *result = foundIt == m_textures.cend() ? nullptr : *foundIt;
    return result;
}

QSSGRhiEffectTexture *QSSGRhiEffectSystem::getTexture(const QByteArray &bufferName, const QSize &size, QRhiTexture::Format format)
{
    QSSGRhiEffectTexture *result = findTexture(bufferName);

    // If not found, look for an unused texture
    if (!result) {
        //TODO: try to find a texture with the right size/format first
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

    QRhi *rhi = m_rhiContext->rhi();
    bool needsRebuild = result->texture && (result->texture->pixelSize() != size || result->texture->format() != format);

    if (!result->texture) {
        result->texture = rhi->newTexture(format, size, 1, QRhiTexture::RenderTarget);
        result->texture->create();
    } else if (needsRebuild) {
        result->texture->setPixelSize(size);
        result->texture->setFormat(format);
        result->texture->create();
    }

    if (!result->renderTarget) {
        result->renderTarget = rhi->newTextureRenderTarget({ result->texture });
        result->renderPassDescriptor = result->renderTarget->newCompatibleRenderPassDescriptor();
        result->renderTarget->setRenderPassDescriptor(result->renderPassDescriptor);
        result->renderTarget->create();
    } else if (needsRebuild) {
        result->renderTarget->create();
    }

    result->name = bufferName;
    return result;
}

void QSSGRhiEffectSystem::releaseTexture(QSSGRhiEffectTexture *texture)
{
    if (!texture->flags.isSceneLifetime())
        texture->name = {};
}

void QSSGRhiEffectSystem::releaseTextures()
{
    for (auto *t : qAsConst(m_textures))
        releaseTexture(t);
}

QRhiTexture *QSSGRhiEffectSystem::process(const QSSGRef<QSSGRhiContext> &rhiCtx,
                                          const QSSGRef<QSSGRenderer> &renderer,
                                          QRhiTexture *inTexture,
                                          QRhiTexture *inDepthTexture,
                                          QVector2D cameraClipRange)
{
    m_rhiContext = rhiCtx;
    m_renderer = renderer.data(); // TODO: This looks interesting
    if (!m_rhiContext || !m_renderer)
        return inTexture;
    m_depthTexture = inDepthTexture;
    m_cameraClipRange = cameraClipRange;
    Q_ASSERT(m_firstEffect);

    //### For now, ignore active state
    m_currentUbufIndex = 0;
    auto *currentEffect = m_firstEffect;
    QSSGRhiEffectTexture firstTex{ inTexture, nullptr, nullptr, {}, {}, {} };
    auto *latestOutput = doRenderEffect(currentEffect, &firstTex);
    firstTex.texture = nullptr; // make sure we don't delete inTexture when we go out of scope

    while ((currentEffect = currentEffect->m_nextEffect)) {
        auto *effectOut = doRenderEffect(currentEffect, latestOutput);
        releaseTexture(latestOutput);
        latestOutput = effectOut;
    }

    releaseTextures(); //we're done here -- TODO: unify texture handling with AA
    return latestOutput->texture;
}

void QSSGRhiEffectSystem::releaseResources()
{
    for (auto *t : qAsConst(m_textures))
        m_rhiContext->invalidateCachedReferences(t->renderPassDescriptor);
    qDeleteAll(m_textures);
    m_textures.clear();
    m_currentOutput = nullptr;

    m_stages.clear();
}

QSSGRhiEffectTexture *QSSGRhiEffectSystem::doRenderEffect(const QSSGRenderEffect *inEffect,
                                            QSSGRhiEffectTexture *inTexture)
{
    // Run through the effect commands and render the effect.
    const auto &theCommands = inEffect->commands;
    qCDebug(lcEffectSystem) << "START effect " << inEffect->className;
    QSSGRhiEffectTexture *finalOutputTexture = nullptr;
    QSSGRhiEffectTexture *currentOutput = nullptr;
    QSSGRhiEffectTexture *currentInput = inTexture;
    for (const auto &theCommand : theCommands) {
        qCDebug(lcEffectSystem).noquote() << "    >" << theCommand->typeAsString() << "--" << theCommand->debugString();

        switch (theCommand->m_type) {
        case CommandType::AllocateBuffer:
            allocateBufferCmd(static_cast<QSSGAllocateBuffer *>(theCommand), inTexture);
            break;

        case CommandType::ApplyBufferValue: {
            auto *applyCommand = static_cast<QSSGApplyBufferValue *>(theCommand);
            // "If no buffer name is given then the special buffer [source] is assumed."
            auto *buffer = applyCommand->m_bufferName.isEmpty() ? inTexture : findTexture(applyCommand->m_bufferName);
            if (applyCommand->m_paramName.isEmpty()) {
                currentInput = buffer;
            } else {
                const bool needsAlphaMultiply = true; //??? directGL uses theTextureToBind.needsAlphaMultiply, which seems to always be false
                addTextureToShaderStages(applyCommand->m_paramName, buffer->texture, buffer->desc, needsAlphaMultiply);
            }
            break;
        }

        case CommandType::ApplyDepthValue: {
            auto *depthCommand = static_cast<QSSGApplyDepthValue *>(theCommand);
            static const QSSGRhiSamplerDescription depthDescription{ QRhiSampler::Nearest, QRhiSampler::Nearest,
                                                                     QRhiSampler::None,
                                                                     QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge };
            addTextureToShaderStages(depthCommand->m_paramName, m_depthTexture, depthDescription, false);
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
            if (!bindCmd->m_needsClear)
                qWarning("##### BindBuffer without needsClear not supported");
            currentOutput = findTexture(bindCmd->m_bufferName);
            break;
        }

        case CommandType::BindShader:
            bindShaderCmd(static_cast<QSSGBindShader *>(theCommand), inEffect);
            break;

        case CommandType::BindTarget: {
            auto targetCmd = static_cast<QSSGBindTarget*>(theCommand);
            // The command can define a format. If not, fall back to the effect's output format.
            // If the effect doesn't define an output format, use the same format as the input texture
            // ??? The direct GL path does not use the command's format: what's up with that?
            // (this is fairly pointless anyway, since it's always Unknown for the predefined effects)

            auto f = targetCmd->m_outputFormat == QSSGRenderTextureFormat::Unknown ?
                        inEffect->outputFormat : targetCmd->m_outputFormat.format;
            qCDebug(lcEffectSystem) << "      Target format" << toString(f);
            QRhiTexture::Format rhiFormat = f == QSSGRenderTextureFormat::Unknown ?
                        currentInput->texture->format() : QSSGBufferManager::toRhiFormat(f);
            // Make sure we use different names for each effect inside one frame
            QByteArray tmpName = QByteArrayLiteral("__output_").append(QByteArray::number(m_currentUbufIndex));
            currentOutput = getTexture(tmpName, m_outSize, rhiFormat);
            finalOutputTexture = currentOutput;
            break;
        }

        case CommandType::Render:
            renderCmd(currentInput, currentOutput);
            currentInput = inTexture; // default input for each new pass is defined to be original input
            break;

            // TODO: commands not implemented:

            //        case CommandType::AllocateDataBuffer:
            //            static_cast<QSSGAllocateDataBuffer *>(theCommand);
            //            break;
            //        case CommandType::ApplyBlending:
            //            static_cast<QSSGApplyBlending *>(theCommand);
            //            break;
            //        case CommandType::ApplyBlitFramebuffer:
            //            static_cast<QSSGApplyBlitFramebuffer *>(theCommand);
            //            break;
            //        case CommandType::ApplyCullMode:
            //            static_cast<QSSGApplyCullMode *>(theCommand);
            //            break;
            //        case CommandType::ApplyDataBufferValue:
            //            static_cast<QSSGApplyDataBufferValue *>(theCommand);
            //            break;
            //        case CommandType::ApplyImageValue:
            //            static_cast<QSSGApplyImageValue *>(theCommand);
            //            break;
            //        case CommandType::ApplyRenderState:
            //            static_cast<QSSGApplyRenderState *>(theCommand);
            //            break;
            //        case CommandType::DepthStencil:
            //            static_cast<QSSGDepthStencil *>(theCommand);
            //            break;

        default:
            qDebug() << "####" << theCommand->typeAsString() << "command not implemented";
            break;
        }
    }
    // TODO: release textures used by this effect now, instead of after processing all the effects
    qCDebug(lcEffectSystem) << "END effect " << inEffect->className;
    return finalOutputTexture;
}

void QSSGRhiEffectSystem::allocateBufferCmd(const QSSGAllocateBuffer *inCmd, QSSGRhiEffectTexture *inTexture)
{
    // Note: Allocate is used both to allocate new, and refer to buffer created earlier
    QSize bufferSize(m_outSize * qreal(inCmd->m_sizeMultiplier));

    QSSGRenderTextureFormat f = inCmd->m_format;
    QRhiTexture::Format rhiFormat = (f == QSSGRenderTextureFormat::Unknown) ? inTexture->texture->format()
                                                                            : QSSGBufferManager::toRhiFormat(f);

    QSSGRhiEffectTexture *buf = getTexture(inCmd->m_name, bufferSize, rhiFormat);
    auto filter = toRhi(inCmd->m_filterOp);
    auto tiling = toRhi(inCmd->m_texCoordOp);
    buf->desc = { filter, filter, QRhiSampler::None, tiling, tiling };
    buf->flags = inCmd->m_bufferFlags;
}

void QSSGRhiEffectSystem::applyInstanceValueCmd(const QSSGApplyInstanceValue *inCmd, const QSSGRenderEffect *inEffect)
{
    if (!m_stages)
        return;

    const bool setAll = inCmd->m_propertyName.isEmpty();
    for (const QSSGRenderEffect::Property &property : qAsConst(inEffect->properties)) {
        if (setAll || property.name == inCmd->m_propertyName) {
            m_stages->setUniformValue(property.name, property.value, property.shaderDataType);
            //qCDebug(lcEffectSystem) << "setUniformValue" << property.name << toString(property.shaderDataType) << "to" << property.value;
        }
    }
    for (const QSSGRenderEffect::TextureProperty &textureProperty : qAsConst(inEffect->textureProperties)) {
        if (setAll || textureProperty.name == inCmd->m_propertyName) {
            QSSGRenderImage *image = textureProperty.texImage;
            if (image) {
                const auto &imageSource = image->m_imagePath;
                const QSSGRef<QSSGBufferManager> &theBufferManager(m_renderer->contextInterface()->bufferManager());
                if (!imageSource.isEmpty()) {
                    QSSGRenderImageTextureData theTextureData = theBufferManager->loadRenderImage(imageSource);
                    if (theTextureData.m_rhiTexture) {
                        const QSSGRhiSamplerDescription desc{
                            toRhi(textureProperty.minFilterType),
                            toRhi(textureProperty.magFilterType),
                            theTextureData.m_mipmaps > 0 ? QRhiSampler::Linear : QRhiSampler::None,
                            toRhi(textureProperty.clampType),
                            toRhi(textureProperty.clampType)
                        };
                        const bool needsAlphaMultiply = true; //??? this is what directGL does, but should it depend on image format?
                        addTextureToShaderStages(textureProperty.name, theTextureData.m_rhiTexture, desc, needsAlphaMultiply);
                    }
                }
            }
        }
    }
}

void QSSGRhiEffectSystem::applyValueCmd(const QSSGApplyValue *inCmd, const QSSGRenderEffect *inEffect)
{
    if (!m_stages)
        return;

    const auto &properties = inEffect->properties;
    const auto foundIt = std::find_if(properties.cbegin(), properties.cend(), [inCmd](const QSSGRenderEffect::Property &prop) {
        return (prop.name == inCmd->m_propertyName);
    });

    if (foundIt != properties.cend())
        m_stages->setUniformValue(inCmd->m_propertyName, inCmd->m_value, foundIt->shaderDataType);
    else
        qWarning() << " #### Could not find property" << inCmd->m_propertyName;
}

void QSSGRhiEffectSystem::bindShaderCmd(const QSSGBindShader *inCmd, const QSSGRenderEffect *inEffect)
{
    QByteArray shaderCode = m_renderer->contextInterface()->shaderLibraryManager()->getShaderSource(inCmd->m_shaderPathKey);

    // Clumsy, since we have all the info we need, but the expanded functions must be placed after
    // the effects.glsl include but before they are used in frag()/vert(), so...
    QSSGShaderLibraryManager::insertSnapperDirectives(shaderCode);

    const QSSGRef<QSSGProgramGenerator> &generator = m_renderer->contextInterface()->shaderProgramGenerator();
    generator->beginProgram();

    QSSGStageGeneratorBase *vStage = generator->getStage(QSSGShaderGeneratorStage::Vertex);
    Q_ASSERT(vStage);
    vStage->append(QByteArrayLiteral("#define VERTEX_SHADER\n"));
    vStage->append(shaderCode);

    QSSGStageGeneratorBase *fStage = generator->getStage(QSSGShaderGeneratorStage::Fragment);
    Q_ASSERT(fStage);
    for (const QSSGRenderEffect::Property &property : qAsConst(inEffect->properties))
        fStage->addUniform(property.name, property.typeName);
    fStage->addUniform(QByteArrayLiteral("Texture0"), QByteArrayLiteral("sampler2D"));
    fStage->addUniform(QByteArrayLiteral("Texture0Info"), QByteArrayLiteral("vec4"));
    for (const QSSGRenderEffect::TextureProperty &property : qAsConst(inEffect->textureProperties)) {
        fStage->addUniform(property.name, QByteArrayLiteral("sampler2D"));
        fStage->addUniform(property.name + QByteArrayLiteral("Info"), QByteArrayLiteral("vec4"));
    }

    fStage->append(QByteArrayLiteral("#define FRAGMENT_SHADER\n"
                                     "#define gl_FragColor fragOutput\n\n"));
    fStage->append(shaderCode);

    QSSGRef<QSSGRhiShaderStages> stages;
    stages = generator->compileGeneratedRhiShader(inCmd->m_shaderPathKey, ShaderFeatureSetList());
    if (stages.isNull())
        m_stages.clear(); // Compilation failed, warning will already have been produced
    else
        m_stages = QSSGRhiShaderStagesWithResources::fromShaderStages(stages);
}

void QSSGRhiEffectSystem::renderCmd(QSSGRhiEffectTexture *inTexture, QSSGRhiEffectTexture *target)
{
    if (!m_stages)
        return;
    if (!target) {
        qWarning("No effect render target?");
        return;
    }

    QRhiCommandBuffer *cb = m_rhiContext->commandBuffer();
    cb->debugMarkBegin(QByteArrayLiteral("Post-processing effect"));

    QSize outputSize{target->texture->pixelSize()};
    addCommonEffectUniforms(outputSize);
    const bool needsAlphaMultiply = false; //###???
    addTextureToShaderStages(QByteArrayLiteral("Texture0"), inTexture->texture, inTexture->desc,needsAlphaMultiply);

    // bake uniform buffer
    QRhiResourceUpdateBatch *rub = m_rhiContext->rhi()->nextResourceUpdateBatch();
    const void *cacheKey1 = reinterpret_cast<const void *>(this);
    const void *cacheKey2 = reinterpret_cast<const void *>(qintptr(m_currentUbufIndex));
    auto &ubs = m_rhiContext->uniformBufferSet({ cacheKey1, cacheKey2, nullptr, QSSGRhiUniformBufferSetKey::Effects });
    m_stages->bakeMainUniformBuffer(&ubs.ubuf, rub);
    m_renderer->rhiQuadRenderer()->prepareQuad(m_rhiContext.data(), rub);

    // do resource bindings
    const QRhiShaderResourceBinding::StageFlags VISIBILITY_ALL =
            QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;
    QSSGRhiContext::ShaderResourceBindingList bindings;
    for (int i = 0; i < m_stages->extraTextureCount(); i++) {
        const QSSGRhiTexture &rhiTex = m_stages->extraTextureAt(i);
        int binding = m_stages->bindingForTexture(rhiTex.name);
        qCDebug(lcEffectSystem) << "    -> texture binding" << binding << "for" << rhiTex.name;
        bindings.append(QRhiShaderResourceBinding::sampledTexture(binding, QRhiShaderResourceBinding::FragmentStage,
                                                                  rhiTex.texture, m_rhiContext->sampler(rhiTex.samplerDesc)));
    }
    bindings.append(QRhiShaderResourceBinding::uniformBuffer(0, VISIBILITY_ALL, ubs.ubuf));
    QRhiShaderResourceBindings *srb = m_rhiContext->srb(bindings);

    QSSGRhiGraphicsPipelineState ps;
    ps.viewport = QRhiViewport(0, 0, float(outputSize.width()), float(outputSize.height()));
    ps.shaderStages = m_stages->stages();

    m_renderer->rhiQuadRenderer()->recordRenderQuadPass(m_rhiContext.data(), &ps, srb, target->renderTarget, QSSGRhiQuadRenderer::UvCoords);
    m_currentUbufIndex++;
    cb->debugMarkEnd();
}

void QSSGRhiEffectSystem::addCommonEffectUniforms(const QSize &outputSize)
{
    // Common effect uniform values
    QVector2D destSize(outputSize.width(), outputSize.height());
    QVector2D colorAlpha(1, 0);
    float yScale = (m_rhiContext->rhi()->isYUpInFramebuffer() != m_rhiContext->rhi()->isYUpInNDC()) ? -2.0f : 2.0f;
    QMatrix4x4 mvp;
    mvp.scale(2.0f / outputSize.width(), yScale / outputSize.height());
    float fc = float(m_sgContext->frameCount());
    float fps = float(m_sgContext->getFPS().first);

    // Built-ins from effect.glsllib
    m_stages->setUniformValue(QByteArrayLiteral("DestSize"), destSize, QSSGRenderShaderDataType::Vec2);
    m_stages->setUniformValue(QByteArrayLiteral("FragColorAlphaSettings"), colorAlpha, QSSGRenderShaderDataType::Vec2);
    m_stages->setUniformValue(QByteArrayLiteral("ModelViewProjectionMatrix"), mvp, QSSGRenderShaderDataType::Matrix4x4);
    m_stages->setUniformValue(QByteArrayLiteral("AppFrame"), fc, QSSGRenderShaderDataType::Float);
    m_stages->setUniformValue(QByteArrayLiteral("FPS"), fps, QSSGRenderShaderDataType::Float);
    m_stages->setUniformValue(QByteArrayLiteral("CameraClipRange"), m_cameraClipRange, QSSGRenderShaderDataType::Vec2);
}

void QSSGRhiEffectSystem::addTextureToShaderStages(const QByteArray &name, QRhiTexture *texture, const QSSGRhiSamplerDescription &samplerDescription, bool needsAlphaMultiply)
{
    if (!m_stages)
        return;
    if (!texture) {
        qWarning("Texture for '%s' cannot be null", name.constData());
        return;
    }

    //set texture-info uniform
    const float theMixValue = needsAlphaMultiply ? 0.0f : 1.0f;
    const QSize texSize = texture->pixelSize();
    QVector4D texInfo(texSize.width(), texSize.height(), theMixValue, 0);
    m_stages->setUniformValue(name + QByteArrayLiteral("Info"), texInfo, QSSGRenderShaderDataType::Vec4);

    //add texture to extraTextures
    static const QSSGRhiSamplerDescription defaultDescription{ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                                        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge };
    bool validDescription = samplerDescription.magFilter != QRhiSampler::None;
    m_stages->addExtraTexture({ name, texture, validDescription ? samplerDescription : defaultDescription});
}

QT_END_NAMESPACE
