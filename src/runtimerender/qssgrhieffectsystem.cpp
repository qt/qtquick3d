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

QSSGRhiEffectSystem::QSSGRhiEffectSystem()
{

}

QSSGRhiEffectSystem::~QSSGRhiEffectSystem()
{
    releaseResources();
}

void QSSGRhiEffectSystem::setup(QRhi *rhi, QSize outputSize, QSSGRenderEffect *firstEffect)
{
    if (!firstEffect || outputSize.isEmpty()) {
        releaseResources();
        return;
    }
    bool dirtySize = (outputSize != m_outSize);
    bool dirtyEffects = (firstEffect != m_firstEffect);  //### tbd: check whole chain, and active states
    m_outSize = outputSize;
    m_firstEffect = firstEffect;

    if (!m_outputTexture || !m_RenderTarget || dirtySize || dirtyEffects)
        qDebug() << "@@ SETUP" << bool(m_outputTexture) << dirtySize << dirtyEffects;

    if (!m_outputTexture) {
        m_outputTexture = rhi->newTexture(QRhiTexture::RGBA8, m_outSize, 1, QRhiTexture::RenderTarget);
        m_outputTexture->build();
    } else if (dirtySize) {
        m_outputTexture->setPixelSize(m_outSize);
        m_outputTexture->build();
    }

    if (!m_RenderTarget) {
        m_RenderTarget = rhi->newTextureRenderTarget({ m_outputTexture });
        m_RenderPassDescriptor = m_RenderTarget->newCompatibleRenderPassDescriptor();
        m_RenderTarget->setRenderPassDescriptor(m_RenderPassDescriptor);
        m_RenderTarget->build();
    } else if (dirtySize) {
        m_RenderTarget->build();
    }
}

QRhiTexture *QSSGRhiEffectSystem::process(const QSSGRef<QSSGRhiContext> &rhiCtx,
                                          const QSSGRef<QSSGRendererInterface> &rendererIf, QRhiTexture *inTexture)
{
    QSSGRendererImpl *renderer = static_cast<QSSGRendererImpl *>(rendererIf.data());

    //### For now, do only first effect
    //### For now, ignore active state
    doRenderEffect(m_firstEffect, rhiCtx, renderer, inTexture, m_outputTexture);

    return m_outputTexture;
}

void QSSGRhiEffectSystem::releaseResources()
{
    delete m_RenderTarget;
    m_RenderTarget = nullptr;
    delete m_RenderPassDescriptor;
    m_RenderPassDescriptor = nullptr;
    delete m_outputTexture;
    m_outputTexture = nullptr;
    delete m_ubuf;
    m_ubuf = nullptr;
    m_stages.clear();
}

void QSSGRhiEffectSystem::doRenderEffect(const QSSGRenderEffect *inEffect,
                                         const QSSGRef<QSSGRhiContext> &rhiCtx,
                                         QSSGRendererImpl *renderer,
                                         QRhiTexture *inTexture,
                                         QRhiTexture *outTexture)
{
    Q_UNUSED(outTexture)

    // Run through the effect commands and render the effect.
    const auto &theCommands = inEffect->commands;
    qDebug() << "START effect " << inEffect->className;
    for (const auto &theCommand : theCommands) {
        qDebug() << "  ->" << theCommand->typeAsString();
        switch (theCommand->m_type) {
        case CommandType::ApplyInstanceValue:
            if (m_stages)
                applyInstanceValueCmd(static_cast<QSSGApplyInstanceValue *>(theCommand), inEffect);
            break;
        case CommandType::BindShader:
            bindShaderCmd(static_cast<QSSGBindShader *>(theCommand), inEffect, renderer);
            break;
        case CommandType::Render:
            renderCmd(rhiCtx, renderer, inTexture);
            break;
        default:
            qDebug() << "    > (command not implemented)";
            break;
        }
    }
    qDebug() << "END effect " << inEffect->className;
}

void QSSGRhiEffectSystem::applyInstanceValueCmd(const QSSGApplyInstanceValue *theCommand, const QSSGRenderEffect *inEffect)
{
    Q_ASSERT(m_stages);
    const bool setAll = theCommand->m_propertyName.isEmpty();
    for (const QSSGRenderEffect::Property &property : qAsConst(inEffect->properties)) {
        if (setAll || property.name == theCommand->m_propertyName) {
            m_stages->setUniformValue(property.name, property.value, property.shaderDataType);
        }
    }
}

