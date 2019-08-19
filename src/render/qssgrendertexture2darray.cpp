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

#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRender/private/qssgrendertexture2darray_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderTexture2DArray::QSSGRenderTexture2DArray(const QSSGRef<QSSGRenderContext> &context)
    : QSSGRenderTextureBase(context, QSSGRenderTextureTargetType::Texture2D_Array), m_width(0), m_height(0), m_slices(0)
{
}

QSSGRenderTexture2DArray::~QSSGRenderTexture2DArray()
{
}

void QSSGRenderTexture2DArray::setTextureData(QSSGByteView newBuffer,
                                                quint8 inMipLevel,
                                                qint32 width,
                                                qint32 height,
                                                qint32 slices,
                                                QSSGRenderTextureFormat format)
{
    Q_ASSERT(m_handle);
    Q_ASSERT(width >= 0 && height >= 0 && slices >= 0);
    if (inMipLevel == 0) {
        m_width = width;
        m_height = height;
        m_slices = slices;
        m_format = format;
        m_maxMipLevel = inMipLevel;
    }

    if (m_maxMipLevel < inMipLevel) {
        m_maxMipLevel = inMipLevel;
    }

    // get max size and check value
    qint32 theMaxLayerSize, theMaxSize;
    m_backend->getRenderBackendValue(QSSGRenderBackend::QSSGRenderBackendQuery::MaxTextureSize, &theMaxSize);
    m_backend->getRenderBackendValue(QSSGRenderBackend::QSSGRenderBackendQuery::MaxTextureArrayLayers, &theMaxLayerSize);
    if (width > theMaxSize || height > theMaxSize || slices > theMaxLayerSize) {
        qCCritical(INVALID_OPERATION, "Width or height or Slices is greater than max texture size (%d, %d, %d)", theMaxSize, theMaxSize, theMaxLayerSize);
    }

    // currently we do not support compressed texture arrays
    Q_ASSERT(format.isUncompressedTextureFormat()
             || format.isDepthTextureFormat());

    if (format.isUncompressedTextureFormat() || format.isDepthTextureFormat()) {
        m_backend->setTextureData3D(m_handle,
                                    m_texTarget,
                                    inMipLevel,
                                    m_format,
                                    width,
                                    height,
                                    slices,
                                    0,
                                    format,
                                    newBuffer);
    }

    // Set our texture parameters to a default that will look the best
    if (inMipLevel > 0)
        setMinFilter(QSSGRenderTextureMinifyingOp::LinearMipmapLinear);
}

QSSGTextureDetails QSSGRenderTexture2DArray::textureDetails() const
{
    return QSSGTextureDetails(m_width, m_height, m_slices, m_sampleCount, m_format);
}

void QSSGRenderTexture2DArray::bind()
{
    m_textureUnit = m_context->nextTextureUnit();

    m_backend->bindTexture(m_handle, m_texTarget, m_textureUnit);

    applyTexParams();
    applyTexSwizzle();
}

QT_END_NAMESPACE
