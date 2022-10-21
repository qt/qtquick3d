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

#include "qquick3dparticlemodelblendparticle_p.h"
#include "qquick3dparticleemitter_p.h"
#include "qquick3dparticlerandomizer_p.h"

#include <QtCore/qdir.h>
#include <QtQml/qqmlfile.h>

#include <QtQuick3D/private/qquick3dobject_p.h>
#include <QtQuick3D/private/qquick3dgeometry_p.h>

#include <QtQuick3DUtils/private/qssgutils_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderparticles_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendergeometry_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermodel_p.h>
#include <QtQuick3DUtils/private/qssgmesh_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ModelBlendParticle3D
    \inherits Particle3D
    \inqmlmodule QtQuick3D.Particles3D
    \brief Blends particle effect with a 3D model.
    \preliminary

    \note This type is in tech preview in 6.2. \b {The API is under development and subject to change.}

    The type provides a way to blend particle effect with a 3D model. The provided model needs to be
    triangle-based. Each triangle in the model is converted into a particle, which are then used by
    the emitter. Instead of particle shader, the model is shaded using the \l {Model::materials}{material}
    specified in the model. The way the effect is blended is determined by the \l modelBlendMode.

    The possible modes are:
    \list
    \li \b Construct, where the model gets constructed from the particles.
    \li \b Explode, where the model gets converted into particles.
    \li \b Transfer, where Construct and Explode are combined to create an effect where the model is
    transferred from one place to another.
    \endlist

    The particles are emitted in the order they are specified in the model unless \l activationNode is set
    or \l random is set to \c true.

    Some features defined in base class and emitters are not functional with this type:
    \list
    \li \l Particle3D::alignMode is not functional since the particles can be in arbitrary orientation
    in the model.
     \li\l Particle3D::sortMode is not functional since the particles are always rendered in the order
    the primitives are specified in the model.
    \li \l ParticleEmitter3D::depthBias is not functional since the model depth bias is used instead.
    \endlist

    \note The default \l {Particle3D::fadeInEffect}{fadeInEffect} and \l {Particle3D::fadeInEffect}{fadeOutEffect}
    are \c Particle3D.FadeNone.
*/

QQuick3DParticleModelBlendParticle::QQuick3DParticleModelBlendParticle(QQuick3DNode *parent)
    : QQuick3DParticle(*new QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::ModelBlendParticle), parent)
{
    setFadeInEffect(QQuick3DParticle::FadeNone);
    setFadeOutEffect(QQuick3DParticle::FadeNone);
    QQuick3DParticle::doSetMaxAmount(0);
}

QQuick3DParticleModelBlendParticle::~QQuick3DParticleModelBlendParticle()
{
    delete m_model;
    delete m_modelGeometry;
}

/*!
    \qmlproperty Component ModelBlendParticle3D::delegate
    \preliminary
    The delegate provides a template defining the model for the ModelBlendParticle3D.

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

    ModelBlendParticle3D {
        id: particleRedSphere
        delegate: modelComponent
    }
    \endqml
*/
QQmlComponent *QQuick3DParticleModelBlendParticle::delegate() const
{
    return m_delegate;
}

void QQuick3DParticleModelBlendParticle::setDelegate(QQmlComponent *delegate)
{
    if (delegate == m_delegate)
        return;
    m_delegate = delegate;

    reset();
    regenerate();
    Q_EMIT delegateChanged();
}

/*!
    \qmlproperty Node ModelBlendParticle3D::endNode
    \preliminary
    This property holds the node that specifies the transformation for the model at the end
    of particle effect. It defines the size, position and rotation where the model is constructed
    when using the \c ModelBlendParticle3D.Construct and \c ModelBlendParticle3D.Transfer blend modes.
*/
QQuick3DNode *QQuick3DParticleModelBlendParticle::endNode() const
{
    return m_endNode;
}

