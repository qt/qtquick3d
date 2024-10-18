// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2016 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "conegeometry_p.h"
#include <limits>

#if QT_CONFIG(concurrent)
#include <QtConcurrentRun>
#endif

QT_BEGIN_NAMESPACE

namespace {

void createConeSidesVertices(float *&verticesPtr,
                             int rings,
                             int slices,
                             double topRadius,
                             double bottomRadius,
                             double length)
{
    const float dY = length / static_cast<float>(rings - 1);
    const float dTheta = (M_PI * 2) / static_cast<float>(slices);

    // Determine the cone type
    bool isTruncatedCone = (topRadius > 0 && bottomRadius > 0);
    bool isUpwardCone = (topRadius > 0 && bottomRadius == 0);
    bool isDownwardCone = (topRadius == 0 && bottomRadius > 0);

    for (int ring = 0; ring < rings; ++ring) {
        const float y = -length / 2.0f + static_cast<float>(ring) * dY;

        const float t = (y + length / 2) / length;
        const float radius = (bottomRadius * (1 - t)) + (t * topRadius);

        for (int slice = 0; slice <= slices; ++slice) {
            const float theta = static_cast<float>(slice) * dTheta;
            const float ta = std::tan((M_PI/2) - std::atan(length / (bottomRadius - topRadius)));
            const float ct = std::cos(theta);
            const float st = std::sin(theta);

            // Position
            *verticesPtr++ = radius * ct;
            *verticesPtr++ = y;
            *verticesPtr++ = radius * st;

            // UV Coordinates
            if (isTruncatedCone) {
                // Case 1: Truncated Cone
                // Map 'y' to V coordinate range [0.5, 1] (top to bottom of truncated cone)
                float v = 0.5f + (y + length / 2.0f) / length / 2.0f;
                *verticesPtr++ = static_cast<float>(slice) / static_cast<float>(slices);  // U-coordinate (theta / 2π)
                *verticesPtr++ = v;  // V-coordinate mapped between [0.5, 1]

            } else if (isDownwardCone) {
                // Case 2: Downward Facing Cone
                // The top ring (ring 0) should be at UV coordinate (0.25, 0.25)
                // We want to map the top ring to a circle around this point
                float rUV = 1.0f - static_cast<float>(ring) / static_cast<float>(rings - 1);
                float u = 0.25f + rUV * ct * 0.25f;  // Mapping to a circle around (0.25, 0.25)
                float v = 0.25f + -rUV * st * 0.25f;
                *verticesPtr++ = u;
                *verticesPtr++ = v;

            } else if (isUpwardCone) {
                // Case 3: Upward Facing Cone
                // The bottom ring (ring 0) should be at UV coordinate (0.75, 0.25)
                // We want to map the bottom ring to a circle around this point
                float rUV = static_cast<float>(ring) / static_cast<float>(rings - 1);  // Normalized distance from center
                float u = 0.75f + rUV * ct * 0.25f;  // Mapping to a circle around (0.75, 0.25)
                float v = 0.25f + rUV * st * 0.25f;
                *verticesPtr++ = u;
                *verticesPtr++ = v;
            }

            // Normal
            QVector3D n(ct, ta, st);
            n.normalize();
            *verticesPtr++ = n.x();
            *verticesPtr++ = n.y();
            *verticesPtr++ = n.z();
        }
    }
}

void createConeSidesIndices(quint16 *&indicesPtr, int rings, int slices)
{
    for (int ring = 0; ring < rings-1; ++ring) {
        const int ringIndexStart = ring * (slices + 1);
        const int nextRingIndexStart = (ring + 1) * (slices + 1);

        for (int slice = 0; slice <= slices; ++slice) {
            if (slice == slices)
                continue;

            const int nextSlice = slice + 1;

            *indicesPtr++ = (ringIndexStart + slice);
            *indicesPtr++ = (nextRingIndexStart + slice);
            *indicesPtr++ = (ringIndexStart + nextSlice);
            *indicesPtr++ = (ringIndexStart + nextSlice);
            *indicesPtr++ = (nextRingIndexStart + slice);
            *indicesPtr++ = (nextRingIndexStart + nextSlice);
        }
    }
}

void createConeDiscVertices(float *&verticesPtr,
                             int slices,
                             double topRadius,
                             double bottomRadius,
                             double length,
                             double yPosition)
{
    const float dTheta = (M_PI * 2) / static_cast<float>(slices);
    const double yNormal = (yPosition < 0.0f) ? -1.0f : 1.0f;

    *verticesPtr++ = 0.0f;
    *verticesPtr++ = yPosition;
    *verticesPtr++ = 0.0f;

    if (yPosition < 0.0f) {
        // Bottom Cap
        *verticesPtr++ = 0.75f;  // Center vertex UV (bottom cap)
        *verticesPtr++ = 0.25f;
    } else {
        // Top Cap
        *verticesPtr++ = 0.25f;  // Center vertex UV (top cap)
        *verticesPtr++ = 0.25f;
    }


    *verticesPtr++ = 0.0f;
    *verticesPtr++ = yNormal;
    *verticesPtr++ = 0.0f;


    for (int slice = 0; slice <= slices; ++slice)
    {
        const float theta = static_cast<float>(slice) * dTheta;
        const float ct = std::cos(theta);
        const float st = std::sin(theta);

        const float t = (yPosition + length / 2) / length;
        const float radius = (bottomRadius * (1 - t)) + (t * topRadius);

        // Position
        *verticesPtr++ = radius * ct;
        *verticesPtr++ = yPosition;
        *verticesPtr++ = radius * st;

        // UV Coordinates
        if (yPosition < 0.0f) {
            // Bottom cap UVs: Map to range (0, 0.5) to (1, 1)
            *verticesPtr++ = 0.75f + 0.25f * ct;
            *verticesPtr++ = 0.25f + 0.25f * st;
        } else {
            // Top cap UVs: Map to range (0, 0.5)
            *verticesPtr++ = 0.25f + 0.25f * ct;
            *verticesPtr++ = 0.25f + 0.25f * -st;
        }

        // Normal
        *verticesPtr++ = 0.0f;
        *verticesPtr++ = yNormal;
        *verticesPtr++ = 0.0f;
    }
}

void createDiscIndices(quint16 *&indicesPtr,
                       int discCenterIndex,
                       int slices,
                       bool isTopCap)
{
    if ( !isTopCap ) {
        for ( int i = slices - 1 ; i >= 0 ; --i )
        {
            if ( i != 0 ) {
                *indicesPtr++ = discCenterIndex;
                *indicesPtr++ = discCenterIndex + i + 1;
                *indicesPtr++ = discCenterIndex + i;
            } else {
                *indicesPtr++ = discCenterIndex;
                *indicesPtr++ = discCenterIndex + i + 1;
                *indicesPtr++ = discCenterIndex + slices;
            }
        }
    } else {
        for ( int i = 0 ; i < slices; ++i )
        {
            if ( i != slices - 1 ) {
                *indicesPtr++ = discCenterIndex;
                *indicesPtr++ = discCenterIndex + i + 1;
                *indicesPtr++ = discCenterIndex + i + 2;
            } else {
                *indicesPtr++ = discCenterIndex;
                *indicesPtr++ = discCenterIndex + i + 1;
                *indicesPtr++ = discCenterIndex + 1;
            }
        }
    }
}

}

