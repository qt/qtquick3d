// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgrendertexturedata_p.h"

QT_BEGIN_NAMESPACE

QSSGRenderTextureData::QSSGRenderTextureData()
    : QSSGRenderGraphObject(QSSGRenderGraphObject::Type::TextureData)
{

}

QSSGRenderTextureData::QSSGRenderTextureData(QSSGRenderGraphObject::Type type)
    : QSSGRenderGraphObject(type)
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
    markDirty();
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
    markDirty();
}

int QSSGRenderTextureData::depth() const
{
    return m_depth;
}

void QSSGRenderTextureData::setDepth(int depth)
{
    if (m_depth == depth)
        return;
    m_depth = depth;
    markDirty();
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
    markDirty();
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
    markDirty();
}

uint32_t QSSGRenderTextureData::generationId() const
{
    return m_generationId;
}

void QSSGRenderTextureData::markDirty()
{
    // The generation ID changes every time a property of this texture
    // changes so that the buffer manager can compare the generation it
    // holds vs the current generation.
    m_generationId++;
}

QT_END_NAMESPACE