/*!
    \qmlproperty enumeration ModelBlendParticle3D::ModelBlendMode
    \preliminary
    Defines the blending mode for the particle effect.

    \value ModelBlendParticle3D.Explode
        The model gets exploded i.e. the particles are emitted from the position of the model.
    \value ModelBlendParticle3D.Construct
        The model gets constructed i.e the particles fly from the emitter and construct the model at the end.
    \value ModelBlendParticle3D.Transfer
        Combines Explode and Transfer for the same model.
*/
/*!
    \qmlproperty ModelBlendMode ModelBlendParticle3D::modelBlendMode
    \preliminary
    This property holds blending mode for the particle effect.
*/
QQuick3DParticleModelBlendParticle::ModelBlendMode QQuick3DParticleModelBlendParticle::modelBlendMode() const
{
    return m_modelBlendMode;
}

/*!
    \qmlproperty int ModelBlendParticle3D::endTime
    \preliminary
    This property holds the end time of the particle in milliseconds. The end time is used during construction
    and defines duration from particle lifetime at the end where the effect is blended with
    the model positions. Before the end time the particles positions are defined only by the
    particle effect, but during end time the particle position is blended linearly with the model
    end position.
*/
int QQuick3DParticleModelBlendParticle::endTime() const
{
    return m_endTime;
}

/*!
    \qmlproperty Node ModelBlendParticle3D::activationNode
    \preliminary
    This property holds a node that activates particles and overrides the reqular emit routine.
    The activation node can be used to control how the particles are emitted spatially when the
    model is exploded/constructed from the particles.
    The activation node emits a particle if the center of that particle is on the positive half
    of the z-axis of the activation node. Animating the activation node to move trough the model
    will cause the particles to be emitted sequentially along the path the activation node moves.
*/
QQuick3DNode *QQuick3DParticleModelBlendParticle::activationNode() const
{
    return m_activationNode;
}

/*!
    \qmlproperty bool ModelBlendParticle3D::random
    \preliminary
    This property holds whether the particles are emitted in random order instead of in the order
    they are specified in the model. The default is \c false.
    \note This property is ignored if \l activationNode is set.
*/
bool QQuick3DParticleModelBlendParticle::random() const
{
    return m_random;
}

void QQuick3DParticleModelBlendParticle::setEndNode(QQuick3DNode *node)
{
    if (m_endNode == node)
        return;
    if (m_endNode)
        QObject::disconnect(this);

    m_endNode = node;

    if (m_endNode) {
        QObject::connect(m_endNode, &QQuick3DNode::positionChanged, this, &QQuick3DParticleModelBlendParticle::handleEndNodeChanged);
        QObject::connect(m_endNode, &QQuick3DNode::rotationChanged, this, &QQuick3DParticleModelBlendParticle::handleEndNodeChanged);
        QObject::connect(m_endNode, &QQuick3DNode::scaleChanged, this, &QQuick3DParticleModelBlendParticle::handleEndNodeChanged);
    }
    handleEndNodeChanged();
    Q_EMIT endNodeChanged();
}

void QQuick3DParticleModelBlendParticle::setModelBlendMode(ModelBlendMode mode)
{
    if (m_modelBlendMode == mode)
        return;
    m_modelBlendMode = mode;
    reset();
    Q_EMIT modelBlendModeChanged();
}

void QQuick3DParticleModelBlendParticle::setEndTime(int endTime)
{
    if (endTime == m_endTime)
        return;
    m_endTime = endTime;
    Q_EMIT endTimeChanged();
}

void QQuick3DParticleModelBlendParticle::setActivationNode(QQuick3DNode *activationNode)
{
    if (m_activationNode == activationNode)
        return;

    m_activationNode = activationNode;
    Q_EMIT activationNodeChanged();
}

void QQuick3DParticleModelBlendParticle::setRandom(bool random)
{
    if (m_random == random)
        return;

    m_random = random;
    Q_EMIT randomChanged();
}

