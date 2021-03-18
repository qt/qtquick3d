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

#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRender/private/qssgrenderframebuffer_p.h>
#include <QtQuick3DRender/private/qssgrenderrenderbuffer_p.h>
#include <QtQuick3DRender/private/qssgrendertexture2d_p.h>
#include <QtQuick3DRender/private/qssgrendertexturecube_p.h>

QT_BEGIN_NAMESPACE

template <typename T>
static void replaceWithLast(QVector<T> &vector, int index)
{
    vector[index] = vector.back();
    vector.pop_back();
}


QSSGResourceManager::QSSGResourceManager(const QSSGRef<QSSGRenderContext> &ctx)
    : renderContext(ctx)
{
}

QSSGResourceManager::~QSSGResourceManager() = default;

QSSGRef<QSSGRenderFrameBuffer> QSSGResourceManager::allocateFrameBuffer()
{
    if (freeFrameBuffers.empty() == true) {
        auto newBuffer = new QSSGRenderFrameBuffer(renderContext);
        freeFrameBuffers.push_back(newBuffer);
    }
    auto retval = freeFrameBuffers.back();
    freeFrameBuffers.pop_back();
    return retval;
}

void QSSGResourceManager::release(const QSSGRef<QSSGRenderFrameBuffer> &inBuffer)
{
    if (inBuffer->hasAnyAttachment()) {
        // Ensure the framebuffer has no attachments.
        inBuffer->attach(QSSGRenderFrameBufferAttachment::Color0, QSSGRenderTextureOrRenderBuffer());
        inBuffer->attach(QSSGRenderFrameBufferAttachment::Color1, QSSGRenderTextureOrRenderBuffer());
        inBuffer->attach(QSSGRenderFrameBufferAttachment::Color2, QSSGRenderTextureOrRenderBuffer());
        inBuffer->attach(QSSGRenderFrameBufferAttachment::Color3, QSSGRenderTextureOrRenderBuffer());
        inBuffer->attach(QSSGRenderFrameBufferAttachment::Color4, QSSGRenderTextureOrRenderBuffer());
        inBuffer->attach(QSSGRenderFrameBufferAttachment::Color5, QSSGRenderTextureOrRenderBuffer());
        inBuffer->attach(QSSGRenderFrameBufferAttachment::Color6, QSSGRenderTextureOrRenderBuffer());
        inBuffer->attach(QSSGRenderFrameBufferAttachment::Color7, QSSGRenderTextureOrRenderBuffer());
        inBuffer->attach(QSSGRenderFrameBufferAttachment::Depth, QSSGRenderTextureOrRenderBuffer());
        inBuffer->attach(QSSGRenderFrameBufferAttachment::Stencil, QSSGRenderTextureOrRenderBuffer());
        if (renderContext->supportsDepthStencil())
            inBuffer->attach(QSSGRenderFrameBufferAttachment::DepthStencil, QSSGRenderTextureOrRenderBuffer());
    }
#ifdef _DEBUG
    auto theFind = std::find(freeFrameBuffers.begin(), freeFrameBuffers.end(), inBuffer);
    Q_ASSERT(theFind == freeFrameBuffers.end());
#endif
    freeFrameBuffers.push_back(inBuffer);
}

QSSGRef<QSSGRenderRenderBuffer> QSSGResourceManager::allocateRenderBuffer(qint32 inWidth, qint32 inHeight, QSSGRenderRenderBufferFormat inBufferFormat)
{
    Q_ASSERT(inWidth >= 0 && inHeight >= 0);
    // Look for one of this specific size and format.
    int existingMatchIdx = freeRenderBuffers.size();
    for (int idx = 0, end = existingMatchIdx; idx < end; ++idx) {
        auto theBuffer = freeRenderBuffers[idx];
        QSize theDims = theBuffer->size();
        QSSGRenderRenderBufferFormat theFormat = theBuffer->storageFormat();
        if (theDims.width() == inWidth && theDims.height() == inHeight && theFormat == inBufferFormat) {
            // Replace idx with last for efficient erasure (that reorders the vector).
            replaceWithLast(freeRenderBuffers, idx);
            return theBuffer;
        }
        if (theFormat == inBufferFormat)
            existingMatchIdx = idx;
    }
    // If a specific exact match couldn't be found, just use the buffer with
    // the same format and resize it.
    if (existingMatchIdx < freeRenderBuffers.size()) {
        auto theBuffer = freeRenderBuffers[existingMatchIdx];
        replaceWithLast(freeRenderBuffers, existingMatchIdx);
        theBuffer->setSize(QSize(inWidth, inHeight));
        return theBuffer;
    }

    auto theBuffer = new QSSGRenderRenderBuffer(renderContext, inBufferFormat, inWidth, inHeight);
    return theBuffer;
}

