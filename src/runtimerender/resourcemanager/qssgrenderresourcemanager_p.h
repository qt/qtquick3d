/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QSSG_RENDER_RESOURCE_MANAGER_H
#define QSSG_RENDER_RESOURCE_MANAGER_H

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

#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>
#include <QtQuick3DRender/private/qssgrenderrenderbuffer_p.h>
#include <QtQuick3DRender/private/qssgrendercontext_p.h>

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>

QT_BEGIN_NAMESPACE
/**
 *	Implements simple pooling of render resources
 */
class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGResourceManager
{
    Q_DISABLE_COPY(QSSGResourceManager)
public:
    QAtomicInt ref;
private:
    QSSGRef<QSSGRenderContext> renderContext;
    // Complete list of all allocated objects
    //    QVector<QSSGRef<QSSGRefCounted>> m_allocatedObjects;

    QVector<QSSGRef<QSSGRenderFrameBuffer>> freeFrameBuffers;
    QVector<QSSGRef<QSSGRenderRenderBuffer>> freeRenderBuffers;
    QVector<QSSGRef<QSSGRenderTexture2D>> freeTextures;
    QVector<QSSGRef<QSSGRenderTextureCube>> freeTexCubes;
    QVector<QSSGRef<QSSGRenderImage2D>> freeImages;

    QSSGRef<QSSGRenderTexture2D> setupAllocatedTexture(QSSGRef<QSSGRenderTexture2D> inTexture);

public:
    QSSGResourceManager(const QSSGRef<QSSGRenderContext> &ctx);
    ~QSSGResourceManager();

    QSSGRef<QSSGRenderFrameBuffer> allocateFrameBuffer();
    void release(const QSSGRef<QSSGRenderFrameBuffer> &inBuffer);
    QSSGRef<QSSGRenderRenderBuffer> allocateRenderBuffer(qint32 inWidth,
                                                                     qint32 inHeight,
                                                                     QSSGRenderRenderBufferFormat inBufferFormat);
    void release(const QSSGRef<QSSGRenderRenderBuffer> &inBuffer);
    QSSGRef<QSSGRenderTexture2D> allocateTexture2D(qint32 inWidth,
                                                   qint32 inHeight,
                                                   QSSGRenderTextureFormat inTextureFormat,
                                                   qint32 inSampleCount = 1,
                                                   bool immutable = false);
    void release(const QSSGRef<QSSGRenderTexture2D> &inBuffer);
    QSSGRef<QSSGRenderTextureCube> allocateTextureCube(qint32 inWidth,
                                                                   qint32 inHeight,
                                                                   QSSGRenderTextureFormat inTextureFormat,
                                                                   qint32 inSampleCount = 1);
    void release(const QSSGRef<QSSGRenderTextureCube> &inBuffer);
    QSSGRef<QSSGRenderImage2D> allocateImage2D(const QSSGRef<QSSGRenderTexture2D> &inTexture,
                                               QSSGRenderImageAccessType inAccess);
    void release(const QSSGRef<QSSGRenderImage2D> &inBuffer);

    QSSGRef<QSSGRenderContext> getRenderContext();
    void destroyFreeSizedResources();
};

QT_END_NAMESPACE

#endif
