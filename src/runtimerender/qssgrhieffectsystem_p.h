// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGRHIEFFECTSYSTEM_P_H
#define QSSGRHIEFFECTSYSTEM_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicontext_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercommands_p.h>

QT_BEGIN_NAMESPACE

struct QSSGRhiEffectTexture;
class QSSGRenderer;

struct QSSGProgramGenerator;
struct QSSGShaderLibraryManager;
class QSSGShaderCache;


class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiEffectSystem
{
public:
    QSSGRhiEffectSystem(const QSSGRef<QSSGRenderContextInterface> &sgContext);
    ~QSSGRhiEffectSystem();

    void setup(QSize outputSize);
    QRhiTexture *process(const QSSGRef<QSSGRhiContext> &rhiCtx,
                         const QSSGRef<QSSGRenderer> &renderer,
                         const QSSGRenderEffect &firstEffect,
                         QRhiTexture *inTexture,
                         QRhiTexture *inDepthTexture,
                         QVector2D cameraClipRange);

    static QSSGRenderTextureFormat::Format overriddenOutputFormat(const QSSGRenderEffect *inEffect);

    static QSSGRef<QSSGRhiShaderPipeline> buildShaderForEffect(const QSSGBindShader &inCmd,
                                                               const QSSGRef<QSSGProgramGenerator> &generator,
                                                               const QSSGRef<QSSGShaderLibraryManager> &shaderLib,
                                                               const QSSGRef<QSSGShaderCache> &shaderCache,
                                                               bool isYUpInFramebuffer);

private:
    void releaseResources();
    QSSGRhiEffectTexture *doRenderEffect(const QSSGRenderEffect *inEffect,
                        QSSGRhiEffectTexture *inTexture);

    void allocateBufferCmd(const QSSGAllocateBuffer *inCmd, QSSGRhiEffectTexture *inTexture);
    void applyInstanceValueCmd(const QSSGApplyInstanceValue *inCmd, const QSSGRenderEffect *inEffect);
    void applyValueCmd(const QSSGApplyValue *inCmd, const QSSGRenderEffect *inEffect);
    void bindShaderCmd(const QSSGBindShader *inCmd);
    void renderCmd(QSSGRhiEffectTexture *inTexture, QSSGRhiEffectTexture *target);

    void addCommonEffectUniforms(const QSize &inputSize, const QSize &outputSize);
    void addTextureToShaderPipeline(const QByteArray &name, QRhiTexture *texture, const QSSGRhiSamplerDescription &samplerDesc);

    QSSGRhiEffectTexture *findTexture(const QByteArray &bufferName);
    QSSGRhiEffectTexture *getTexture(const QByteArray &bufferName, const QSize &size,
                                     QRhiTexture::Format format, bool isFinalOutput);
    void releaseTexture(QSSGRhiEffectTexture *texture);
    void releaseTextures();

    QSize m_outSize;
    QSSGRenderContextInterface *m_sgContext = nullptr;
    QVector<QSSGRhiEffectTexture *> m_textures;
    QRhiTexture *m_depthTexture = nullptr;
    QVector2D m_cameraClipRange;
    QSSGRhiEffectTexture *m_currentOutput = nullptr;
    int m_currentUbufIndex = 0;
    QSSGRef<QSSGRhiContext> m_rhiContext;
    QSSGRenderer *m_renderer = nullptr;
    QHash<quintptr, QSSGRef<QSSGRhiShaderPipeline>> m_shaderPipelines;
    QSSGRhiShaderPipeline *m_currentShaderPipeline = nullptr;
    char *m_currentUBufData = nullptr;
    QHash<QByteArray, QSSGRhiTexture> m_currentTextures;
    QSet<QRhiTextureRenderTarget *> m_pendingClears;
};

QT_END_NAMESPACE

#endif
