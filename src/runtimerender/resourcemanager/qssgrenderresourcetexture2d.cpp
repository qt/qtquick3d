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

#include "qssgrenderresourcetexture2d_p.h"

QT_BEGIN_NAMESPACE

QSSGResourceTexture2D::QSSGResourceTexture2D(const QSSGRef<QSSGResourceManager> &mgr,
                                                 const QSSGRef<QSSGRenderTexture2D> &inTexture)
    : m_resourceManager(mgr), m_texture(inTexture)
{
    if (inTexture)
        m_textureDetails = inTexture->textureDetails();
}

QSSGResourceTexture2D::QSSGResourceTexture2D(const QSSGRef<QSSGResourceManager> &mgr,
                                                 quint32 width,
                                                 quint32 height,
                                                 QSSGRenderTextureFormat inFormat,
                                                 quint32 inSamples)
    : m_resourceManager(mgr)
{
    ensureTexture(width, height, inFormat, inSamples);
}

QSSGResourceTexture2D::~QSSGResourceTexture2D()
{
    releaseTexture();
}

// Returns true if the texture was allocated, false if nothing changed (no allocation).
bool QSSGResourceTexture2D::textureMatches(qint32 width, qint32 height, QSSGRenderTextureFormat inFormat, qint32 inSamples)
{
    Q_ASSERT(width >= 0 && height >= 0 && inSamples >= 0);
    return m_texture && m_textureDetails.width == width && m_textureDetails.height == height
            && m_textureDetails.format == inFormat && m_textureDetails.sampleCount == inSamples;
}

bool QSSGResourceTexture2D::ensureTexture(qint32 width, qint32 height, QSSGRenderTextureFormat inFormat, qint32 inSamples)
{
    Q_ASSERT(width >= 0 && height >= 0 && inSamples >= 0);
    if (textureMatches(width, height, inFormat, inSamples))
        return false;

    if (m_texture && inSamples > 1) {
        // we cannot resize MSAA textures though release first
        releaseTexture();
    }

    if (!m_texture)
        m_texture = m_resourceManager->allocateTexture2D(width, height, inFormat, inSamples);
    else {
        // multisampled textures are immuteable
        Q_ASSERT(inSamples == 1);
        m_texture->setTextureData(QSSGByteView(), 0, width, height, inFormat);
    }

    m_textureDetails = m_texture->textureDetails();
    return true;
}

void QSSGResourceTexture2D::releaseTexture()
{
    if (m_texture) {
        m_resourceManager->release(m_texture);
        forgetTexture();
    }
}

void QSSGResourceTexture2D::forgetTexture()
{
    m_texture = nullptr;
}

void QSSGResourceTexture2D::stealTexture(QSSGResourceTexture2D &inOther)
{
    releaseTexture();
    m_texture = inOther.m_texture;
    m_textureDetails = inOther.m_textureDetails;
    inOther.m_texture = nullptr;
}

void QSSGResourceTexture2D::swapTexture(QSSGResourceTexture2D &inOther)
{
    QSSGRef<QSSGRenderTexture2D> temp = inOther.m_texture;
    QSSGTextureDetails detailsTemp = inOther.m_textureDetails;
    inOther.m_texture = m_texture;
    inOther.m_textureDetails = m_textureDetails;
    m_texture = temp;
    m_textureDetails = detailsTemp;
}

QT_END_NAMESPACE