void QSSGResourceManager::release(const QSSGRef<QSSGRenderRenderBuffer> &inBuffer)
{
    freeRenderBuffers.push_back(inBuffer);
}

QSSGRef<QSSGRenderTexture2D> QSSGResourceManager::setupAllocatedTexture(QSSGRef<QSSGRenderTexture2D> inTexture)
{
    inTexture->setMinFilter(QSSGRenderTextureMinifyingOp::Linear);
    inTexture->setMagFilter(QSSGRenderTextureMagnifyingOp::Linear);
    return inTexture;
}

QSSGRef<QSSGRenderTexture2D> QSSGResourceManager::allocateTexture2D(qint32 inWidth, qint32 inHeight, QSSGRenderTextureFormat inTextureFormat, qint32 inSampleCount, bool immutable)
{
    Q_ASSERT(inWidth >= 0 && inHeight >= 0 && inSampleCount >= 0);
    bool inMultisample = inSampleCount > 1 && renderContext->supportsMultisampleTextures();
    for (qint32 idx = 0, end = freeTextures.size(); idx < end; ++idx) {
        auto theTexture = freeTextures[idx];
        QSSGTextureDetails theDetails = theTexture->textureDetails();
        if (theDetails.width == inWidth && theDetails.height == inHeight && inTextureFormat == theDetails.format
                && theTexture->sampleCount() == inSampleCount) {
            replaceWithLast(freeTextures, idx);
            return setupAllocatedTexture(theTexture);
        }
    }
    // else resize an existing texture.  This is very expensive
    // note that MSAA textures are not resizable ( in GLES )
    /*
        if ( !freeTextures.empty() && !inMultisample )
        {
                QSSGRenderTexture2D* theTexture = freeTextures.back();
                freeTextures.pop_back();

                // note we could re-use a former MSAA texture
                // this causes a entiere destroy of the previous texture object
                theTexture->SetTextureData( QSSGByteView(), 0, inWidth, inHeight, inTextureFormat
        );

                return SetupAllocatedTexture( *theTexture );
        }*/
    // else create a new texture.
    auto theTexture = new QSSGRenderTexture2D(renderContext);

    if (inMultisample)
        theTexture->setTextureDataMultisample(inSampleCount, inWidth, inHeight, inTextureFormat);
    else if (immutable)
        theTexture->setTextureStorage(1, inWidth, inHeight, inTextureFormat);
    else
        theTexture->setTextureData(QSSGByteView(), 0, inWidth, inHeight, inTextureFormat);

    return setupAllocatedTexture(theTexture);
}

void QSSGResourceManager::release(const QSSGRef<QSSGRenderTexture2D> &inBuffer)
{
#ifdef _DEBUG
    auto theFind = std::find(freeTextures.begin(), freeTextures.end(), inBuffer);
    Q_ASSERT(theFind == freeTextures.end());
#endif
    freeTextures.push_back(inBuffer);
}

