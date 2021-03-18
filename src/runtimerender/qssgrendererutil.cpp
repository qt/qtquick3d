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

#include "qssgrendererutil_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrenderresourcebufferobjects_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderresourcetexture2d_p.h>

QT_BEGIN_NAMESPACE

void QSSGRendererUtil::resolveMutisampleFBOColorOnly(const QSSGRef<QSSGResourceManager> &inManager,
                                                       QSSGResourceTexture2D &ioResult,
                                                       QSSGRenderContext &inRenderContext,
                                                       qint32 inWidth,
                                                       qint32 inHeight,
                                                       QSSGRenderTextureFormat inColorFormat,
                                                       const QSSGRef<QSSGRenderFrameBuffer> &inSourceFBO)
{
    // create resolve FBO
    QSSGResourceFrameBuffer theResolveFB(inManager);
    // Allocates the frame buffer which has the side effect of setting the current render target to
    // that frame buffer.
    theResolveFB.ensureFrameBuffer();
    // set copy flags
    QSSGRenderClearFlags copyFlags(QSSGRenderClearValues::Color);

    // get / create resolve targets and attach
    ioResult.ensureTexture(inWidth, inHeight, inColorFormat);
    theResolveFB->attach(QSSGRenderFrameBufferAttachment::Color0, ioResult.getTexture());
    // CN - I don't believe we have to resolve the depth.
    // The reason is we render the depth texture specially unresolved.  So there is no need to
    // resolve
    // the depth prepass texture to anything else.

    // 1. Make resolve buffer be the render target ( already happend )
    // 2. Make the current layer FBO the current read target
    // 3. Do the blit from MSAA to non MSAA

    // 2.
    inRenderContext.setReadTarget(inSourceFBO);
    inRenderContext.setReadBuffer(QSSGReadFace::Color0);
    // 3.
    inRenderContext.blitFramebuffer(0, 0, inWidth, inHeight, 0, 0, inWidth, inHeight, copyFlags, QSSGRenderTextureMagnifyingOp::Nearest);
}

void QSSGRendererUtil::resolveSSAAFBOColorOnly(const QSSGRef<QSSGResourceManager> &inManager,
                                                 QSSGResourceTexture2D &ioResult,
                                                 qint32 outWidth,
                                                 qint32 outHeight,
                                                 QSSGRenderContext &inRenderContext,
                                                 qint32 inWidth,
                                                 qint32 inHeight,
                                                 QSSGRenderTextureFormat inColorFormat,
                                                 const QSSGRef<QSSGRenderFrameBuffer> &inSourceFBO)
{
    // create resolve FBO
    QSSGResourceFrameBuffer theResolveFB(inManager);
    // Allocates the frame buffer which has the side effect of setting the current render target to
    // that frame buffer.
    theResolveFB.ensureFrameBuffer();
    // set copy flags
    QSSGRenderClearFlags copyFlags(QSSGRenderClearValues::Color);

    // get / create resolve targets and attach
    ioResult.ensureTexture(outWidth, outHeight, inColorFormat);
    theResolveFB->attach(QSSGRenderFrameBufferAttachment::Color0, ioResult.getTexture());
    // CN - I don't believe we have to resolve the depth.
    // The reason is we render the depth texture specially unresolved.  So there is no need to
    // resolve
    // the depth prepass texture to anything else.

    // 1. Make resolve buffer be the render target ( already happend )
    // 2. Make the current layer FBO the current read target
    // 3. Do the blit from High res to low res buffer

    // 2.
    inRenderContext.setReadTarget(inSourceFBO);
    inRenderContext.setReadBuffer(QSSGReadFace::Color0);
    // 3.
    inRenderContext.blitFramebuffer(0, 0, inWidth, inHeight, 0, 0, outWidth, outHeight, copyFlags, QSSGRenderTextureMagnifyingOp::Linear);
}

void QSSGRendererUtil::getSSAARenderSize(qint32 inWidth, qint32 inHeight, qint32 &outWidth, qint32 &outHeight)
{
    // we currently double width and height
    outWidth = inWidth * 2;
    outHeight = inHeight * 2;

    // keep aspect ration?
    // clamp to max
    if (outWidth > MAX_SSAA_DIM)
        outWidth = MAX_SSAA_DIM;
    if (outHeight > MAX_SSAA_DIM)
        outHeight = MAX_SSAA_DIM;
}

QT_END_NAMESPACE