/*!
    \qmltype ConeGeometry
    \inqmlmodule QtQuick3D.Helpers
    \inherits Geometry
    \since 6.9
    \brief Provides geometry for a cone.

    ConeGeometry is a geometry type that represents a cone. The cone's size is
    defined by its top radius, bottom radius, and length. The topology of the
    cone is defined by the number of rings and segments.
*/

/*!
    \qmlproperty float ConeGeometry::topRadius
    This property holds the top radius of the cone. This value must be 0 or greater.

    \note topRadius and bottomRadius can not both be \c 0.0
*/

/*!
    \qmlproperty float ConeGeometry::bottomRadius
    This property holds the bottom radius of the cone.

    \note topRadius and bottomRadius can not both be \c 0.0
*/

/*!
    \qmlproperty float ConeGeometry::length
    This property holds the length of the cone. This value must be greater than 0 to
    generate a cone.
*/

/*!
    \qmlproperty int ConeGeometry::rings
    This property holds the number of rings of the cone, which are vertical subdivisions
    of the cone. The minimum value is 0, which means the cone has no vertical subdivisions.
*/

/*!
    \qmlproperty int ConeGeometry::segments
    This property holds the number of segments of the cone.  The minimum value is 3, and any value
    lower will not generate any geometry.
*/

/*!
    \qmlproperty bool ConeGeometry::asynchronous

    This property holds whether the geometry generation should be asynchronous.
*/

/*!
    \qmlproperty bool ConeGeometry::status
    \readonly

    This property holds the status of the geometry generation when asynchronous is true.

    \value ConeGeometry.Null The geometry generation has not started
    \value ConeGeometry.Ready The geometry generation is complete.
    \value ConeGeometry.Loading The geometry generation is in progress.
    \value ConeGeometry.Error The geometry generation failed.
*/

ConeGeometry::ConeGeometry(QQuick3DObject *parent)
    : QQuick3DGeometry(parent)
{
#if QT_CONFIG(concurrent)
    connect(&m_geometryDataWatcher, &QFutureWatcher<GeometryData>::finished, this, &ConeGeometry::requestFinished);
#endif
    scheduleGeometryUpdate();
}