void QQuick3DParticleModelBlendParticle::regenerate()
{
    delete m_model;
    m_model = nullptr;

    if (!isComponentComplete())
        return;

    if (!m_delegate)
        return;

    if (QQuick3DParticleSystem::isGloballyDisabled())
        return;

    auto *obj = m_delegate->create(m_delegate->creationContext());

    m_model = qobject_cast<QQuick3DModel *>(obj);
    if (m_model) {
        updateParticles();
        auto *psystem = QQuick3DParticle::system();
        m_model->setParent(psystem);
        m_model->setParentItem(psystem);
    } else {
        delete obj;
    }
    handleEndNodeChanged();
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

static QVector3D getPosition(const quint8 *srcVertices, quint32 idx, quint32 vertexStride, quint32 posOffset)
{
    const quint8 *vertex = srcVertices + idx * vertexStride;
    return *reinterpret_cast<const QVector3D *>(vertex + posOffset);
}

static void copyToUnindexedVertices(QByteArray &unindexedVertexData,
                                    QVector<QVector3D> &centerData,
                                    const QByteArray &vertexBufferData,
                                    quint32 vertexStride,
                                    quint32 posOffset,
                                    const QByteArray &indexBufferData,
                                    bool u16Indices,
                                    quint32 primitiveCount)
{
    const quint8 *srcVertices = reinterpret_cast<const quint8 *>(vertexBufferData.data());
    quint8 *dst = reinterpret_cast<quint8 *>(unindexedVertexData.data());
    const quint16 *indexData16 = reinterpret_cast<const quint16 *>(indexBufferData.begin());
    const quint32 *indexData32 = reinterpret_cast<const quint32 *>(indexBufferData.begin());
    const float c_div3 = 1.0f / 3.0f;
    for (quint32 i = 0; i < primitiveCount; i++) {
        quint32 i0, i1, i2;
        if (u16Indices) {
            i0 = indexData16[3 * i];
            i1 = indexData16[3 * i + 1];
            i2 = indexData16[3 * i + 2];
        } else {
            i0 = indexData32[3 * i];
            i1 = indexData32[3 * i + 1];
            i2 = indexData32[3 * i + 2];
        }
        QVector3D p0 = getPosition(srcVertices, i0, vertexStride, posOffset);
        QVector3D p1 = getPosition(srcVertices, i1, vertexStride, posOffset);
        QVector3D p2 = getPosition(srcVertices, i2, vertexStride, posOffset);
        QVector3D center = (p0 + p1 + p2) * c_div3;
        centerData[i] = center;
        memcpy(dst, srcVertices + i0 * vertexStride, vertexStride);
        dst += vertexStride;
        memcpy(dst, srcVertices + i1 * vertexStride, vertexStride);
        dst += vertexStride;
        memcpy(dst, srcVertices + i2 * vertexStride, vertexStride);
        dst += vertexStride;
    }
}

static void getVertexCenterData(QVector<QVector3D> &centerData,
                                const QByteArray &vertexBufferData,
                                quint32 vertexStride,
                                quint32 posOffset,
                                quint32 primitiveCount)
{
    const quint8 *srcVertices = reinterpret_cast<const quint8 *>(vertexBufferData.data());
    const float c_div3 = 1.0f / 3.0f;
    for (quint32 i = 0; i < primitiveCount; i++) {
        QVector3D p0 = getPosition(srcVertices, 3 * i, vertexStride, posOffset);
        QVector3D p1 = getPosition(srcVertices, 3 * i + 1, vertexStride, posOffset);
        QVector3D p2 = getPosition(srcVertices, 3 * i + 2, vertexStride, posOffset);
        QVector3D center = (p0 + p1 + p2) * c_div3;
        centerData[i] = center;
    }
}

void QQuick3DParticleModelBlendParticle::updateParticles()
{
    // The primitives needs to be triangle list without indexing, because each triangle
    // needs to be it's own primitive and we need vertex index to get the particle index,
    // which we don't get with indexed primitives
    if (m_model->geometry()) {
        QQuick3DGeometry *geometry = m_model->geometry();
        if (geometry->primitiveType() != QQuick3DGeometry::PrimitiveType::Triangles) {
            qWarning () << "ModelBlendParticle3D: Invalid geometry primitive type, must be Triangles. ";
            return;
        }
        auto vertexBuffer = geometry->vertexData();
        auto indexBuffer = geometry->indexData();

        if (!vertexBuffer.size()) {
            qWarning () << "ModelBlendParticle3D: Invalid geometry, vertexData is empty. ";
            return;
        }

        const auto attributeBySemantic = [&](const QQuick3DGeometry *geometry, QQuick3DGeometry::Attribute::Semantic semantic) {
            for (int i = 0; i < geometry->attributeCount(); i++) {
                const auto attr = geometry->attribute(i);
                if (attr.semantic == semantic)
                    return attr;
            }
            Q_ASSERT(0);
            return QQuick3DGeometry::Attribute();
        };

        if (indexBuffer.size()) {
            m_modelGeometry = new QQuick3DGeometry;

            m_modelGeometry->setBounds(geometry->boundsMin(), geometry->boundsMax());
            m_modelGeometry->setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
            m_modelGeometry->setStride(geometry->stride());

            for (int i = 0; i < geometry->attributeCount(); i++) {
                auto attr = geometry->attribute(i);
                if (attr.semantic != QQuick3DGeometry::Attribute::IndexSemantic)
                    m_modelGeometry->addAttribute(attr);
            }

            // deindex data
            QByteArray unindexedVertexData;
            quint32 primitiveCount = indexBuffer.size();
            auto indexAttribute = attributeBySemantic(geometry, QQuick3DGeometry::Attribute::IndexSemantic);
            bool u16IndexType = indexAttribute.componentType == QQuick3DGeometry::Attribute::U16Type;
            if (u16IndexType)
                primitiveCount /= 6;
            else
                primitiveCount /= 12;

            unindexedVertexData.resize(geometry->stride() * primitiveCount * 3);
            m_centerData.resize(primitiveCount);
            m_particleCount = primitiveCount;
            copyToUnindexedVertices(unindexedVertexData, m_centerData, vertexBuffer, geometry->stride(), attributeBySemantic(geometry, QQuick3DGeometry::Attribute::PositionSemantic).offset, indexBuffer, u16IndexType, primitiveCount);

            m_modelGeometry->setVertexData(unindexedVertexData);
            m_model->setGeometry(m_modelGeometry);
        } else {
            // can use provided geometry directly
            quint32 primitiveCount = vertexBuffer.size() / geometry->stride() / 3;
            m_centerData.resize(primitiveCount);
            m_particleCount = primitiveCount;
            getVertexCenterData(m_centerData, vertexBuffer, geometry->stride(), attributeBySemantic(geometry, QQuick3DGeometry::Attribute::PositionSemantic).offset, primitiveCount);
        }
    } else {
        const QQmlContext *context = qmlContext(this);
        QString src = m_model->source().toString();
        if (context && !src.startsWith(QLatin1Char('#')))
            src = QQmlFile::urlToLocalFileOrQrc(context->resolvedUrl(m_model->source()));
        QSSGMesh::Mesh mesh = loadMesh(src);
        if (!mesh.isValid()) {
            qWarning () << "ModelBlendParticle3D: Unable to load mesh: " << src;
            return;
        }
        if (mesh.drawMode() != QSSGMesh::Mesh::DrawMode::Triangles) {
            qWarning () << "ModelBlendParticle3D: Invalid mesh primitive type, must be Triangles. ";
            return;
        }

        m_modelGeometry = new QQuick3DGeometry;

        const auto vertexBuffer = mesh.vertexBuffer();
        const auto indexBuffer = mesh.indexBuffer();

        const auto entryOffset = [&](const QSSGMesh::Mesh::VertexBuffer &vb, const QByteArray &name) -> int {
            for (const auto &e : vb.entries) {
                if (e.name == name) {
                    Q_ASSERT(e.componentType == QSSGMesh::Mesh::ComponentType::Float32);
                    return e.offset;
                }
            }
            Q_ASSERT(0);
            return -1;
        };
        const auto toAttribute = [&](const QSSGMesh::Mesh::VertexBufferEntry &e) -> QQuick3DGeometry::Attribute {
            QQuick3DGeometry::Attribute a;
            a.componentType = QQuick3DGeometryPrivate::toComponentType(e.componentType);
            a.offset = e.offset;
            a.semantic = QQuick3DGeometryPrivate::semanticFromName(e.name);
            return a;
        };

        const auto indexedPrimitiveCount = [&](const QSSGMesh::Mesh::IndexBuffer &indexBuffer) -> quint32 {
            if (indexBuffer.componentType == QSSGMesh::Mesh::ComponentType::UnsignedInt16)
                return quint32(indexBuffer.data.size() / sizeof(quint16) / 3);
            return quint32(indexBuffer.data.size() / sizeof(quint32) / 3);
        };

        if (indexBuffer.data.size()) {
            // deindex data
            QByteArray unindexedVertexData;
            quint32 primitiveCount = indexedPrimitiveCount(indexBuffer);
            bool u16IndexType = indexBuffer.componentType == QSSGMesh::Mesh::ComponentType::UnsignedInt16;
            unindexedVertexData.resize(vertexBuffer.stride * primitiveCount * 3);
            m_centerData.resize(primitiveCount);
            m_particleCount = primitiveCount;

            copyToUnindexedVertices(unindexedVertexData, m_centerData, vertexBuffer.data, vertexBuffer.stride, entryOffset(vertexBuffer, QByteArray(QSSGMesh::MeshInternal::getPositionAttrName())), indexBuffer.data, u16IndexType, primitiveCount);
            m_modelGeometry->setBounds(mesh.subsets().first().bounds.min, mesh.subsets().first().bounds.max);
            m_modelGeometry->setStride(vertexBuffer.stride);
            m_modelGeometry->setVertexData(unindexedVertexData);
            m_modelGeometry->setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
        } else {
            // can use vertexbuffer directly
            quint32 primitiveCount = vertexBuffer.data.size() / vertexBuffer.stride / 3;
            m_centerData.resize(primitiveCount);
            m_particleCount = primitiveCount;
            getVertexCenterData(m_centerData, vertexBuffer.data, vertexBuffer.stride, entryOffset(vertexBuffer, QByteArray(QSSGMesh::MeshInternal::getPositionAttrName())), primitiveCount);
            m_modelGeometry->setBounds(mesh.subsets().first().bounds.min, mesh.subsets().first().bounds.max);
            m_modelGeometry->setStride(vertexBuffer.stride);
            m_modelGeometry->setVertexData(vertexBuffer.data);
            m_modelGeometry->setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
        }
        QQuick3DGeometryPrivate *geometryPrivate = static_cast<QQuick3DGeometryPrivate *>(QQuick3DGeometryPrivate::get(m_modelGeometry));
        for (auto &e : vertexBuffer.entries)
            m_modelGeometry->addAttribute(toAttribute(e));
        for (auto &s : mesh.subsets())
            geometryPrivate->m_subsets.append({s.name, s.bounds.min, s.bounds.max, s.offset, s.count});
        m_model->setSource({});
        m_model->setGeometry(m_modelGeometry);
    }

    QMatrix4x4 transform = m_model->sceneTransform();
    if (m_model->parentNode())
        transform = m_model->parentNode()->sceneTransform().inverted() * transform;
    m_triangleParticleData.resize(m_particleCount);
    m_particleData.resize(m_particleCount);
    m_particleData.fill({});
    for (int i = 0; i < m_particleCount; i++) {
        m_triangleParticleData[i].center = m_centerData[i];
        m_centerData[i] = transform * m_centerData[i];
        if (m_modelBlendMode == Construct) {
            m_triangleParticleData[i].size = 0.0f;
        } else {
            m_triangleParticleData[i].size = 1.0f;
            m_triangleParticleData[i].position = m_centerData[i];
        }
    }
    QQuick3DParticle::doSetMaxAmount(m_particleCount);
}

QSSGRenderGraphObject *QQuick3DParticleModelBlendParticle::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!m_model)
        return node;
    auto *spatialNode = QQuick3DObjectPrivate::get(m_model)->spatialNode;
    if (!spatialNode) {
        spatialNode = QQuick3DObjectPrivate::updateSpatialNode(m_model, nullptr);
        QQuick3DObjectPrivate::get(m_model)->spatialNode = spatialNode;
    }

    QSSGRenderModel *model = static_cast<QSSGRenderModel *>(spatialNode);

    if (!model->particleBuffer) {
        QSSGParticleBuffer *buffer = model->particleBuffer = new QSSGParticleBuffer;
        buffer->resize(m_particleCount, sizeof(QSSGTriangleParticle));
    }
    QQuick3DParticleSystem *psystem = QQuick3DParticle::system();
    QMatrix4x4 particleMatrix = psystem->sceneTransform().inverted() * m_model->sceneTransform();
    model->particleMatrix = particleMatrix.inverted();
    model->hasTransparency = fadeInEffect() == QQuick3DParticle::FadeOpacity || fadeOutEffect() == QQuick3DParticle::FadeOpacity;
    updateParticleBuffer(model->particleBuffer);

    return node;
}

