/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qquick3dparticleshape_p.h"
#include "qquick3dparticlerandomizer_p.h"
#include "qquick3dparticlesystem_p.h"
#include "qquick3dparticleutils_p.h"
#include <QtCore/qdir.h>
#include <QtQml/qqmlfile.h>
#include <QtQuick3D/private/qquick3dobject_p.h>
#include <QtQuick3D/private/qquick3dmodel_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <algorithm>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ParticleShape3D
    \inherits QtObject
    \inqmlmodule QtQuick3D.Particles3D
    \brief Offers 3D shapes for emitters and affectors.
    \since 6.2

    The ParticleShape3D element supports shapes like \c Cube, \c Sphere and \c Cylinder for particles needs.
    For example, emitter can use \l {ParticleEmitter3D::shape}{shape} property to emit particles from the
    shape area.

    Shapes don't have position, scale or rotation. Instead, they use parent node for these properties.
*/

QQuick3DParticleShape::QQuick3DParticleShape(QObject *parent)
    : QObject(parent)
{
    m_parentNode = qobject_cast<QQuick3DNode *>(parent);
}

QQuick3DParticleShape::~QQuick3DParticleShape()
{
    delete m_model;
}

/*!
    \qmlproperty bool ParticleShape3D::fill

    This property defines if the shape should be filled or just use the shape outlines.

    The default value is \c true.
*/
bool QQuick3DParticleShape::fill() const
{
    return m_fill;
}

void QQuick3DParticleShape::setFill(bool fill)
{
    if (m_fill == fill)
        return;

    m_fill = fill;
    Q_EMIT fillChanged();
}

/*!
    \qmlproperty ShapeType ParticleShape3D::type

    This property defines the type of the shape.

    The default value is \c ParticleShape3D.Cube.
*/

/*!
    \qmlproperty enumeration ParticleShape3D::ShapeType

    Defines the type of the shape.

    \value ParticleShape3D.Cube
        Cube shape.
    \value ParticleShape3D.Sphere
        Sphere shape.
    \value ParticleShape3D.Cylinder
        Cylinder shape.
*/

QQuick3DParticleShape::ShapeType QQuick3DParticleShape::type() const
{
    return m_type;
}

/*!
    \qmlproperty vector3d ParticleShape3D::extents

    This property defines the extents of the shape.

    The default value for each axis is \c 50.
*/
QVector3D QQuick3DParticleShape::extents() const
{
    return m_extents;
}

/*!
    \qmlproperty Component ParticleShape::delegate
    \preliminary
    The delegate provides a template defining the model for the ParticleShape.
    For example, using the default sphere model with default material
    \qml
    Component {
        id: modelComponent
        Model {
            source: "#Sphere"
            scale: Qt.vector3d(0.5, 0.5, 0.5)
            materials: DefaultMaterial { diffuseColor: "red" }
        }
    }
    ParticleShape {
        delegate: modelComponent
    }
    \endqml
*/
QQmlComponent *QQuick3DParticleShape::delegate() const
{
    return m_delegate;
}

void QQuick3DParticleShape::setType(QQuick3DParticleShape::ShapeType type)
{
    if (m_type == type)
        return;

    m_type = type;
    Q_EMIT typeChanged();
}

void QQuick3DParticleShape::setExtents(QVector3D extents)
{
    if (m_extents == extents)
        return;

    m_extents = extents;
    Q_EMIT extentsChanged();
}

static QSSGMesh::Mesh loadMesh(const QString &source)
{
    QString src = source;
    if (source.startsWith(QLatin1Char('#'))) {
        src = QSSGBufferManager::primitivePath(source);
        src.prepend(QLatin1String(":/"));
    }
    src = QDir::cleanPath(src);
    if (src.startsWith(QLatin1String("qrc:/")))
        src = src.mid(3);
    QSSGMesh::Mesh mesh;
    QFileInfo fileInfo = QFileInfo(src);
    if (fileInfo.exists()) {
        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QFile::ReadOnly))
            return {};
        mesh = QSSGMesh::Mesh::loadMesh(&file);
    }
    return mesh;
}

void QQuick3DParticleShape::setDelegate(QQmlComponent *delegate)
{
    if (delegate == m_delegate)
        return;
    m_delegate = delegate;
    clearModelVertexPositions();
    createModel();
    Q_EMIT delegateChanged();
}

void QQuick3DParticleShape::createModel()
{
    delete m_model;
    m_model = nullptr;
    if (!m_delegate)
        return;
    auto *obj = m_delegate->create(m_delegate->creationContext());
    m_model = qobject_cast<QQuick3DModel *>(obj);
    if (!m_model)
        delete obj;
}

void QQuick3DParticleShape::componentComplete()
{
    m_parentNode = qobject_cast<QQuick3DNode *>(parent());
    if (!m_parentNode)
        qWarning() << "Shape requires parent Node to function correctly!";
}