ConeGeometry::~ConeGeometry()
{

}

float ConeGeometry::topRadius() const
{
    return m_topRadius;
}

void ConeGeometry::setTopRadius(float newTopRadius)
{
    if (qFuzzyCompare(m_topRadius, newTopRadius))
        return;
    m_topRadius = newTopRadius;
    emit topRadiusChanged();
    scheduleGeometryUpdate();
}

float ConeGeometry::bottomRadius() const
{
    return m_bottomRadius;
}

void ConeGeometry::setBottomRadius(float newBottomRadius)
{
    if (qFuzzyCompare(m_bottomRadius, newBottomRadius))
        return;
    m_bottomRadius = newBottomRadius;
    emit bottomRadiusChanged();
    scheduleGeometryUpdate();
}

float ConeGeometry::length() const
{
    return m_length;
}

void ConeGeometry::setLength(float newLength)
{
    if (qFuzzyCompare(m_length, newLength))
        return;
    m_length = newLength;
    emit lengthChanged();
    scheduleGeometryUpdate();
}

int ConeGeometry::rings() const
{
    return m_rings;
}

void ConeGeometry::setRings(int newRings)
{
    if (m_rings == newRings)
        return;
    m_rings = newRings;
    emit ringsChanged();
    scheduleGeometryUpdate();
}

int ConeGeometry::segments() const
{
    return m_segments;
}

void ConeGeometry::setSegments(int newSegments)
{
    if (m_segments == newSegments)
        return;
    m_segments = newSegments;
    emit segmentsChanged();
    scheduleGeometryUpdate();
}

bool ConeGeometry::asynchronous() const
{
    return m_asynchronous;
}

void ConeGeometry::setAsynchronous(bool newAsynchronous)
{
    if (m_asynchronous == newAsynchronous)
        return;
    m_asynchronous = newAsynchronous;
    emit asynchronousChanged();
}

ConeGeometry::Status ConeGeometry::status() const
{
    return m_status;
}

void ConeGeometry::doUpdateGeometry()
{
    // reset the flag since we are processing the update
    m_geometryUpdateRequested = false;

#if QT_CONFIG(concurrent)
    if (m_geometryDataFuture.isRunning()) {
        m_pendingAsyncUpdate = true;
        return;
    }
#endif

    // Check validity of the geometry parameters
    if (m_topRadius < 0 || m_bottomRadius < 0 || (m_topRadius <= 0 && m_bottomRadius <= 0) || m_length <= 0 || m_rings < 0 || m_segments < 3) {
        clear();
        update();
        return;
    }

#if QT_CONFIG(concurrent)
    if (m_asynchronous) {
        m_geometryDataFuture = QtConcurrent::run(generateConeGeometryAsync,
                                                 m_topRadius,
                                                 m_bottomRadius,
                                                 m_length,
                                                 m_rings,
                                                 m_segments);
        m_geometryDataWatcher.setFuture(m_geometryDataFuture);
        m_status = Status::Loading;
        Q_EMIT statusChanged();
    } else {
#else
    {

#endif // QT_CONFIG(concurrent)
        updateGeometry(generateConeGeometry(m_topRadius, m_bottomRadius, m_length, m_rings, m_segments));
    }
}

void ConeGeometry::requestFinished()
{
#if QT_CONFIG(concurrent)
    const auto output = m_geometryDataFuture.takeResult();
    updateGeometry(output);
#endif
}

void ConeGeometry::scheduleGeometryUpdate()
{
    if (!m_geometryUpdateRequested) {
        QMetaObject::invokeMethod(this, "doUpdateGeometry", Qt::QueuedConnection);
        m_geometryUpdateRequested = true;
    }
}

void ConeGeometry::updateGeometry(const GeometryData &geometryData)
{
    setStride(sizeof(float) * 8); // 3 for position, 2 for uv0, 3 for normal
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,
                 0,
                 QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::TexCoord0Semantic,
                 3 * sizeof(float),
                 QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::NormalSemantic,
                 5 * sizeof(float),
                 QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::IndexSemantic,
                 0,
                 QQuick3DGeometry::Attribute::U16Type);

    setBounds(geometryData.boundsMin, geometryData.boundsMax);
    setVertexData(geometryData.vertexData);
    setIndexData(geometryData.indexData);

           // If the geometry update was requested while the geometry was being generated asynchronously,
           // we need to schedule another geometry update now that the geometry is ready.
    if (m_pendingAsyncUpdate) {
        m_pendingAsyncUpdate = false;
        scheduleGeometryUpdate();
    } else {
        m_status = Status::Ready;
        Q_EMIT statusChanged();
    }
    update();
}

