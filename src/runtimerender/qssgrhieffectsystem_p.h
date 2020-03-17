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
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRender/private/qssgrhicontext_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdynamicobjectsystemcommands_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiEffectSystem
{
public:
    QSSGRhiEffectSystem();
    ~QSSGRhiEffectSystem();

    void setup(QRhi *rhi, QSize outputSize, QSSGRenderEffect *firstEffect);
    QRhiTexture *process(const QSSGRef<QSSGRhiContext> &rhiCtx,
                         const QSSGRef<QSSGRendererInterface> &rendererIf,
                         QRhiTexture *inTexture);

private:
    void releaseResources();
    void doRenderEffect(const QSSGRenderEffect *inEffect,
                        QRhiTexture *inTexture,
                        QRhiTextureRenderTarget *renderTarget,
                        QRhiTexture *outTexture);

    void applyInstanceValueCmd(const dynamic::QSSGApplyInstanceValue *theCommand,
                               const QSSGRenderEffect *inEffect);
    void bindShaderCmd(const dynamic::QSSGBindShader *theCommand,
                       const QSSGRenderEffect *inEffect);
    void renderCmd(QRhiTexture *inTexture, QRhiTextureRenderTarget *renderTarget);

    void setTextureInfoUniform(const QByteArray &texName, QRhiTexture *tex, bool needsAlphaMultiply = false);

    QSize m_outSize;
    const QSSGRenderEffect *m_firstEffect = nullptr;

    //TODO: combined texture & target convenience class
    QRhiTextureRenderTarget *m_RenderTarget = nullptr;
    QRhiRenderPassDescriptor *m_RenderPassDescriptor = nullptr;
    QRhiTexture *m_outputTexture = nullptr;
    QRhiTextureRenderTarget *m_tmpRenderTarget = nullptr;
    QRhiRenderPassDescriptor *m_tmpRenderPassDescriptor = nullptr;
    QRhiTexture *m_tmpTexture = nullptr; // TODO texture pool
    int m_currentUbufIndex = 0;
    QSSGRef<QSSGRhiContext> m_rhiContext;
    QSSGRendererImpl *m_renderer = nullptr;
    QSSGRef<QSSGRhiShaderStagesWithResources> m_stages;
};

QT_END_NAMESPACE

#endif