QVector3D QQuick3DParticleShape::randomPosition(int particleIndex)
{
    if (!m_parentNode || !m_system)
        return QVector3D();

    if (m_model)
        return randomPositionModel(particleIndex);

    switch (m_type) {
    case QQuick3DParticleShape::ShapeType::Cube:
        return randomPositionCube(particleIndex);
    case QQuick3DParticleShape::ShapeType::Sphere:
        return randomPositionSphere(particleIndex);
    case QQuick3DParticleShape::ShapeType::Cylinder:
        return randomPositionCylinder(particleIndex);
    default:
        Q_ASSERT(false);
    }
    return QVector3D();
}

QVector3D QQuick3DParticleShape::randomPositionCube(int particleIndex) const
{
    auto rand = m_system->rand();
    QVector3D s = m_parentNode->scale() * m_extents;
    float x = s.x() - (rand->get(particleIndex, QPRand::Shape1) * s.x() * 2.0f);
    float y = s.y() - (rand->get(particleIndex, QPRand::Shape2) * s.y() * 2.0f);
    float z = s.z() - (rand->get(particleIndex, QPRand::Shape3) * s.z() * 2.0f);
    if (!m_fill) {
        // Random 0..5 for cube sides
        int side = int(rand->get(particleIndex, QPRand::Shape4) * 6);
        if (side == 0)
            x = -s.x();
        else if (side == 1)
            x = s.x();
        else if (side == 2)
            y = -s.y();
        else if (side == 3)
            y = s.y();
        else if (side == 4)
            z = -s.z();
        else
            z = s.z();
    }
    QMatrix4x4 mat;
    mat.rotate(m_parentNode->rotation());
    return mat.mapVector(QVector3D(x, y, z));
}

QVector3D QQuick3DParticleShape::randomPositionSphere(int particleIndex) const
{
    auto rand = m_system->rand();
    QVector3D scale = m_parentNode->scale() * m_extents;
    float theta = rand->get(particleIndex, QPRand::Shape1) * float(M_PI) * 2.0f;
    float v = rand->get(particleIndex, QPRand::Shape2);
    float phi = acos((2.0f * v) - 1.0f);
    float r = m_fill ? pow(rand->get(particleIndex, QPRand::Shape3), 1.0f / 3.0f) : 1.0f;
    float x = r * QPSIN(phi) * QPCOS(theta);
    float y = r * QPSIN(phi) * QPSIN(theta);
    float z = r * QPCOS(phi);
    QVector3D pos(x, y, z);
    pos *= scale;
    QMatrix4x4 mat;
    mat.rotate(m_parentNode->rotation());
    return mat.mapVector(pos);
}

QVector3D QQuick3DParticleShape::randomPositionCylinder(int particleIndex) const
{
    auto rand = m_system->rand();
    QVector3D scale = m_parentNode->scale() * m_extents;
    float y = scale.y() - (rand->get(particleIndex, QPRand::Shape1) * scale.y() * 2.0f);
    float r = 1.0f;
    if (m_fill)
        r = sqrt(rand->get(particleIndex, QPRand::Shape2));
    float theta = rand->get(particleIndex, QPRand::Shape3) * float(M_PI) * 2.0f;
    float x = r * QPCOS(theta);
    float z = r * QPSIN(theta);
    x = x * scale.x();
    z = z * scale.z();
    QVector3D pos(x, y, z);
    QMatrix4x4 mat;
    mat.rotate(m_parentNode->rotation());
    return mat.mapVector(pos);
}

QVector3D QQuick3DParticleShape::randomPositionModel(int particleIndex)
{
    if (m_model) {
        calculateModelVertexPositions();

        const QVector<QVector3D> &positions = m_vertexPositions;
        if (positions.size() > 0) {
            auto rand = m_system->rand();

            // Calculate model triangle areas so that the random triangle selection can be weighted
            // by the area. This way particles are uniformly emitted from the whole model.
            if (m_modelTriangleAreas.size() == 0) {
                m_modelTriangleAreas.reserve(positions.size() / 3);
                for (int i = 0; i + 2 < positions.size(); i += 3) {
                    const QVector3D &v1 = positions[i];
                    const QVector3D &v2 = positions[i + 1];
                    const QVector3D &v3 = positions[i + 2];
                    const float area = QVector3D::crossProduct(v1 - v2, v1 - v3).length() * 0.5f;
                    m_modelTriangleAreasSum += area;
                    m_modelTriangleAreas.append(m_modelTriangleAreasSum);
                    m_modelTriangleCenter += v1 + v2 + v3;
                }
                m_modelTriangleCenter /= positions.size();
            }

            const float rndWeight = rand->get(particleIndex, QPRand::Shape1) * m_modelTriangleAreasSum;

            // Use binary search to find the weighted random index
            int index = std::lower_bound(m_modelTriangleAreas.begin(), m_modelTriangleAreas.end(), rndWeight) - m_modelTriangleAreas.begin();

            const QVector3D &v1 = positions[index * 3];
            const QVector3D &v2 = positions[index * 3 + 1];
            const QVector3D &v3 = positions[index * 3 + 2];
            const float a = rand->get(particleIndex, QPRand::Shape2);
            const float b = rand->get(particleIndex, QPRand::Shape3);
            const float aSqrt = qSqrt(a);

            // Calculate a random point from the selected triangle
            QVector3D pos = (1.0 - aSqrt) * v1 + (aSqrt * (1.0 - b)) * v2 + (b * aSqrt) * v3;

            if (m_fill) {
                // The model is filled by selecting a random point between a random surface point
                // and the center of the model. The random point selection is exponentially weighted
                // towards the surface so that particles aren't clustered in the center.
                const float uniform = rand->get(particleIndex, QPRand::Shape4);
                const float lambda = 5.0f;
                const float alpha = -qLn(1 - (1 - qExp(-lambda)) * uniform) / lambda;
                pos += (m_modelTriangleCenter - pos) * alpha;
            }

            QMatrix4x4 mat;
            mat.rotate(m_parentNode->rotation() * m_model->rotation());
            return mat.mapVector(pos * m_parentNode->sceneScale() * m_model->scale());
        }
    }
    return QVector3D(0, 0, 0);
}

