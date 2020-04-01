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
#include <QtQuick3DRuntimeRender/private/qssgrendererimpl_p.h>

QT_BEGIN_NAMESPACE

using namespace dynamic;

// TODO:
// Q_DECLARE_LOGGING_CATEGORY(lcEffectSystem);
// Q_LOGGING_CATEGORY(lcSharedImage, "qt.quick3d.effects");

// TODO: use QSSGRhiRenderableTexture

struct QSSGRhiEffectTexture
{
    QRhiTexture *texture;
    QRhiRenderPassDescriptor *renderPassDescriptor;
    QRhiTextureRenderTarget *renderTarget;
    QByteArray name;

    QSSGRhiSamplerDescription desc;

    ~QSSGRhiEffectTexture()
    {
        delete texture;
        delete renderPassDescriptor;
        delete renderTarget;
    }
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
    //TODO: try to find a texture with the right size/format first
    auto findUnused = [](const QSSGRhiEffectTexture *rt){ return rt->name.isEmpty(); };
    const auto unusedIt = std::find_if(m_textures.cbegin(), m_textures.cend(), findUnused);
    QSSGRhiEffectTexture *result = unusedIt == m_textures.cend() ? nullptr : *unusedIt;

    if (!result) {
        result = new QSSGRhiEffectTexture{};
        m_textures.append(result);
    } else {
        result->desc = {};
    }

    QRhi *rhi = m_rhiContext->rhi();
    bool needsRebuild = result->texture && (result->texture->pixelSize() != size || result->texture->format() != format);

    if (!result->texture) {
        result->texture = rhi->newTexture(format, size, 1, QRhiTexture::RenderTarget);
        result->texture->build();
    } else if (needsRebuild) {
        result->texture->setPixelSize(size);
        result->texture->setFormat(format);
        result->texture->build();
    }

    if (!result->renderTarget) {
        result->renderTarget = rhi->newTextureRenderTarget({ result->texture });
        result->renderPassDescriptor = result->renderTarget->newCompatibleRenderPassDescriptor();
        result->renderTarget->setRenderPassDescriptor(result->renderPassDescriptor);
        result->renderTarget->build();
    } else if (needsRebuild) {
        result->renderTarget->build();
    }

    result->name = bufferName;
    return result;
}

void QSSGRhiEffectSystem::releaseTexture(QSSGRhiEffectTexture *texture)
{
    texture->name = {};
}

void QSSGRhiEffectSystem::releaseTextures()
{
    for (auto *t : qAsConst(m_textures))
        t->name = {};
}

QRhiTexture *QSSGRhiEffectSystem::process(const QSSGRef<QSSGRhiContext> &rhiCtx,
                                          const QSSGRef<QSSGRendererInterface> &rendererIf,
                                          QRhiTexture *inTexture,
                                          QRhiTexture *inDepthTexture,
                                          QVector2D cameraClipRange)
{
    m_rhiContext = rhiCtx;
    m_renderer = static_cast<QSSGRendererImpl *>(rendererIf.data());
    if (!m_rhiContext || !m_renderer)
        return inTexture;
    m_depthTexture = inDepthTexture;
    m_cameraClipRange = cameraClipRange;
    Q_ASSERT(m_firstEffect);

    //### For now, ignore active state
    m_currentUbufIndex = 0;
    auto *currentEffect = m_firstEffect;
    QSSGRhiEffectTexture firstTex{ inTexture, nullptr, nullptr, {}, {} };
    auto *latestOutput = doRenderEffect(currentEffect, &firstTex);
    firstTex = {}; //make sure we don't delete inTexture when we go out of scope

    while ((currentEffect = currentEffect->m_nextEffect)) {
        m_currentUbufIndex++;
        auto *effectOut = doRenderEffect(currentEffect, latestOutput);
        releaseTexture(latestOutput);
        latestOutput = effectOut;
    }

    //TODO: handle SceneLifeTime -- we probably need a hook in ~QSSGRenderEffect()
    releaseTextures(); //we're done here -- TODO: unify texture handling with AA
    return latestOutput->texture;
}

void QSSGRhiEffectSystem::releaseResources()
{
    qDeleteAll(m_textures);
    m_currentOutput = nullptr;

    m_stages.clear();
}