ConeGeometry::GeometryData ConeGeometry::generateConeGeometry(float topRadius, float bottomRadius, float length, int rings, int slices)
{
    GeometryData geomData;

    rings += 2; // Add two extra rings for the top and bottom caps

    // Cap count: if top and/or bottom radius is greater than zero
    int capCount = (topRadius > 0 ? 1 : 0) + (bottomRadius > 0 ? 1 : 0);

    // Compute total number of vertices and indices
    int totalFaces = (slices * 2) * (rings - 1) + slices * capCount;
    int totalVertices = (slices + 1) * rings + capCount * (slices + 2);
    int totalIndices = totalFaces * 3;

    // Resize QByteArray to hold vertex and index data
    geomData.vertexData.resize(totalVertices * 8 * sizeof(float));  // Each vertex has 8 attributes (3 position, 2 UV, 3 normal)
    geomData.indexData.resize(totalIndices * sizeof(quint16));      // Each index is a 16-bit unsigned integer

    // Get pointers to raw data in QByteArray
    float* verticesPtr = reinterpret_cast<float*>(geomData.vertexData.data());
    quint16* indicesPtr = reinterpret_cast<quint16*>(geomData.indexData.data());

    // Truncated Cone
    if (bottomRadius > 0 && topRadius > 0) {
        createConeSidesVertices(verticesPtr, rings, slices, topRadius, bottomRadius, length);
        createConeSidesIndices(indicesPtr, rings, slices);
        int bottomCenterIndex = rings * (slices + 1);
        createConeDiscVertices(verticesPtr, slices, topRadius, bottomRadius, length, -length / 2);
        createDiscIndices(indicesPtr, bottomCenterIndex, slices, true);
        int topCenterIndex = bottomRadius > 0 ? rings * (slices + 1) + (slices + 2) : rings * (slices + 1);
        createConeDiscVertices(verticesPtr, slices, topRadius, bottomRadius, length, length / 2);
        createDiscIndices(indicesPtr, topCenterIndex, slices, false);
    } else {
        // Cone Facing up
        if (topRadius > 0) {
            // Create the Cone Shape
            createConeSidesVertices(verticesPtr, rings, slices, topRadius, bottomRadius, length);
            createConeSidesIndices(indicesPtr, rings, slices);

            // Create flat disk at the bottom of the cone
            int topCenterIndex = rings * (slices + 1);
            createConeDiscVertices(verticesPtr, slices, topRadius, bottomRadius, length, length / 2);
            createDiscIndices(indicesPtr, topCenterIndex, slices, false);
        }

        // Cone Facing down
        if (bottomRadius > 0) {
            // Create the Cone shape
            createConeSidesVertices(verticesPtr, rings, slices, topRadius, bottomRadius, length);
            createConeSidesIndices(indicesPtr, rings, slices);

            // Create flat disk at the top of the cone
            int bottomCenterIndex = rings * (slices + 1);
            createConeDiscVertices(verticesPtr, slices, topRadius, bottomRadius, length, -length / 2);
            createDiscIndices(indicesPtr, bottomCenterIndex, slices, true);
        }
    }

    // Calculate bounding box (min and max)
    float* vertexData = reinterpret_cast<float*>(geomData.vertexData.data());
    QVector3D boundsMin(std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max());
    QVector3D boundsMax(std::numeric_limits<float>::lowest(),
                        std::numeric_limits<float>::lowest(),
                        std::numeric_limits<float>::lowest());
    for (int i = 0; i < totalVertices; ++i) {
        QVector3D pos(vertexData[i * 8], vertexData[i * 8 + 1], vertexData[i * 8 + 2]);
        boundsMin.setX(qMin(boundsMin.x(), pos.x()));
        boundsMin.setY(qMin(boundsMin.y(), pos.y()));
        boundsMin.setZ(qMin(boundsMin.z(), pos.z()));

        boundsMax.setX(qMax(boundsMax.x(), pos.x()));
        boundsMax.setY(qMax(boundsMax.y(), pos.y()));
        boundsMax.setZ(qMax(boundsMax.z(), pos.z()));
    }
    geomData.boundsMin = boundsMin;
    geomData.boundsMax = boundsMax;

    return geomData;
}

#if QT_CONFIG(concurrent)
void ConeGeometry::generateConeGeometryAsync(QPromise<ConeGeometry::GeometryData> &promise,
                                             float topRadius,
                                             float bottomRadius,
                                             float length,
                                             int rings,
                                             int segments)
{
    auto output = generateConeGeometry(topRadius, bottomRadius, length, rings, segments);
    promise.addResult(output);
}
#endif

QT_END_NAMESPACE