void QQuick3DParticleModelBlendParticle::componentComplete()
{
    if (!system() && qobject_cast<QQuick3DParticleSystem *>(parentItem()))
        setSystem(qobject_cast<QQuick3DParticleSystem *>(parentItem()));

    // don't call particles componentComplete, we don't wan't to emit maxAmountChanged yet
    QQuick3DObject::componentComplete();
    regenerate();
}

void QQuick3DParticleModelBlendParticle::doSetMaxAmount(int)
{
    qWarning() << "ModelBlendParticle3D.maxAmount: Unable to set maximum amount, because it is set from the model.";
    return;
}

int QQuick3DParticleModelBlendParticle::nextCurrentIndex(const QQuick3DParticleEmitter *emitter)
{
    if (!m_perEmitterData.contains(emitter)) {
        m_perEmitterData.insert(emitter, PerEmitterData());
        auto &perEmitter = m_perEmitterData[emitter];
        perEmitter.emitter = emitter;
        perEmitter.emitterIndex = m_nextEmitterIndex++;
    }
    auto &perEmitter = m_perEmitterData[emitter];
    int index = QQuick3DParticle::nextCurrentIndex(emitter);
    if (m_triangleParticleData[index].emitterIndex != perEmitter.emitterIndex) {
        if (m_triangleParticleData[index].emitterIndex >= 0)
            perEmitterData(m_triangleParticleData[index].emitterIndex).particleCount--;
        perEmitter.particleCount++;
    }
    m_triangleParticleData[index].emitterIndex = perEmitter.emitterIndex;
    return index;
}


