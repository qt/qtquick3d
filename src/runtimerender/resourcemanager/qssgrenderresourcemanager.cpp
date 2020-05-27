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

#include "qssgrenderresourcemanager_p.h"

QT_BEGIN_NAMESPACE

template <typename T>
static void replaceWithLast(QVector<T> &vector, int index)
{
    vector[index] = vector.back();
    vector.pop_back();
}

QSSGResourceManager::QSSGResourceManager(const QSSGRef<QSSGRhiContext> &ctx)
    : rhiContext(ctx)
{
}

QSSGResourceManager::~QSSGResourceManager() = default;

QRhiTexture *QSSGResourceManager::allocateRhiTexture(qint32 inWidth,
                                                     qint32 inHeight,
                                                     QRhiTexture::Format inFormat,
                                                     QRhiTexture::Flags inFlags)
{
    for (int idx = 0, end = freeRhiTextures.size(); idx < end; ++idx) {
        QRhiTexture *theTexture = freeRhiTextures[idx];
        const QSize theSize = theTexture->pixelSize();
        if (theSize.width() == inWidth && theSize.height() == inHeight
                && theTexture->format() == inFormat && theTexture->flags() == inFlags)
        {
            replaceWithLast(freeRhiTextures, idx);
            return theTexture;
        }
    }
    QRhi *rhi = rhiContext->rhi();
    QRhiTexture *tex = rhi->newTexture(inFormat, QSize(inWidth, inHeight), 1, inFlags);
    if (!tex->create())
        qWarning("Failed to build shadow map texture of size %dx%d", inWidth, inHeight);
    return tex;
}

void QSSGResourceManager::release(QRhiTexture *inTexture)
{
#ifdef _DEBUG
    Q_ASSERT(!freeRhiTextures.contains(inTexture));
#endif
    freeRhiTextures.push_back(inTexture);
}

QRhiRenderBuffer *QSSGResourceManager::allocateRhiRenderBuffer(qint32 inWidth,
                                                               qint32 inHeight,
                                                               QRhiRenderBuffer::Type inType)
{
    for (int idx = 0, end = freeRhiRenderBuffers.size(); idx < end; ++idx) {
        QRhiRenderBuffer *theRenderBuffer = freeRhiRenderBuffers[idx];
        const QSize theSize = theRenderBuffer->pixelSize();
        if (theSize.width() == inWidth && theSize.height() == inHeight && theRenderBuffer->type() == inType) {
            replaceWithLast(freeRhiRenderBuffers, idx);
            return theRenderBuffer;
        }
    }
    QRhi *rhi = rhiContext->rhi();
    QRhiRenderBuffer *rb = rhi->newRenderBuffer(inType, QSize(inWidth, inHeight), 1);
    if (!rb->create())
        qWarning("Failed to build depth-stencil buffer of size %dx%d", inWidth, inHeight);
    return rb;
}

void QSSGResourceManager::release(QRhiRenderBuffer *inRenderBuffer)
{
#ifdef _DEBUG
    Q_ASSERT(!freeRhiRenderBuffers.contains(inRenderBuffer));
#endif
    freeRhiRenderBuffers.push_back(inRenderBuffer);
}

void QSSGResourceManager::destroyFreeSizedResources()
{
    qDeleteAll(freeRhiTextures);
    freeRhiTextures.clear();
    qDeleteAll(freeRhiRenderBuffers);
    freeRhiRenderBuffers.clear();
}

QT_END_NAMESPACE