QSSGRhiEffectTexture *QSSGRhiEffectSystem::doRenderEffect(const QSSGRenderEffect *inEffect,
                                            QSSGRhiEffectTexture *inTexture)
{
    // Run through the effect commands and render the effect.
    const auto &theCommands = inEffect->commands;
    qDebug() << "START effect " << inEffect->className;
    QSSGRhiEffectTexture *finalOutputTexture = nullptr;
    QSSGRhiEffectTexture *currentOutput = nullptr;
    QSSGRhiEffectTexture *currentInput = inTexture;
    for (const auto &theCommand : theCommands) {
        qDebug().noquote() << "    >" << theCommand->typeAsString() << "--" << theCommand->debugString();

        switch (theCommand->m_type) {
        case CommandType::ApplyInstanceValue:
            if (m_stages)
                applyInstanceValueCmd(static_cast<QSSGApplyInstanceValue *>(theCommand), inEffect);
            break;

        case CommandType::BindShader:
            bindShaderCmd(static_cast<QSSGBindShader *>(theCommand), inEffect);
            break;

        case CommandType::Render:
            if (currentOutput)
                renderCmd(currentInput, currentOutput);
            else
                qWarning("NO OUTPUT FOR RENDER!!!!!"); // TODO: assert
            currentInput = inTexture; // default input for each new pass is defined to be original input
            break;

        case CommandType::BindBuffer: {
            auto *bindCmd = static_cast<QSSGBindBuffer *>(theCommand);
            if (!bindCmd->m_needsClear)
                qWarning("##### BindBuffer without needsClear not supported");

            currentOutput = findTexture(bindCmd->m_bufferName);
            break;
        }
        case CommandType::BindTarget: {
            auto targetCmd = static_cast<QSSGBindTarget*>(theCommand);
            // The command can define a format. If not, fall back to the effect's output format.
            // If the effect doesn't define an output format, use the same format as the input texture
            // ??? The direct GL path does not use the command's format: what's up with that?
            // (this is fairly pointless anyway, since it's always Unknown for the predefined effects)

            auto f = targetCmd->m_outputFormat == QSSGRenderTextureFormat::Unknown ?
                        inEffect->outputFormat : targetCmd->m_outputFormat.format;
            qDebug() << "      Target format" << toString(f);
            QRhiTexture::Format rhiFormat = f == QSSGRenderTextureFormat::Unknown ?
                        currentInput->texture->format() : QSSGBufferManager::toRhiFormat(f);
            currentOutput = getTexture("__output", m_outSize, rhiFormat);
            finalOutputTexture = currentOutput;
            break;
        }
        case CommandType::ApplyBufferValue: {
            auto *applyCommand = static_cast<QSSGApplyBufferValue *>(theCommand);
            // "If no buffer name is given then the special buffer [source] is assumed."
            auto *buffer = applyCommand->m_bufferName.isEmpty() ? inTexture : findTexture(applyCommand->m_bufferName);
            if (applyCommand->m_paramName.isEmpty()) {
                currentInput = buffer;
            } else {
                addTextureToShaderStages(applyCommand->m_paramName, buffer->texture, buffer->desc);
            }
            break;
        }
        case CommandType::AllocateBuffer: {
            // Note: Allocate is used both to allocate new, and refer to buffer created earlier
            auto *allocateCmd = static_cast<QSSGAllocateBuffer *>(theCommand);
            QSize bufferSize(m_outSize * allocateCmd->m_sizeMultiplier);

            QSSGRenderTextureFormat f = allocateCmd->m_format;
            QRhiTexture::Format rhiFormat = (f == QSSGRenderTextureFormat::Unknown) ? inTexture->texture->format() :
                                QSSGBufferManager::toRhiFormat(f);

            QSSGRhiEffectTexture *buf = nullptr;
            if (!(buf = findTexture(allocateCmd->m_name)))
                buf = getTexture(allocateCmd->m_name, bufferSize, rhiFormat);
            auto filter = toRhi(allocateCmd->m_filterOp);
            auto tiling = toRhi(allocateCmd->m_texCoordOp);
            buf->desc = {
                filter,
                filter,
                QRhiSampler::None,
                tiling,
                tiling
            };

            // TODO: flags sceneLifetime
            break;
        }
        case CommandType::ApplyDepthValue: {
            auto *depthCommand = static_cast<QSSGApplyDepthValue *>(theCommand);
            addTextureToShaderStages(depthCommand->m_paramName, m_depthTexture, {});
            break;
        }
        case CommandType::ApplyValue: {
            auto *applyCmd = static_cast<QSSGApplyValue *>(theCommand);
            const auto &properties = inEffect->properties;
            const auto foundIt = std::find_if(properties.cbegin(), properties.cend(), [applyCmd](const QSSGRenderEffect::Property &prop) {
                return (prop.name == applyCmd->m_propertyName);
            });

            if (foundIt != properties.cend())
                m_stages->setUniformValue(applyCmd->m_propertyName, applyCmd->m_value, foundIt->shaderDataType);
            else
                qWarning() << " #### Could not find property" << applyCmd->m_propertyName;
            break;
        }

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
    qDebug() << "END effect " << inEffect->className;
    return finalOutputTexture;
}

void QSSGRhiEffectSystem::applyInstanceValueCmd(const QSSGApplyInstanceValue *theCommand, const QSSGRenderEffect *inEffect)
{
    Q_ASSERT(m_stages);
    const bool setAll = theCommand->m_propertyName.isEmpty();
    for (const QSSGRenderEffect::Property &property : qAsConst(inEffect->properties)) {
        if (setAll || property.name == theCommand->m_propertyName) {
            m_stages->setUniformValue(property.name, property.value, property.shaderDataType);
            //qDebug() << "setUniformValue" << property.name << toString(property.shaderDataType) << "to" << property.value;
        }
    }
    for (const QSSGRenderEffect::TextureProperty &textureProperty : qAsConst(inEffect->textureProperties)) {
        if (setAll || textureProperty.name == theCommand->m_propertyName) {
            QSSGRenderImage *image = textureProperty.texImage;
            if (image) {
                const QString &imageSource = image->m_imagePath;
                const QSSGRef<QSSGBufferManager> &theBufferManager(m_renderer->contextInterface()->bufferManager());
                QSSGRef<QSSGRenderTexture2D> theTexture;
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
                        addTextureToShaderStages(textureProperty.name, theTextureData.m_rhiTexture, desc);
                    }
                }
            }
        }
    }
}

