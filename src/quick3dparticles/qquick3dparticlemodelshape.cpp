// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dparticlemodelshape_p.h"
#include "qquick3dparticlerandomizer_p.h"
#include "qquick3dparticlesystem_p.h"
#include <QtCore/qdir.h>
#include <QtQml/qqmlfile.h>
#include <QtQuick3D/private/qquick3dmodel_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <algorithm>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ParticleModelShape3D
    \inherits ParticleAbstractShape3D
    \inqmlmodule QtQuick3D.Particles3D
    \brief Offers particle shape from model for emitters and affectors.
    \since 6.2

    The ParticleModelShape3D element can be used to get particle shape from a 3D model.

    For example, to emit particles from outlines of a model shape:

    \qml
    Component {
        id: suzanneComponent
        Model {
            source: "meshes/suzanne.mesh"
            scale: Qt.vector3d(100, 100, 100)
        }
    }

    ParticleEmitter3D {
        shape: ParticleModelShape3D {
            model: suzanneComponent
            fill: false
        }
        ...
    }
    \endqml
*/

QQuick3DParticleModelShape::QQuick3DParticleModelShape(QObject *parent)
    : QQuick3DParticleAbstractShape(parent)
{

}

QQuick3DParticleModelShape::~QQuick3DParticleModelShape()
{
    delete m_model;
}

/*!
    \qmlproperty bool ParticleModelShape3D::fill

    This property defines if the shape should be filled or just use the shape outlines.

    The default value is \c true.
*/
bool QQuick3DParticleModelShape::fill() const
{
    return m_fill;
}

/*!
    \qmlproperty Component ParticleModelShape3D::delegate
    The delegate provides a template defining the model for the ParticleModelShape3D.
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
    ParticleModelShape3D {
        delegate: modelComponent
    }
    \endqml
*/
QQmlComponent *QQuick3DParticleModelShape::delegate() const
{
    return m_delegate;
}

void QQuick3DParticleModelShape::setFill(bool fill)
{
    if (m_fill == fill)
        return;

    m_fill = fill;
    Q_EMIT fillChanged();
}

QVector3D QQuick3DParticleModelShape::getPosition(int particleIndex)
{
    return randomPositionModel(particleIndex);
}

QVector3D QQuick3DParticleModelShape::getSurfaceNormal(int particleIndex)
{
    if (m_cachedIndex != particleIndex)
        getPosition(particleIndex);
    return m_cachedNormal;
}

static QSSGMesh::Mesh loadModelShapeMesh(const QString &source)
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

void QQuick3DParticleModelShape::setDelegate(QQmlComponent *delegate)
{
    if (delegate == m_delegate)
        return;
    m_delegate = delegate;
    clearModelVertexPositions();
    createModel();
    Q_EMIT delegateChanged();
}

void QQuick3DParticleModelShape::createModel()
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

QVector3D QQuick3DParticleModelShape::randomPositionModel(int particleIndex)
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
            } else {
                m_cachedIndex = particleIndex;
                m_cachedNormal = QVector3D::crossProduct(v2 - v1, v3 - v2).normalized();
            }

            auto *parent = parentNode();
            if (parent) {
                QMatrix4x4 mat;
                mat.rotate(parent->rotation() * m_model->rotation());
                if (!m_fill)
                    m_cachedNormal = mat.mapVector(m_cachedNormal);
                return mat.mapVector(pos * parent->sceneScale() * m_model->scale());
            }
        }
    }
    return QVector3D(0, 0, 0);
}

void QQuick3DParticleModelShape::clearModelVertexPositions()
{
    m_vertexPositions.clear();
    m_modelTriangleAreas.clear();
    m_modelTriangleAreasSum = 0;
}

void QQuick3DParticleModelShape::calculateModelVertexPositions()
{
    if (m_vertexPositions.empty()) {
        QVector<QVector3D> indicedPositions;
        QVector<QVector3D> positions;

        if (m_model->geometry()) {
            QQuick3DGeometry *geometry = m_model->geometry();
            bool hasIndexBuffer = false;
            QQuick3DGeometry::Attribute::ComponentType indexBufferFormat;
            int posOffset = 0;
            QQuick3DGeometry::Attribute::ComponentType posType = QQuick3DGeometry::Attribute::U16Type;
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
                if (hasIndexBuffer) {
                    const auto &data = geometry->vertexData();
                    int indexSize = 4;
                    if (indexBufferFormat == QQuick3DGeometry::Attribute::U16Type)
                        indexSize = 2;
                    for (int i = 0; i < data.size(); i += indexSize) {
                        qsizetype index = 0;
                        memcpy(&index, data + i, indexSize);
                        if (positions.size() > index)
                            indicedPositions.append(positions[index]);
                    }
                }
            }
        } else {
                const QQmlContext *context = qmlContext(this);
                QString src = m_model->source().toString();
                if (context && !src.startsWith(QLatin1Char('#')))
                    src = QQmlFile::urlToLocalFileOrQrc(context->resolvedUrl(m_model->source()));
                QSSGMesh::Mesh mesh = loadModelShapeMesh(src);
                if (!mesh.isValid())
                    return;
                if (mesh.drawMode() != QSSGMesh::Mesh::DrawMode::Triangles)
                    return;

                auto entries = mesh.vertexBuffer().entries;
                int posOffset = 0;
                int posCount = 0;
                // Just set 'posType' to something to avoid invalid 'maybe-uninitialized' warning
                QSSGMesh::Mesh::ComponentType posType = QSSGMesh::Mesh::ComponentType::UnsignedInt8;
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
                    const auto &indexData = mesh.indexBuffer().data;
                    int indexSize = QSSGMesh::MeshInternal::byteSizeForComponentType(mesh.indexBuffer().componentType);
                    for (int i = 0; i < indexData.size(); i += indexSize) {
                        qsizetype index = 0;
                        memcpy(&index, indexData + i, indexSize);
                        if (positions.size() > index)
                            indicedPositions.append(positions[index]);
                    }
                }
        }
        if (!indicedPositions.empty())
            m_vertexPositions = indicedPositions;
        else
            m_vertexPositions = positions;
    }
}

QT_END_NAMESPACE