void QQuick3DParticleModelBlendParticle::setParticleData(int particleIndex,
                                                         const QVector3D &position,
                                                         const QVector3D &rotation,
                                                         const QVector4D &color,
                                                         float size, float age)
{
    auto &dst = m_triangleParticleData[particleIndex];
    dst = {position, rotation, dst.center, color, age, size, dst.emitterIndex};
    m_dataChanged = true;
}

QQuick3DParticleModelBlendParticle::PerEmitterData &QQuick3DParticleModelBlendParticle::perEmitterData(int emitterIndex)
{
    for (auto &perEmitter : m_perEmitterData) {
        if (perEmitter.emitterIndex == emitterIndex)
            return perEmitter;
    }
    return n_noPerEmitterData;
}

void QQuick3DParticleModelBlendParticle::updateParticleBuffer(QSSGParticleBuffer *buffer)
{
    const auto &particles = m_triangleParticleData;

    if (!buffer || !m_dataChanged)
        return;

    const int particleCount = m_particleCount;

    char *dest = buffer->pointer();
    const TriangleParticleData *src = particles.data();
    const int pps = buffer->particlesPerSlice();
    const int ss = buffer->sliceStride();
    const int slices = buffer->sliceCount();
    const float c_degToRad = float(M_PI / 180.0f);
    int i = 0;
    QSSGBounds3 bounds;
    for (int s = 0; s < slices; s++) {
        QSSGTriangleParticle *dp = reinterpret_cast<QSSGTriangleParticle *>(dest);
        for (int p = 0; p < pps && i < particleCount; ) {
            if (src->size > 0.0f)
                bounds.include(src->position);
            dp->position = src->position;
            dp->rotation = src->rotation * c_degToRad;
            dp->color = src->color;
            dp->age = src->age;
            dp->center = src->center;
            dp->size = src->size;
            dp++;
            p++;
            i++;
            src++;
        }
        dest += ss;
    }
    buffer->setBounds(bounds);
    m_dataChanged = false;
}