void QQuick3DParticleShape::clearModelVertexPositions()
{
    m_vertexPositions.clear();
    m_modelTriangleAreas.clear();
    m_modelTriangleAreasSum = 0;
}

void QQuick3DParticleShape::calculateModelVertexPositions()
{
    if (m_vertexPositions.empty()) {
        QVector<QVector3D> indicedPositions;
        QVector<QVector3D> positions;

        if (m_model->geometry()) {
            QQuick3DGeometry *geometry = m_model->geometry();
            bool hasIndexBuffer = false;
            QQuick3DGeometry::Attribute::ComponentType indexBufferFormat;
            int posOffset = 0;
            QQuick3DGeometry::Attribute::ComponentType posType;
            for (int i = 0; i < geometry->attributeCount(); ++i) {
                auto attribute = geometry->attribute(i);
                if (attribute.semantic == QQuick3DGeometry::Attribute::PositionSemantic) {
                    posOffset = attribute.offset;
                    posType = attribute.componentType;
                } else if (attribute.semantic == QQuick3DGeometry::Attribute::IndexSemantic) {
                    hasIndexBuffer = true;
                    indexBufferFormat = attribute.componentType;
                }
            }
            if (posType == QQuick3DGeometry::Attribute::F32Type) {
                const auto &data = geometry->vertexData();
                int stride = geometry->stride();
                for (int i = 0; i < data.size(); i += stride) {
                    float v[3];
                    memcpy(v, data + posOffset + i, sizeof(v));
                    positions.append(QVector3D(v[0], v[1], v[2]));
                }
            }
            if (hasIndexBuffer) {
                const auto &data = geometry->vertexData();
                int indexSize = 4;
                if (indexBufferFormat == QQuick3DGeometry::Attribute::U16Type)
                    indexSize = 2;
                for (int i = 0; i < data.size(); i += indexSize) {
                    quint64 index = 0;
                    memcpy(&index, data + i, indexSize);
                    indicedPositions.append(positions[index]);
                }
            }
        } else {
                const QQmlContext *context = qmlContext(this);
                QString src = m_model->source().toString();
                if (context && !src.startsWith(QLatin1Char('#')))
                    src = QQmlFile::urlToLocalFileOrQrc(context->resolvedUrl(m_model->source()));
                QSSGMesh::Mesh mesh = loadMesh(src);
                if (!mesh.isValid())
                    return;
                if (mesh.drawMode() != QSSGMesh::Mesh::DrawMode::Triangles)
                    return;

                auto entries = mesh.vertexBuffer().entries;
                int posOffset = 0;
                int posCount = 0;
                QSSGMesh::Mesh::ComponentType posType;
                for (int i = 0; i < entries.size(); ++i) {
                    const char *nameStr = entries[i].name.constData();
                    if (!strcmp(nameStr, QSSGMesh::MeshInternal::getPositionAttrName())) {
                        posOffset = entries[i].offset;
                        posCount = entries[i].componentCount;
                        posType = entries[i].componentType;
                        break;
                    }
                }
                if (posCount == 3 && posType == QSSGMesh::Mesh::ComponentType::Float32) {
                    const auto &data = mesh.vertexBuffer().data;
                    int stride = mesh.vertexBuffer().stride;
                    for (int i = 0; i < data.size(); i += stride) {
                        float v[3];
                        memcpy(v, data + posOffset + i, sizeof(v));
                        positions.append(QVector3D(v[0], v[1], v[2]));
                    }
                }
                const auto &data = mesh.indexBuffer().data;
                int indexSize = QSSGMesh::MeshInternal::byteSizeForComponentType(mesh.indexBuffer().componentType);
                for (int i = 0; i < data.size(); i += indexSize) {
                    quint64 index = 0;
                    memcpy(&index, data + i, indexSize);
                    indicedPositions.append(positions[index]);
                }
        }
        if (!indicedPositions.empty())
            m_vertexPositions = indicedPositions;
        else
            m_vertexPositions = positions;
    }
}

QT_END_NAMESPACE
