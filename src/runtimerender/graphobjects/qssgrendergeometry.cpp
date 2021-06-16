/****************************************************************************
**
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

QSSGMesh::Mesh::DrawMode QSSGRenderGeometry::primitiveType() const
{
    return m_meshData.m_primitiveType;
}

QSSGRenderGeometry::Attribute QSSGRenderGeometry::attribute(int idx) const
{
    Attribute attr;
    const auto &mattr = m_meshData.m_attributes[idx];
    attr.offset = mattr.offset;
    attr.semantic = mattr.semantic;
    attr.componentType = mattr.componentType;
    return attr;
}

void QSSGRenderGeometry::addAttribute(QSSGMesh::RuntimeMeshData::Attribute::Semantic semantic,
                                      int offset,
                                      QSSGMesh::Mesh::ComponentType componentType)
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
    if (index == QSSGMesh::RuntimeMeshData::MAX_ATTRIBUTES) {
        qWarning("Maximum number (%d) of vertex attributes in custom geometry has been reached; ignoring extra attributes",
                 QSSGMesh::RuntimeMeshData::MAX_ATTRIBUTES);
        return;
    }
    m_meshData.m_attributes[index].semantic
            = static_cast<QSSGMesh::RuntimeMeshData::Attribute::Semantic>(att.semantic);
    m_meshData.m_attributes[index].offset = att.offset;
    m_meshData.m_attributes[index].componentType = att.componentType;
    ++m_meshData.m_attributeCount;
    m_dirty = true;
}

void QSSGRenderGeometry::addSubset(quint32 offset, quint32 count, const QVector3D &boundsMin, const QVector3D &boundsMax, const QString &name)
{
    m_meshData.m_subsets.append({name, {boundsMin, boundsMax}, count, offset, {}});
}

void QSSGRenderGeometry::setStride(int stride)
{
    m_meshData.m_stride = stride;
    m_dirty = true;
}

void QSSGRenderGeometry::setPrimitiveType(QSSGMesh::Mesh::DrawMode type)
{
    m_meshData.m_primitiveType = type;
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
    m_bounds.setEmpty();
    m_dirty = true;
}

void QSSGRenderGeometry::clearAttributes()
{
    m_meshData.m_attributeCount = 0;
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
    if (m_dirty) {
        QSSGRenderMesh *renderMesh = nullptr;
        QString error;
        QSSGMesh::Mesh mesh = QSSGMesh::Mesh::fromRuntimeData(m_meshData, &error);
        if (mesh.isValid())
            renderMesh = bufferManager->loadCustomMesh(this, mesh, true);
        else
            qWarning("Mesh building failed: %s", qPrintable(error));

        m_dirty = false;
        return renderMesh;
    }

    return bufferManager->getMesh(this);
}