void QSSGRhiEffectSystem::bindShaderCmd(const QSSGBindShader *theCommand,
                                        const QSSGRenderEffect *inEffect)
{
    //#tbd: check caching here, i.e. how much work per non-initial frame
    QByteArray shaderCode = m_renderer->contextInterface()->dynamicObjectSystem()->doLoadShader(theCommand->m_shaderPath);

    // Clumsy, since we have all the info we need, but the expanded functions must be placed after
    // the effects.glsl include but before they are used in frag()/vert(), so...
    QSSGDynamicObjectSystem::insertSnapperDirectives(shaderCode, true);

    const QSSGRef<QSSGShaderProgramGeneratorInterface> &generator = m_renderer->contextInterface()->shaderProgramGenerator();
    generator->beginProgram();
    //#TBD shell out a function instead of duplicating the following lines for vertex and fragment shaders
    QSSGShaderStageGeneratorInterface *vStage = generator->getStage(QSSGShaderGeneratorStage::Vertex);
    Q_ASSERT(vStage);
    vStage->append(QByteArrayLiteral("#define VERTEX_SHADER\n"));
    vStage->append(shaderCode);
    QSSGShaderStageGeneratorInterface *fStage = generator->getStage(QSSGShaderGeneratorStage::Fragment);
    Q_ASSERT(fStage);
    for (const QSSGRenderEffect::Property &property : qAsConst(inEffect->properties)) {
        //###vStage->addUniform(property.name, property.typeName);
        fStage->addUniform(property.name, property.typeName);
    }
    if (true) { //# unless input is buffer?
        fStage->addUniform(QByteArrayLiteral("Texture0"), QByteArrayLiteral("sampler2D"));
        fStage->addUniform(QByteArrayLiteral("Texture0Info"), QByteArrayLiteral("vec4"));
    }
    for (const QSSGRenderEffect::TextureProperty &property : qAsConst(inEffect->textureProperties)) {
        fStage->addUniform(property.name, QByteArrayLiteral("sampler2D"));
        fStage->addUniform(property.name + QByteArrayLiteral("Info"), QByteArrayLiteral("vec4"));
    }

    fStage->append(QByteArrayLiteral("#define FRAGMENT_SHADER\n"
                                     "#define gl_FragColor fragOutput\n\n"));
    fStage->append(shaderCode);

    ShaderFeatureSetList features;
    features.push_back(QSSGShaderPreprocessorFeature(QSSGShaderDefines::asString(QSSGShaderDefines::Rhi), true));

    QSSGRef<QSSGRhiShaderStages> stages;
    stages = generator->compileGeneratedRhiShader(theCommand->m_shaderPath, QSSGShaderCacheProgramFlags(), features);
    if (!stages.isNull())
        m_stages = QSSGRhiShaderStagesWithResources::fromShaderStages(stages);
}