void QSSGRhiEffectSystem::bindShaderCmd(const QSSGBindShader *theCommand,
                                                                const QSSGRenderEffect *inEffect,
                                                                QSSGRendererImpl *renderer)
{
    //#tbd: check caching here, i.e. how much work per non-initial frame
    QByteArray shaderCode = renderer->contextInterface()->dynamicObjectSystem()->doLoadShader(theCommand->m_shaderPath);

    const QSSGRef<QSSGShaderProgramGeneratorInterface> &generator = renderer->contextInterface()->shaderProgramGenerator();
    generator->beginProgram();
    //#TBD shell out a function instead of duplicating the following lines for vertex and fragment shaders
    QSSGShaderStageGeneratorInterface *vStage = generator->getStage(QSSGShaderGeneratorStage::Vertex);
    Q_ASSERT(vStage);
    vStage->append(QByteArrayLiteral("#define VERTEX_SHADER\n"));
    vStage->append(shaderCode);
    QSSGShaderStageGeneratorInterface *fStage = generator->getStage(QSSGShaderGeneratorStage::Fragment);
    Q_ASSERT(fStage);
    for (const QSSGRenderEffect::Property &property : qAsConst(inEffect->properties)) {
        fStage->addUniform(property.name, property.typeName);
    }
    //for (const QSSGRenderEffect::TextureProperty &property : qAsConst(inEffect->textureProperties)) {
        //fStage->addUniform(property.name, property.typeName);
        //#TBD
    //}

    //#TBD texture properties
    fStage->append(QByteArrayLiteral("#define FRAGMENT_SHADER\n"
                                     "#define gl_FragColor fragOutput\n"));
    fStage->append(shaderCode);

    ShaderFeatureSetList features;
    features.push_back(QSSGShaderPreprocessorFeature(QSSGShaderDefines::asString(QSSGShaderDefines::Rhi), true));

    QSSGRef<QSSGRhiShaderStages> stages;
    stages = generator->compileGeneratedRhiShader(theCommand->m_shaderPath, QSSGShaderCacheProgramFlags(), features);
    if (!stages.isNull())
        m_stages = QSSGRhiShaderStagesWithResources::fromShaderStages(stages);
}

//### copied from qssgrendererimpllayerrenderdata_rhi.cpp, TBD: merge
static inline int bindingForTexture(const QString &name, const QSSGRhiShaderStages &shaderStages)
{
    QVector<QShaderDescription::InOutVariable> samplerVariables =
            shaderStages.fragmentStage()->shader().description().combinedImageSamplers();
    auto it = std::find_if(samplerVariables.cbegin(), samplerVariables.cend(),
                           [&name](const QShaderDescription::InOutVariable &s) { return s.name == name; });
    if (it != samplerVariables.cend())
        return it->binding;

    return -1;
}

void QSSGRhiEffectSystem::renderCmd(const QSSGRef<QSSGRhiContext> &rhiCtx, QSSGRendererImpl *renderer,
                                    QRhiTexture *inTexture)
{
    if (!m_stages) {
        qWarning("No post-processing shader set");
        return;
    }
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    cb->debugMarkBegin(QByteArrayLiteral("Post-processing effect"));
    renderer->rhiQuadRenderer()->prepareQuad(rhiCtx.data(), nullptr);

    // Uniforms from effects.glsl.
    QVector2D destSize;
    QVector2D colorAlpha;
    QMatrix4x4 mvp;
    QVector4D tex0Info;

    // effects.glsl uniform values  ##For now, hardcoded or computed here
    destSize = QVector2D(m_outSize.width(), m_outSize.height());
    colorAlpha = QVector2D(1, 0);
    float yScale = (rhiCtx->rhi()->isYUpInFramebuffer() != rhiCtx->rhi()->isYUpInNDC()) ? -2.0f : 2.0f;
    mvp.scale(2.0f / m_outSize.width(), yScale / m_outSize.height());
    QSize inTexSize = inTexture->pixelSize();
    const bool needsAlphaMultiply = false; //#?
    float theMixValue = needsAlphaMultiply ? 0.0f : 1.0f;
    tex0Info = QVector4D(inTexSize.width(), inTexSize.height(), theMixValue, 0);

    // Put values to shader stages
    m_stages->setUniformValue(QByteArrayLiteral("DestSize"), destSize, QSSGRenderShaderDataType::Vec2);
    m_stages->setUniformValue(QByteArrayLiteral("FragColorAlphaSettings"), colorAlpha, QSSGRenderShaderDataType::Vec2);
    m_stages->setUniformValue(QByteArrayLiteral("ModelViewProjectionMatrix"), mvp, QSSGRenderShaderDataType::Matrix4x4);
    m_stages->setUniformValue(QByteArrayLiteral("Texture0Info"), tex0Info, QSSGRenderShaderDataType::Vec4);

    // bake uniform buffer
    QRhiResourceUpdateBatch *rub = rhiCtx->rhi()->nextResourceUpdateBatch();
    m_stages->bakeMainUniformBuffer(&m_ubuf, rub);
    cb->resourceUpdate(rub);

    // do resource bindings
    QRhiSampler *sampler = rhiCtx->sampler({ QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                             QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge });
    int tex0Binding = bindingForTexture(QStringLiteral("Texture0"), *m_stages->stages());
    const QRhiShaderResourceBinding::StageFlags VISIBILITY_ALL =
            QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;
    QSSGRhiContext::ShaderResourceBindingList bindings;
    bindings.append(QRhiShaderResourceBinding::sampledTexture(tex0Binding, QRhiShaderResourceBinding::FragmentStage, inTexture, sampler));
    bindings.append(QRhiShaderResourceBinding::uniformBuffer(0, VISIBILITY_ALL, m_ubuf));
    QRhiShaderResourceBindings *srb = rhiCtx->srb(bindings);

    QSSGRhiGraphicsPipelineState ps;
    ps.viewport = QRhiViewport(0, 0, float(m_outSize.width()), float(m_outSize.height()));
    ps.shaderStages = m_stages->stages();

    renderer->rhiQuadRenderer()->recordRenderQuadPass(rhiCtx.data(), &ps, srb, m_RenderTarget, true);
    cb->debugMarkEnd();
}

QT_END_NAMESPACE
