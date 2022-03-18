/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "heightfieldgeometry_p.h"

/*!
    \qmltype HeightFieldGeometry
    \inqmlmodule QtQuick3D
    \since 6.4
    \brief A height field geometry.

    This is a height-field geometry. The geometry is built from an image file where
    every pixel is converted into a point on the height map where the height is
    defined by the intensity of the pixel. The image's left-most, upper-most pixel
    will be placed at position (0,0,0). The image's x-axis and y-axis will go along
    the geometry's z-axis and x-axis respectively. The height of the field (y-axis)
    goes between -0.5..0.5 depending on the intesity at the pixel. Each pixel is
    distanced one (1) unit in the xz-plane from its adjacents.
*/

/*!
    \qmlproperty vector3d HeightFieldGeometry::heightFieldScale
    This property defines the geometric scaling vector for the height-field.
    The default value is (1, 1, 1).
*/

/*!
    \qmlproperty QUrl HeightFieldGeometry::heightMap
    This property defines the URL of the heightMap.
*/

HeightFieldGeometry::HeightFieldGeometry()
{
    updateData();
}

const QVector3D &HeightFieldGeometry::heightFieldScale() const
{
    return m_heightFieldScale;
}

void HeightFieldGeometry::setHeightFieldScale(const QVector3D &newHeightFieldScale)
{
    if (m_heightFieldScale == newHeightFieldScale)
        return;
    m_heightFieldScale = newHeightFieldScale;

    updateData();
    update();

    emit heightFieldScaleChanged();
}

const QUrl &HeightFieldGeometry::heightMap() const
{
    return m_heightMapSource;
}

void HeightFieldGeometry::setHeightMap(const QUrl &newHeightMap)
{
    if (m_heightMapSource == newHeightMap)
        return;
    m_heightMapSource = newHeightMap;

    updateData();
    update();

    emit heightMapChanged();
}

struct Vertex
{
    QVector3D position;
    QVector3D normal;
};

void HeightFieldGeometry::updateData()
{
    clear();
    const QQmlContext *context = qmlContext(this);

    const auto resolvedUrl = context ? context->resolvedUrl(m_heightMapSource) : m_heightMapSource;
    const auto qmlSource = QQmlFile::urlToLocalFileOrQrc(resolvedUrl);

    QImage heightMap(qmlSource);
    int numRows = heightMap.height();
    int numCols = heightMap.width();

    const int numVertices = numRows * numCols;
    if (numVertices == 0)
        return;

    QVector<Vertex> vertices;
    vertices.reserve(numVertices);

    const auto scale = heightFieldScale();
    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            float f = heightMap.pixelColor(x, y).valueF() - 0.5;
            Vertex vertex;
            vertex.position = QVector3D(y, f, x) * scale;
            vertex.normal = QVector3D(0, 0, 0);
            vertices.push_back(vertex);
        }
    }

    QVector<quint32> indices;
    for (int iy = 0; iy < numRows - 1; ++iy) {
        for (int ix = 0; ix < numCols - 1; ++ix) {
            const int idx = iy * numCols + ix;

            const auto tri0 = std::array<int, 3> { idx + numCols + 1, idx + numCols, idx };
            const auto tri1 = std::array<int, 3> { idx + 1, idx + numCols + 1, idx };

            for (const auto [i0, i1, i2] : { tri0, tri1 }) {
                indices.push_back(i0);
                indices.push_back(i1);
                indices.push_back(i2);

                // Calculate face normal
                const QVector3D e0 = vertices[i1].position - vertices[i0].position;
                const QVector3D e1 = vertices[i2].position - vertices[i0].position;
                QVector3D normal = QVector3D::crossProduct(e0, e1).normalized();

                // Add normal to vertex, will normalize later
                vertices[i0].normal += normal;
                vertices[i1].normal += normal;
                vertices[i2].normal += normal;
            }
        }
    }

    // Normalize
    for (auto &vertex : vertices)
        vertex.normal.normalize();

    // Calculate bounds
    QVector3D boundsMin = vertices[0].position;
    QVector3D boundsMax = vertices[0].position;

    for (const auto &vertex : vertices) {
        const auto &p = vertex.position;
        boundsMin = QVector3D(qMin(boundsMin.x(), p.x()), qMin(boundsMin.y(), p.y()), qMin(boundsMin.z(), p.z()));
        boundsMax = QVector3D(qMax(boundsMax.x(), p.x()), qMax(boundsMax.y(), p.y()), qMax(boundsMax.z(), p.z()));
    }

    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic, 0, QQuick3DGeometry::Attribute::F32Type);

    addAttribute(QQuick3DGeometry::Attribute::NormalSemantic, sizeof(QVector3D), QQuick3DGeometry::Attribute::F32Type);

    addAttribute(QQuick3DGeometry::Attribute::IndexSemantic, 0, QQuick3DGeometry::Attribute::ComponentType::U32Type);

    setStride(sizeof(Vertex));
    QByteArray vertexBuffer(reinterpret_cast<char *>(vertices.data()), vertices.size() * sizeof(Vertex));
    setVertexData(vertexBuffer);
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
    setBounds(boundsMin, boundsMax);

    QByteArray indexBuffer(reinterpret_cast<char *>(indices.data()), indices.size() * sizeof(quint32));
    setIndexData(indexBuffer);
}
