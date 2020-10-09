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

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiEffectSystem
{
public:
    QSSGRhiEffectSystem(const QSSGRef<QSSGRenderContextInterface> &sgContext);
    ~QSSGRhiEffectSystem();

    void setup(QRhi *rhi, QSize outputSize, QSSGRenderEffect *firstEffect);
    QRhiTexture *process(const QSSGRef<QSSGRhiContext> &rhiCtx,
                         const QSSGRef<QSSGRenderer> &renderer,
                         QRhiTexture *inTexture,
                         QRhiTexture *inDepthTexture,
                         QVector2D cameraClipRange);

    static QSSGRenderTextureFormat::Format overriddenOutputFormat(const QSSGRenderEffect *inEffect);

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
    const QSSGRenderEffect *m_firstEffect = nullptr;
    QSSGRenderContextInterface *m_sgContext = nullptr;
    QVector<QSSGRhiEffectTexture *> m_textures;
    QRhiTexture *m_depthTexture = nullptr;
    QVector2D m_cameraClipRange;
    QSSGRhiEffectTexture *m_currentOutput = nullptr;
    int m_currentUbufIndex = 0;
    QSSGRef<QSSGRhiContext> m_rhiContext;
    QSSGRenderer *m_renderer = nullptr;
    QHash<QByteArray, QSSGRef<QSSGRhiShaderPipeline>> m_shaderPipelines;
    QSSGRhiShaderPipeline *m_currentShaderPipeline = nullptr;
    char *m_currentUBufData = nullptr;
    QHash<QByteArray, QSSGRhiTexture> m_currentTextures;
    QSet<QRhiTextureRenderTarget *> m_pendingClears;
};

QT_END_NAMESPACE

#endif
