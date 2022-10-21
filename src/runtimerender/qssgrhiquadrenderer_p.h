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

#ifndef QSSGRHIQUADRENDERER_P_H
#define QSSGRHIQUADRENDERER_P_H

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

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicontext_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRhiQuadRenderer
{
public:
    // The expected usage is one of the following:
    // prepareQuad() + recordRenderQuadPass() right after each other (to record a full standalone renderpass, in the prepare phase)
    // prepareQuad() in prepare phase, then recordRenderQuad() in render phase (to draw a quad within the main renderpass)

    enum Flag {
        UvCoords = 0x01,
        DepthTest = 0x02,
        DepthWrite = 0x04,
        PremulBlend = 0x08
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    void prepareQuad(QSSGRhiContext *rhiCtx, QRhiResourceUpdateBatch *maybeRub);

    void recordRenderQuad(QSSGRhiContext *rhiCtx,
                          QSSGRhiGraphicsPipelineState *ps, QRhiShaderResourceBindings *srb,
                          QRhiRenderPassDescriptor *rpDesc, Flags flags);

    void recordRenderQuadPass(QSSGRhiContext *rhiCtx,
                              QSSGRhiGraphicsPipelineState *ps, QRhiShaderResourceBindings *srb,
                              QRhiTextureRenderTarget *rt, Flags flags);

private:
    void ensureBuffers(QSSGRhiContext *rhiCtx, QRhiResourceUpdateBatch *rub);

    QSSGRef<QSSGRhiBuffer> m_vbuf;
    QSSGRef<QSSGRhiBuffer> m_ibuf;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSSGRhiQuadRenderer::Flags)

QT_END_NAMESPACE

#endif