QSSGRef<QSSGRenderTextureCube> QSSGResourceManager::allocateTextureCube(qint32 inWidth, qint32 inHeight, QSSGRenderTextureFormat inTextureFormat, qint32 inSampleCount)
{
    bool inMultisample = inSampleCount > 1 && renderContext->supportsMultisampleTextures();
    for (int idx = 0, end = freeTexCubes.size(); idx < end; ++idx) {
        auto theTexture = freeTexCubes[idx];
        QSSGTextureDetails theDetails = theTexture->textureDetails();
        if (theDetails.width == inWidth && theDetails.height == inHeight && inTextureFormat == theDetails.format
                && theTexture->sampleCount() == inSampleCount) {
            replaceWithLast(freeTexCubes, idx);

            theTexture->setMinFilter(QSSGRenderTextureMinifyingOp::Linear);
            theTexture->setMagFilter(QSSGRenderTextureMagnifyingOp::Linear);
            return theTexture;
        }
    }

    // else resize an existing texture.  This should be fairly quick at the driver level.
    // note that MSAA textures are not resizable ( in GLES )
    if (!freeTexCubes.empty() && !inMultisample) {
        auto theTexture = freeTexCubes.back();
        freeTexCubes.pop_back();

        // note we could re-use a former MSAA texture
        // this causes a entire destroy of the previous texture object
        theTexture->setTextureData(QSSGByteView(), 0, QSSGRenderTextureCubeFace::CubePosX, inWidth, inHeight, inTextureFormat);
        theTexture->setTextureData(QSSGByteView(), 0, QSSGRenderTextureCubeFace::CubeNegX, inWidth, inHeight, inTextureFormat);
        theTexture->setTextureData(QSSGByteView(), 0, QSSGRenderTextureCubeFace::CubePosY, inWidth, inHeight, inTextureFormat);
        theTexture->setTextureData(QSSGByteView(), 0, QSSGRenderTextureCubeFace::CubeNegY, inWidth, inHeight, inTextureFormat);
        theTexture->setTextureData(QSSGByteView(), 0, QSSGRenderTextureCubeFace::CubePosZ, inWidth, inHeight, inTextureFormat);
        theTexture->setTextureData(QSSGByteView(), 0, QSSGRenderTextureCubeFace::CubeNegZ, inWidth, inHeight, inTextureFormat);
        theTexture->setMinFilter(QSSGRenderTextureMinifyingOp::Linear);
        theTexture->setMagFilter(QSSGRenderTextureMagnifyingOp::Linear);
        return theTexture;
    }

    // else create a new texture.
    QSSGRef<QSSGRenderTextureCube> theTexture = nullptr;

    if (!inMultisample) {
        theTexture = new QSSGRenderTextureCube(renderContext);
        theTexture->setTextureData(QSSGByteView(), 0, QSSGRenderTextureCubeFace::CubePosX, inWidth, inHeight, inTextureFormat);
        theTexture->setTextureData(QSSGByteView(), 0, QSSGRenderTextureCubeFace::CubeNegX, inWidth, inHeight, inTextureFormat);
        theTexture->setTextureData(QSSGByteView(), 0, QSSGRenderTextureCubeFace::CubePosY, inWidth, inHeight, inTextureFormat);
        theTexture->setTextureData(QSSGByteView(), 0, QSSGRenderTextureCubeFace::CubeNegY, inWidth, inHeight, inTextureFormat);
        theTexture->setTextureData(QSSGByteView(), 0, QSSGRenderTextureCubeFace::CubePosZ, inWidth, inHeight, inTextureFormat);
        theTexture->setTextureData(QSSGByteView(), 0, QSSGRenderTextureCubeFace::CubeNegZ, inWidth, inHeight, inTextureFormat);
    } else {
        // Not supported yet
        return nullptr;
    }

    theTexture->setMinFilter(QSSGRenderTextureMinifyingOp::Linear);
    theTexture->setMagFilter(QSSGRenderTextureMagnifyingOp::Linear);
    return theTexture;
}

void QSSGResourceManager::release(const QSSGRef<QSSGRenderTextureCube> &inBuffer)
{
#ifdef _DEBUG
    auto theFind = std::find(freeTexCubes.begin(), freeTexCubes.end(), inBuffer);
    Q_ASSERT(theFind == freeTexCubes.end());
#endif
    freeTexCubes.push_back(inBuffer);
}

QSSGRef<QSSGRenderImage2D> QSSGResourceManager::allocateImage2D(const QSSGRef<QSSGRenderTexture2D> &inTexture,
                                                                QSSGRenderImageAccessType inAccess)
{
    if (freeImages.empty() == true) {
        auto newImage = new QSSGRenderImage2D(renderContext, inTexture, inAccess);
        if (newImage) {
            freeImages.push_back(newImage);
        }
    }

    auto retval = freeImages.back();
    freeImages.pop_back();

    return retval;
}

void QSSGResourceManager::release(const QSSGRef<QSSGRenderImage2D> &inBuffer)
{
#ifdef _DEBUG
    auto theFind = std::find(freeImages.begin(), freeImages.end(), inBuffer);
    Q_ASSERT(theFind == freeImages.end());
#endif
    freeImages.push_back(inBuffer);
}

QSSGRef<QSSGRenderContext> QSSGResourceManager::getRenderContext() { return renderContext; }

void QSSGResourceManager::destroyFreeSizedResources()
{
    for (int idx = freeRenderBuffers.size() - 1; idx >= 0; --idx) {
        auto obj = freeRenderBuffers[idx];
        replaceWithLast(freeRenderBuffers, idx);
    }
    for (int idx = freeTextures.size() - 1; idx >= 0; --idx) {
        auto obj = freeTextures[idx];
        replaceWithLast(freeTextures, idx);
    }
    for (int idx = freeTexCubes.size() - 1; idx >= 0; --idx) {
        auto obj = freeTexCubes[idx];
        replaceWithLast(freeTexCubes, idx);
    }
}

QT_END_NAMESPACE
