/****************************************************************************
**
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

#include "qssgrendergeometry_p.h"
#include "qssgrendermesh_p.h"
#include "resourcemanager/qssgrenderbuffermanager_p.h"

QSSGRenderGeometry::QSSGRenderGeometry()
    : QSSGRenderGraphObject(QSSGRenderGraphObject::Type::Geometry)
{

}

QSSGRenderGeometry::~QSSGRenderGeometry()
{

}

const QByteArray &QSSGRenderGeometry::vertexBuffer() const
{
    return m_meshData.m_vertexBuffer;
}

QByteArray &QSSGRenderGeometry::vertexBuffer()
{
    return m_meshData.m_vertexBuffer;
}

const QByteArray &QSSGRenderGeometry::indexBuffer() const
{
    return m_meshData.m_indexBuffer;
}

QByteArray &QSSGRenderGeometry::indexBuffer()
{
    return m_meshData.m_indexBuffer;
}

int QSSGRenderGeometry::attributeCount() const
{
    return m_meshData.m_attributeCount;
}

QVector3D QSSGRenderGeometry::boundsMin() const
{
    return m_bounds.minimum;
}

QVector3D QSSGRenderGeometry::boundsMax() const
{
    return m_bounds.maximum;
}

int QSSGRenderGeometry::stride() const
{
    return m_meshData.m_stride;
}

QString QSSGRenderGeometry::path() const
{
    return m_meshPath.path;
}

QSSGRenderGeometry::PrimitiveType QSSGRenderGeometry::primitiveType() const
{
    return static_cast<QSSGRenderGeometry::PrimitiveType>(m_meshData.m_primitiveType);
}

QSSGRenderGeometry::Attribute QSSGRenderGeometry::attribute(int idx) const
{
    Attribute attr;
    const auto &mattr = m_meshData.m_attributes[idx];
    attr.offset = mattr.offset;
    attr.semantic = static_cast<QSSGRenderGeometry::Attribute::Semantic>(mattr.semantic);
    attr.componentType
            = static_cast<QSSGRenderGeometry::Attribute::ComponentType>(mattr.componentType);
    return attr;
}

void QSSGRenderGeometry::addAttribute(Attribute::Semantic semantic, int offset,
                                      Attribute::ComponentType componentType)
{
    Attribute attr;
    attr.semantic = semantic;
    attr.offset = offset;
    attr.componentType = componentType;
    addAttribute(attr);
}

void QSSGRenderGeometry::addAttribute(const Attribute &att)
{
    int index = m_meshData.m_attributeCount;
    m_meshData.m_attributes[index].semantic
            = static_cast<QSSGMeshUtilities::MeshData::Attribute::Semantic>(att.semantic);
    m_meshData.m_attributes[index].offset = att.offset;
    m_meshData.m_attributes[index].componentType
            = static_cast<QSSGMeshUtilities::MeshData::Attribute::ComponentType>(att.componentType);
    ++m_meshData.m_attributeCount;
    m_dirty = true;
}

void QSSGRenderGeometry::setStride(int stride)
{
    m_meshData.m_stride = stride;
    m_dirty = true;
}

void QSSGRenderGeometry::setPrimitiveType(PrimitiveType type)
{
    m_meshData.m_primitiveType
            = static_cast<QSSGMeshUtilities::MeshData::PrimitiveType>(type);
    m_dirty = true;
}

void QSSGRenderGeometry::setBounds(const QVector3D &min, const QVector3D &max)
{
    m_bounds = QSSGBounds3(min, max);
    m_dirty = true;
}

void QSSGRenderGeometry::clear()
{
    m_meshData.clear();
    m_bounds = QSSGBounds3::empty();
    m_dirty = true;
}

void QSSGRenderGeometry::clearAttributes()
{
    m_meshData.m_attributeCount = 0;
}

void QSSGRenderGeometry::setPath(const QString &path)
{
    m_meshPath = QSSGRenderMeshPath::create(path);
    m_dirty = true;
}

void QSSGRenderGeometry::setVertexData(const QByteArray &data)
{
    m_meshData.m_vertexBuffer = data;
    m_dirty = true;
}

void QSSGRenderGeometry::setIndexData(const QByteArray &data)
{
    m_meshData.m_indexBuffer = data;
    m_dirty = true;
}

QSSGRenderMesh *QSSGRenderGeometry::createOrUpdate(const QSSGRef<QSSGBufferManager> &bufferManager)
{
    if (!m_meshBuilder)
        m_meshBuilder = QSSGMeshUtilities::QSSGMeshBuilder::createMeshBuilder();

    if (m_dirty) {
        QString error;
        auto mesh = m_meshBuilder->buildMesh(m_meshData, error, m_bounds);
        bufferManager->loadCustomMesh(m_meshPath, mesh, true);
        m_meshBuilder->reset();
        m_dirty = false;
    }

    return bufferManager->loadMesh(m_meshPath);
}
