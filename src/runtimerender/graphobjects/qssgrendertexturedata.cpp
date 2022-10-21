/****************************************************************************
**
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

#include "qssgrendertexturedata_p.h"

QT_BEGIN_NAMESPACE

QSSGRenderTextureData::QSSGRenderTextureData()
    : QSSGRenderGraphObject(QSSGRenderGraphObject::Type::TextureData)
{

}

QSSGRenderTextureData::~QSSGRenderTextureData()
{

}

const QByteArray &QSSGRenderTextureData::textureData() const
{
    return m_textureData;
}

void QSSGRenderTextureData::setTextureData(const QByteArray &data)
{
    m_textureData = data;
    m_dirty = true;
}

QSize QSSGRenderTextureData::size() const
{
    return m_size;
}

void QSSGRenderTextureData::setSize(const QSize &size)
{
    if (m_size == size)
        return;
    m_size = size;
    m_dirty = true;
}

QSSGRenderTextureFormat QSSGRenderTextureData::format() const
{
    return m_format;
}

void QSSGRenderTextureData::setFormat(QSSGRenderTextureFormat format)
{
    if (m_format == format)
        return;

    m_format = format;
    m_dirty = true;
}

bool QSSGRenderTextureData::hasTransparancy() const
{
    return m_hasTransparency;
}

void QSSGRenderTextureData::setHasTransparency(bool hasTransparency)
{
    if (m_hasTransparency == hasTransparency)
        return;

    m_hasTransparency = hasTransparency;
    m_dirty = true;
}

QSSGRenderImageTexture QSSGRenderTextureData::createOrUpdate(const QSSGRef<QSSGBufferManager> &bufferManager,
                                                             QSSGBufferManager::MipMode mipMode)
{
    if (m_dirty) {
        // The dirty flag is important also because loadTextureData destroys
        // and creates a new QRhiTexture every time, so we do not want to enter
        // it unless absolutely necessary.
        m_dirty = false;
        m_texture = bufferManager->loadTextureData(this, mipMode);
        // release CPU texture data
        // ### make sure that we dont try to use the raw data again unless it is reset
        m_textureData.clear();
    }
    return m_texture;
}

QT_END_NAMESPACE