void QSSGRhiEffectSystem::renderCmd(QSSGRhiEffectTexture *inTexture, QSSGRhiEffectTexture *target)
{
    if (!m_stages) {
        qWarning("No post-processing shader set");
        return;
    }

    QRhiCommandBuffer *cb = m_rhiContext->commandBuffer();
    cb->debugMarkBegin(QByteArrayLiteral("Post-processing effect"));

    // Effect Common uniform values  ##For now, hardcoded or computed here
    QSize outputSize{target->texture->pixelSize()};
    QVector2D destSize(outputSize.width(), outputSize.height());
    QVector2D colorAlpha(1, 0);
    float yScale = (m_rhiContext->rhi()->isYUpInFramebuffer() != m_rhiContext->rhi()->isYUpInNDC()) ? -2.0f : 2.0f;
    QMatrix4x4 mvp;
    mvp.scale(2.0f / outputSize.width(), yScale / outputSize.height());
    float fc = float(m_sgContext->frameCount());
    float fps = float(m_sgContext->getFPS().first);

    // Put values to shader stages
    m_stages->setUniformValue(QByteArrayLiteral("DestSize"), destSize, QSSGRenderShaderDataType::Vec2);
    m_stages->setUniformValue(QByteArrayLiteral("FragColorAlphaSettings"), colorAlpha, QSSGRenderShaderDataType::Vec2);
    m_stages->setUniformValue(QByteArrayLiteral("ModelViewProjectionMatrix"), mvp, QSSGRenderShaderDataType::Matrix4x4);
    m_stages->setUniformValue(QByteArrayLiteral("AppFrame"), fc, QSSGRenderShaderDataType::Float);
    m_stages->setUniformValue(QByteArrayLiteral("FPS"), fps, QSSGRenderShaderDataType::Float);
    m_stages->setUniformValue(QByteArrayLiteral("CameraClipRange"), m_cameraClipRange, QSSGRenderShaderDataType::Vec2);

    addTextureToShaderStages(QByteArrayLiteral("Texture0"), inTexture->texture, inTexture->desc);

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
        qDebug() << "    -> texture binding" << binding << "for" << rhiTex.name;
        bindings.append(QRhiShaderResourceBinding::sampledTexture(binding, QRhiShaderResourceBinding::FragmentStage,
                                                                  rhiTex.texture, m_rhiContext->sampler(rhiTex.samplerDesc)));
    }
    bindings.append(QRhiShaderResourceBinding::uniformBuffer(0, VISIBILITY_ALL, ubs.ubuf));
    QRhiShaderResourceBindings *srb = m_rhiContext->srb(bindings);

    QSSGRhiGraphicsPipelineState ps;
    ps.viewport = QRhiViewport(0, 0, float(outputSize.width()), float(outputSize.height()));
    ps.shaderStages = m_stages->stages();

    m_renderer->rhiQuadRenderer()->recordRenderQuadPass(m_rhiContext.data(), &ps, srb, target->renderTarget, true);
    cb->debugMarkEnd();
}

void QSSGRhiEffectSystem::addTextureToShaderStages(const QByteArray &name, QRhiTexture *texture, const QSSGRhiSamplerDescription &samplerDescription)
{
    //set texture-info uniform
    const bool needsAlphaMultiply = false;
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