void QQuick3DParticleModelBlendParticle::itemChange(QQuick3DObject::ItemChange change,
                                                    const QQuick3DObject::ItemChangeData &value)
{
    QQuick3DObject::itemChange(change, value);
    if (change == ItemParentHasChanged && value.sceneManager)
        regenerate();
}

void QQuick3DParticleModelBlendParticle::reset()
{
    QQuick3DParticle::reset();
    if (m_particleCount) {
        for (int i = 0; i < m_particleCount; i++) {
            if (m_modelBlendMode == Construct) {
                m_triangleParticleData[i].size = 0.0f;
            } else {
                m_triangleParticleData[i].size = 1.0f;
                m_triangleParticleData[i].position = m_triangleParticleData[i].center;
            }
        }
    }
}

QVector3D QQuick3DParticleModelBlendParticle::particleCenter(int particleIndex) const
{
    return m_centerData[particleIndex];
}

bool QQuick3DParticleModelBlendParticle::lastParticle() const
{
    return m_currentIndex >= m_maxAmount - 1;
}

static QMatrix3x3 qt_fromEulerRotation(const QVector3D &eulerRotation)
{
    float x = qDegreesToRadians(eulerRotation.x());
    float y = qDegreesToRadians(eulerRotation.y());
    float z = qDegreesToRadians(eulerRotation.z());
    float a = cos(x);
    float b = sin(x);
    float c = cos(y);
    float d = sin(y);
    float e = cos(z);
    float f = sin(z);
    QMatrix3x3 ret;
    float bd = b * d;
    float ad = a * d;
    ret(0,0) = c * e;
    ret(0,1) = -c * f;
    ret(0,2) = d;
    ret(1,0) = bd * e + a * f;
    ret(1,1) = a * e - bd * f;
    ret(1,2) = -b * c;
    ret(2,0) = b * f - ad * e;
    ret(2,1) = ad * f + b * e;
    ret(2,2) = a * c;
    return ret;
}

