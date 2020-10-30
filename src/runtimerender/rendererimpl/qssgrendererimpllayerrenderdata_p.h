/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2019 The Qt Company Ltd.
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

#ifndef QSSG_RENDERER_IMPL_LAYER_RENDER_DATA_H
#define QSSG_RENDERER_IMPL_LAYER_RENDER_DATA_H

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

#include <QtQuick3DRuntimeRender/private/qssgrendererimpllayerrenderpreparationdata_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>

QT_BEGIN_NAMESPACE

struct QSSGRhiRenderableTexture
{
    QSSGRhiContext *rhiCtx = nullptr;
    QRhiTexture *texture = nullptr;
    QRhiRenderBuffer *depthStencil = nullptr;
    QRhiRenderPassDescriptor *rpDesc = nullptr;
    QRhiTextureRenderTarget *rt = nullptr;
    bool isValid() const { return texture && rpDesc && rt; }
    void resetRenderTarget() {
        if (rhiCtx && rpDesc)
            rhiCtx->invalidateCachedReferences(rpDesc);
        delete rt;
        rt = nullptr;
        delete rpDesc;
        rpDesc = nullptr;
    }
    void reset() {
        resetRenderTarget();
        delete texture;
        delete depthStencil;
        *this = QSSGRhiRenderableTexture();
    }
};

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGLayerRenderData : public QSSGLayerRenderPreparationData
{
    QAtomicInt ref;

    QSSGRenderTextureFormat m_depthBufferFormat;

    // RHI resources
    QSSGRhiRenderableTexture m_rhiDepthTexture;
    QSSGRhiRenderableTexture m_rhiAoTexture;
    QSSGRhiRenderableTexture m_rhiScreenTexture;

    QSSGRenderCamera m_sceneCamera;
    QVector2D m_sceneDimensions;

    // ProgressiveAA algorithm details.
    quint32 m_progressiveAAPassIndex;
    // Increments every frame regardless to provide appropriate jittering
    quint32 m_temporalAAPassIndex;

    float m_textScale;

    QSSGOption<QVector3D> m_boundingRectColor;

    QSize m_previousDimensions;

    bool m_zPrePassPossible;

    QSSGLayerRenderData(QSSGRenderLayer &inLayer, const QSSGRef<QSSGRenderer> &inRenderer);

    virtual ~QSSGLayerRenderData() override;

    // Internal Call
    void prepareForRender(const QSize &inViewportDimensions) override;
    void resetForFrame() final;

    // RHI-only
    void rhiPrepare();
    void rhiRender();

};
QT_END_NAMESPACE
#endif