void QQuick3DParticleModelBlendParticle::handleEndNodeChanged()
{
    if (m_endNode && m_model) {
        if (!m_model->rotation().isIdentity()) {
            // Use the same function as the shader for end node rotation so that they produce same matrix
            QMatrix3x3 r1 = qt_fromEulerRotation(m_endNode->eulerRotation());
            QMatrix3x3 r2 = m_model->rotation().toRotationMatrix();
            QMatrix3x3 r = r2 * r1.transposed() * r2.transposed();
            m_endNodeRotation = m_endNode->eulerRotation();
            m_endRotationMatrix = QMatrix4x4(r);
        } else {
            m_endNodeRotation = m_endNode->eulerRotation();
            m_endRotationMatrix = QMatrix4x4(m_endNode->rotation().toRotationMatrix().transposed());
        }
        m_endNodePosition = m_endNode->position();
        m_endNodeScale = m_endNode->scale();
    } else {
        m_endNodePosition = QVector3D();
        m_endNodeRotation = QVector3D();
        m_endNodeScale = QVector3D(1.0f, 1.0f, 1.0f);
        m_endRotationMatrix.setToIdentity();
    }
}

QVector3D QQuick3DParticleModelBlendParticle::particleEndPosition(int idx) const
{
    return m_endRotationMatrix * QVector3D(m_endNodeScale * m_centerData[idx]) + m_endNodePosition;
}

QVector3D QQuick3DParticleModelBlendParticle::particleEndRotation(int) const
{
    return m_endNodeRotation;
}

int QQuick3DParticleModelBlendParticle::randomIndex(int particleIndex)
{
    if (m_randomParticles.isEmpty()) {
        m_randomParticles.resize(m_maxAmount);
        for (int i = 0; i < m_maxAmount; i++)
            m_randomParticles[i] = i;

        // Randomize particle indices just once
        QRandomGenerator rand(system()->rand()->generator());
        for (int i = 0; i < m_maxAmount; i++) {
            int ridx = rand.generate() % m_maxAmount;
            if (i != ridx)
                qSwap(m_randomParticles[i], m_randomParticles[ridx]);
        }
    }
    return m_randomParticles[particleIndex];
}

QT_END_NAMESPACE
