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

#include "qquick3dmodel_p.h"
#include "qquick3dobject_p.h"
#include "qquick3dscenemanager_p.h"
#include "qquick3dnode_p_p.h"
#include "qquick3dinstancing_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermodel_p.h>

#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtQml/QQmlFile>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Model
    \inherits Node
    \inqmlmodule QtQuick3D
    \brief Lets you load a 3D model data.

    The Model item makes it possible to load a mesh and modify how its shaded, by adding materials
    to it. For a model to be renderable, it needs at least a mesh and a material.

    \section1 Mesh format and built-in primitives

    The model can load static meshes from storage or one of the built-in primitive types.
    The mesh format used is a run-time format that's native to the engine, but additional formats are
    supported through the asset import tool \l {Balsam Asset Import Tool}{Balsam}.

    The built-in primitives can be loaded by setting the \c source property to one of these values:
    \c {#Rectangle, #Sphere, #Cube, #Cylinder or #Cone}.

    \qml
    Model {
        source: "#Sphere"
    }
    \endqml

    \section2 Custom geometry

    In addition to using static meshes, it is possible to implement a \l {QQuick3DGeometry}{custom geometry} provider that
    provides the model with custom vertex data at run-time. See the \l {Qt Quick 3D - Custom Geometry Example}{Custom Geometry Example}
    for an example on how to create and use a custom material with your model.

    \section1 Materials

    A model can consist of several sub-meshes, each of which can have its own material.
    The sub-mesh uses a material from the \l{materials} list, corresponding to its index.
    If the number of materials is less than the sub-meshes, the last material in the list is used
    for subsequent sub-meshes. This is demonstrated in the \l {Qt Quick 3D - Sub-mesh Example}{Sub-mesh example}.

    There are currently three different materials that can be used with the model item,
    the \l {PrincipledMaterial}, the \l {DefaultMaterial}, and the \l {CustomMaterial}.

    \section1 Picking

    Picking is the process of sending a ray through the scene from some starting position to find which model(s) intersects
    with the ray. In QtQuick3D the ray is normally sent from the view using 2D coordinates resulting from a touch or mouse
    event. If a model was hit by the ray a \l {PickResult} will be returned with a handle to the model and information about
    where the ray hit the model. For models that use \l {QQuick3DGeometry}{custom geometry} the picking is less accurate then
    for static mesh data, as picking is only done against the models \l {Bounds}{bounding volume}.
    If the ray goes through more then one model, the closest \l {Model::pickable}{pickable} model is selected.

    Note that models are not \l {Model::pickable}{pickable} by default, so to be able to \l {View3D::pick}{pick} a model
    in the scene, the model will need to make it self discoverable by setting the \l {Model::pickable}{pickable} property to true.
    Visit the \l {Qt Quick 3D - Picking example} to see how picking can be enabled.

*/

/*!
    \qmltype Bounds
    \inqmlmodule QtQuick3D
    \since 5.15
    \brief Specifies the bounds of a model.

    Bounds specify a bounding box with minimum and maximum points.
    Bounds is a readonly property of the model.
*/

/*!
    \qmlproperty vector3d Bounds::minimum

    Specifies the minimum point of the model bounds.
    \sa maximum
*/

/*!
    \qmlproperty vector3d Bounds::maximum

    Specifies the maximum point of the model bounds.
    \sa minimum
*/

QQuick3DModel::QQuick3DModel(QQuick3DNode *parent)
    : QQuick3DNode(*(new QQuick3DNodePrivate(QQuick3DNodePrivate::Type::Model)), parent) {}

QQuick3DModel::~QQuick3DModel()
{
    disconnect(m_geometryConnection);
    for (const auto &connection : qAsConst(m_connections))
        disconnect(connection);

    auto matList = materials();
    qmlClearMaterials(&matList);
    auto morphList = morphTargets();
    qmlClearMorphTargets(&morphList);
}

/*!
    \qmlproperty url Model::source

    This property defines the location of the mesh file containing the geometry
    of this Model or one of the built-in primitive meshes listed below
    as described in \l {Mesh format and built-in primitives}.

    \list
    \li "#Rectangle"
    \li "#Sphere"
    \li "#Cube"
    \li "#Cone"
    \li "#Cylinder"
    \endlist
*/

QUrl QQuick3DModel::source() const
{
    return m_source;
}

/*!
    \qmlproperty List<QtQuick3D::Material> Model::materials

    This property contains a list of materials used to render the provided
    geometry. To render anything, there must be at least one material. Normally
    there should be one material for each sub-mesh included in the source
    geometry.

    \sa {Qt Quick 3D - Sub-mesh Example}
*/


QQmlListProperty<QQuick3DMaterial> QQuick3DModel::materials()
{
    return QQmlListProperty<QQuick3DMaterial>(this,
                                            nullptr,
                                            QQuick3DModel::qmlAppendMaterial,
                                            QQuick3DModel::qmlMaterialsCount,
                                            QQuick3DModel::qmlMaterialAt,
                                            QQuick3DModel::qmlClearMaterials);
}

/*!
    \qmlproperty List<QtQuick3D::MorphTarget> Model::morphTargets

    This property contains a list of \l [QtQuick3D QML] {MorphTarget}{MorphTarget}s used to
    render the provided geometry. Meshes should have at least one attribute
    among positions, normals, tangent, bitangent for the morph targets.
    Quick3D supports maximum 8 morph targets and remains will be ignored.

    \note First 2 morph targets can have maximum 4 attributes among position,
    normal, tangent, and binormal.
    \note 3rd and 4th  morph targets can have maximum 2 attributes among
    position, and normal.
    \note Remaining morph targets can have only the position attribute.
    \note This property is not used when the model is shaded by \l {CustomMaterial}.

    \sa {MorphTarget}
*/

QQmlListProperty<QQuick3DMorphTarget> QQuick3DModel::morphTargets()
{
    return QQmlListProperty<QQuick3DMorphTarget>(this,
                                            nullptr,
                                            QQuick3DModel::qmlAppendMorphTarget,
                                            QQuick3DModel::qmlMorphTargetsCount,
                                            QQuick3DModel::qmlMorphTargetAt,
                                            QQuick3DModel::qmlClearMorphTargets);
}

/*!
    \qmlproperty QtQuick3D::Instancing Model::instancing

    If this property is set, the model will not be rendered normally. Instead, a number of
    instances of the model will be rendered, as defined by the instance table.

    \sa Instancing
*/

QQuick3DInstancing *QQuick3DModel::instancing() const
{
    return m_instancing;
}

/*!
    \qmlproperty QtQuick3D::Node Model::instanceRoot

    This property defines the origin of the instance’s coordinate system.

    See the \l{Transforms and instancing}{overview documentation} for a detailed explanation.

    \sa instancing, Instancing
*/
QQuick3DNode *QQuick3DModel::instanceRoot() const
{
    return m_instanceRoot;
}


void QQuick3DModel::markAllDirty()
{
    m_dirtyAttributes = 0xffffffff;
    QQuick3DNode::markAllDirty();
}

/*!
    \qmlproperty bool Model::castsShadows

    When this property is \c true, the geometry of this model is used when
    rendering to the shadow maps.
*/

bool QQuick3DModel::castsShadows() const
{
    return m_castsShadows;
}

/*!
    \qmlproperty bool Model::receivesShadows

    When this property is set to \c true, the model's materials take shadow contribution from
    shadow casting lights into account.
*/

bool QQuick3DModel::receivesShadows() const
{
    return m_receivesShadows;
}

/*!
    \qmlproperty bool Model::pickable

    This property controls whether the model is pickable or not. Set this property
    to \c true to make the model pickable. Models are not pickable by default.

    \sa {View3D::pick}
*/
bool QQuick3DModel::pickable() const
{
    return m_pickable;
}

/*!
    \qmlproperty Geometry Model::geometry

    Specify a custom geometry for the model. The Model::source must be empty when custom geometry
    is used.

*/
QQuick3DGeometry *QQuick3DModel::geometry() const
{
    return m_geometry;
}

/*!
    \qmlproperty Skeleton Model::skeleton

    Contains the skeleton for the model. The Skeleton is used together with \l {inverseBindPoses}
    for \l {Vertex Skinning}{skinning}.

    \note Meshes of the model must have both joints and weights attributes.
    \note If this property is set, skinning animation is enabled. It means
    that \l {Model} is transformed based on \l {Skeleton} ignoring Model's global
    transformation.

    \sa {Model::inverseBindPoses}, {Qt Quick 3D - Simple Skinning Example}
*/
QQuick3DSkeleton *QQuick3DModel::skeleton() const
{
    return m_skeleton;
}

/*!
    \qmlproperty List<matrix4x4> Model::inverseBindPoses

    This property contains a list of Inverse Bind Pose matrixes used for the
    skeletal animation. Each inverseBindPose matrix means the inverse of the
    global transform of the repective \l {Joint::index} in \l {skeleton}, which
    will be used initially.

    \note This property is only used if the Model::skeleton is valid.
    \note If some of the matrices are not set, identity values will be used.

    \sa {skeleton} {Joint::index}
*/
QList<QMatrix4x4> QQuick3DModel::inverseBindPoses() const
{
    return m_inverseBindPoses;
}

/*!
    \qmlproperty Bounds Model::bounds

    The bounds of the model descibes the extents of the bounding volume around the model.

    \note The bounds might not be immediately available if the model needs to be loaded first.

    \readonly
*/
QQuick3DBounds3 QQuick3DModel::bounds() const
{
    return m_bounds;
}

/*!
    \qmlproperty real Model::depthBias

    Holds the depth bias of the model. Depth bias is added to the object distance from camera when sorting
    objects. This can be used to force rendering order between objects close to each other, that
    might otherwise be rendered in different order in different frames. Negative values cause the
    sorting value to move closer to the camera while positive values move it further from the camera.
*/
float QQuick3DModel::depthBias() const
{
    return m_depthBias;
}

void QQuick3DModel::setSource(const QUrl &source)
{
    if (m_source == source)
        return;

    m_source = source;
    emit sourceChanged();
    markDirty(SourceDirty);
    if (QQuick3DObjectPrivate::get(this)->sceneManager)
        QQuick3DObjectPrivate::get(this)->sceneManager->dirtyBoundingBoxList.append(this);
}

void QQuick3DModel::setCastsShadows(bool castsShadows)
{
    if (m_castsShadows == castsShadows)
        return;

    m_castsShadows = castsShadows;
    emit castsShadowsChanged();
    markDirty(ShadowsDirty);
}

void QQuick3DModel::setReceivesShadows(bool receivesShadows)
{
    if (m_receivesShadows == receivesShadows)
        return;

    m_receivesShadows = receivesShadows;
    emit receivesShadowsChanged();
    markDirty(ShadowsDirty);
}

void QQuick3DModel::setPickable(bool isPickable)
{
    if (m_pickable == isPickable)
        return;

    m_pickable = isPickable;
    emit pickableChanged();
    markDirty(PickingDirty);
}

void QQuick3DModel::setGeometry(QQuick3DGeometry *geometry)
{
    if (geometry == m_geometry)
        return;

    // Make sure to disconnect if the geometry gets deleted out from under us
    QQuick3DObjectPrivate::updatePropertyListener(geometry, m_geometry, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("geometry"), m_connections, [this](QQuick3DObject *n) {
        setGeometry(qobject_cast<QQuick3DGeometry *>(n));
    });

    if (m_geometry)
        QObject::disconnect(m_geometryConnection);
    m_geometry = geometry;

    if (m_geometry) {
        m_geometryConnection
                = QObject::connect(m_geometry, &QQuick3DGeometry::geometryNodeDirty, [this]() {
            markDirty(GeometryDirty);
        });
    }
    emit geometryChanged();
    markDirty(GeometryDirty);
}

void QQuick3DModel::setSkeleton(QQuick3DSkeleton *skeleton)
{
    if (skeleton == m_skeleton)
        return;

    // Make sure to disconnect if the skeleton gets deleted out from under us
    QQuick3DObjectPrivate::updatePropertyListener(skeleton, m_skeleton, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("skeleton"), m_connections, [this](QQuick3DObject *n) {
        setSkeleton(qobject_cast<QQuick3DSkeleton *>(n));
    });

    if (m_skeleton)
        QObject::disconnect(m_skeletonConnection);
    m_skeleton = skeleton;
    if (m_skeleton) {
        m_skeletonConnection
                = QObject::connect(m_skeleton, &QQuick3DSkeleton::skeletonNodeDirty, [this]() {
            auto modelNode = static_cast<QSSGRenderModel *>(QQuick3DNodePrivate::get(this)->spatialNode);
            if (modelNode)
                modelNode->skinningDirty = true;
        });
    }
    emit skeletonChanged();
    markDirty(SkeletonDirty);
}

void QQuick3DModel::setInverseBindPoses(const QList<QMatrix4x4> &poses)
{
    if (m_inverseBindPoses == poses)
        return;

    m_inverseBindPoses = poses;
    emit inverseBindPosesChanged();
    markDirty(PoseDirty);
}


void QQuick3DModel::setBounds(const QVector3D &min, const QVector3D &max)
{
    if (!qFuzzyCompare(m_bounds.maximum(), max)
            || !qFuzzyCompare(m_bounds.minimum(), min))  {
        m_bounds.bounds = QSSGBounds3 { min, max };
        emit boundsChanged();
    }
}

void QQuick3DModel::setInstancing(QQuick3DInstancing *instancing)
{
    if (m_instancing == instancing)
        return;

    // Make sure to disconnect if the instance table gets deleted out from under us
    QQuick3DObjectPrivate::updatePropertyListener(instancing, m_instancing, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("instancing"), m_connections, [this](QQuick3DObject *n) {
        setInstancing(qobject_cast<QQuick3DInstancing *>(n));
    });
    if (m_instancing)
        QObject::disconnect(m_instancingConnection);
    m_instancing = instancing;
    if (m_instancing) {
        m_instancingConnection = QObject::connect
                (m_instancing, &QQuick3DInstancing::instanceNodeDirty,
                 this, [this]{ markDirty(InstancesDirty);});
    }
    markDirty(InstancesDirty);
    emit instancingChanged();
}

void QQuick3DModel::setInstanceRoot(QQuick3DNode *instanceRoot)
{
    if (m_instanceRoot == instanceRoot)
        return;

    m_instanceRoot = instanceRoot;
    emit instanceRootChanged();
}

void QQuick3DModel::setDepthBias(float bias)
{
    if (qFuzzyCompare(bias, m_depthBias))
        return;

    m_depthBias = bias;
    markDirty(PropertyDirty);
    emit depthBiasChanged();
}

void QQuick3DModel::itemChange(ItemChange change, const ItemChangeData &value)
{
    if (change == QQuick3DObject::ItemSceneChange)
        updateSceneManager(value.sceneManager);
}

QSSGRenderGraphObject *QQuick3DModel::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node) {
        markAllDirty();
        node = new QSSGRenderModel();
    }

    QQuick3DNode::updateSpatialNode(node);
    int dirtyAttribute = 0;

    auto modelNode = static_cast<QSSGRenderModel *>(node);
    if (m_dirtyAttributes & SourceDirty)
        modelNode->meshPath = QSSGRenderPath(translateSource());
    if (m_dirtyAttributes & PickingDirty)
        modelNode->flags.setFlag(QSSGRenderModel::Flag::LocallyPickable, m_pickable);

    if (m_dirtyAttributes & ShadowsDirty) {
        modelNode->castsShadows = m_castsShadows;
        modelNode->receivesShadows = m_receivesShadows;
    }

    if (m_dirtyAttributes & MaterialsDirty) {
        if (!m_materials.isEmpty()) {
            if (modelNode->materials.isEmpty()) {
                // Easy mode, just add each material
                for (const Material &material : m_materials) {
                    QSSGRenderGraphObject *graphObject = QQuick3DObjectPrivate::get(material.material)->spatialNode;
                    if (graphObject)
                        modelNode->materials.append(graphObject);
                    else
                        dirtyAttribute |= MaterialsDirty; // We still got dirty materials
                }
            } else {
                // Hard mode, go through each material and see if they match
                if (modelNode->materials.size() != m_materials.size())
                    modelNode->materials.resize(m_materials.size());
                for (int i = 0; i < m_materials.size(); ++i) {
                    QSSGRenderGraphObject *graphObject = QQuick3DObjectPrivate::get(m_materials[i].material)->spatialNode;
                    if (modelNode->materials[i] != graphObject)
                        modelNode->materials[i] = graphObject;
                }
            }
        } else {
            // No materials
            modelNode->materials.clear();
        }
    }

    if (m_dirtyAttributes & MorphTargetsDirty) {
        if (!m_morphTargets.isEmpty()) {
            const int numMorphTarget = m_morphTargets.size();
            if (modelNode->morphTargets.isEmpty()) {
                // Easy mode, just add each morphTarget
                for (const auto morphTarget : qAsConst(m_morphTargets)) {
                    QSSGRenderGraphObject *graphObject = QQuick3DObjectPrivate::get(morphTarget)->spatialNode;
                    if (graphObject)
                        modelNode->morphTargets.append(graphObject);
                    else
                        dirtyAttribute |= MorphTargetsDirty; // We still got dirty morphTargets
                }
                modelNode->morphWeights.resize(numMorphTarget);
                modelNode->morphAttributes.resize(numMorphTarget);
            } else {
                // Hard mode, go through each morphTarget and see if they match
                if (modelNode->morphTargets.size() != numMorphTarget) {
                    modelNode->morphTargets.resize(numMorphTarget);
                    modelNode->morphWeights.resize(numMorphTarget);
                    modelNode->morphAttributes.resize(numMorphTarget);
                }
                for (int i = 0; i < numMorphTarget; ++i)
                    modelNode->morphTargets[i] = QQuick3DObjectPrivate::get(m_morphTargets.at(i))->spatialNode;
            }
        } else {
            // No morphTargets
            modelNode->morphTargets.clear();
        }
    }

    if (m_dirtyAttributes & InstancesDirty) {
        if (m_instancing) {
            modelNode->instanceTable = static_cast<QSSGRenderInstanceTable *>(QQuick3DObjectPrivate::get(m_instancing)->spatialNode);
        } else {
            modelNode->instanceTable = nullptr;
        }
    }

    if (m_dirtyAttributes & GeometryDirty) {
        if (m_geometry) {
            modelNode->geometry = static_cast<QSSGRenderGeometry *>(QQuick3DObjectPrivate::get(m_geometry)->spatialNode);
            setBounds(m_geometry->boundsMin(), m_geometry->boundsMax());
        } else {
            modelNode->geometry = nullptr;
            setBounds(QVector3D(), QVector3D());
        }
    }

    if (m_dirtyAttributes & SkeletonDirty) {
        modelNode->skinningDirty = true;
        if (m_skeleton)
            modelNode->skeleton = static_cast<QSSGRenderSkeleton *>(QQuick3DObjectPrivate::get(m_skeleton)->spatialNode);
        else
            modelNode->skeleton = nullptr;
    }

    if (m_dirtyAttributes & PoseDirty) {
        modelNode->inverseBindPoses = m_inverseBindPoses.toVector();
        modelNode->skinningDirty = true;
    }

    if (m_dirtyAttributes & PropertyDirty)
        modelNode->m_depthBias = m_depthBias;

    m_dirtyAttributes = dirtyAttribute;

    return modelNode;
}

// Source URL's need a bit of translation for the engine because of the
// use of fragment syntax for specifiying primitives and sub-meshes
// So we need to check for the fragment before translating to a qmlfile

QString QQuick3DModel::translateSource()
{
    QString fragment;
    if (m_source.hasFragment()) {
        // Check if this is an index, or primitive
        bool isNumber = false;
        m_source.fragment().toInt(&isNumber);
        fragment = QStringLiteral("#") + m_source.fragment();
        // If it wasn't an index, then it was a primitive
        if (!isNumber)
            return fragment;
    }

    const QQmlContext *context = qmlContext(this);
    const auto resolvedUrl = context ? context->resolvedUrl(m_source) : m_source;
    const auto qmlSource = QQmlFile::urlToLocalFileOrQrc(resolvedUrl);
    return (qmlSource.isEmpty() ? m_source.path() : qmlSource) + fragment;
}

void QQuick3DModel::markDirty(QQuick3DModel::QSSGModelDirtyType type)
{
    if (!(m_dirtyAttributes & quint32(type))) {
        m_dirtyAttributes |= quint32(type);
        update();
    }
}

void QQuick3DModel::updateSceneManager(QQuick3DSceneManager *sceneManager)
{
    if (sceneManager) {
        sceneManager->dirtyBoundingBoxList.append(this);
        QQuick3DObjectPrivate::refSceneManager(m_skeleton, *sceneManager);
        QQuick3DObjectPrivate::refSceneManager(m_geometry, *sceneManager);
        QQuick3DObjectPrivate::refSceneManager(m_instancing, *sceneManager);
        for (Material &mat : m_materials) {
            if (!mat.material->parentItem() && !QQuick3DObjectPrivate::get(mat.material)->sceneManager) {
                if (!mat.refed) {
                    QQuick3DObjectPrivate::refSceneManager(mat.material, *sceneManager);
                    mat.refed = true;
                }
            }
        }
    } else {
        QQuick3DObjectPrivate::derefSceneManager(m_skeleton);
        QQuick3DObjectPrivate::derefSceneManager(m_geometry);
        QQuick3DObjectPrivate::derefSceneManager(m_instancing);
        for (Material &mat : m_materials) {
            if (mat.refed) {
                QQuick3DObjectPrivate::derefSceneManager(mat.material);
                mat.refed = false;
            }
        }
    }
}

void QQuick3DModel::onMaterialDestroyed(QObject *object)
{
    bool found = false;
    for (int i = 0; i < m_materials.count(); ++i) {
        if (m_materials[i].material == object) {
            m_materials.removeAt(i--);
            found = true;
        }
    }
    if (found)
        markDirty(QQuick3DModel::MaterialsDirty);
}

void QQuick3DModel::qmlAppendMaterial(QQmlListProperty<QQuick3DMaterial> *list, QQuick3DMaterial *material)
{
    if (material == nullptr)
        return;
    QQuick3DModel *self = static_cast<QQuick3DModel *>(list->object);
    self->m_materials.push_back({ material, false });
    self->markDirty(QQuick3DModel::MaterialsDirty);

    if (material->parentItem() == nullptr) {
        // If the material has no parent, check if it has a hierarchical parent that's a QQuick3DObject
        // and re-parent it to that, e.g., inline materials
        QQuick3DObject *parentItem = qobject_cast<QQuick3DObject *>(material->parent());
        if (parentItem) {
            material->setParentItem(parentItem);
        } else { // If no valid parent was found, make sure the material refs our scene manager
            const auto &sceneManager = QQuick3DObjectPrivate::get(self)->sceneManager;
            if (sceneManager) {
                QQuick3DObjectPrivate::get(material)->refSceneManager(*sceneManager);
                // Have to keep track if we called refSceneManager because we
                // can end up in double deref attempts when a model is going
                // away, due to updateSceneManager() being called on
                // ItemSceneChanged (and also doing deref). We must ensure that
                // is one deref for each ref.
                self->m_materials.last().refed = true;
            }
            // else: If there's no scene manager, defer until one is set, see itemChange()
        }
    }

    // Make sure materials are removed when destroyed
    connect(material, &QQuick3DMaterial::destroyed, self, &QQuick3DModel::onMaterialDestroyed);
}

QQuick3DMaterial *QQuick3DModel::qmlMaterialAt(QQmlListProperty<QQuick3DMaterial> *list, qsizetype index)
{
    QQuick3DModel *self = static_cast<QQuick3DModel *>(list->object);
    return self->m_materials.at(index).material;
}

qsizetype QQuick3DModel::qmlMaterialsCount(QQmlListProperty<QQuick3DMaterial> *list)
{
    QQuick3DModel *self = static_cast<QQuick3DModel *>(list->object);
    return self->m_materials.count();
}

void QQuick3DModel::qmlClearMaterials(QQmlListProperty<QQuick3DMaterial> *list)
{
    QQuick3DModel *self = static_cast<QQuick3DModel *>(list->object);
    for (Material &mat : self->m_materials) {
        if (mat.material->parentItem() == nullptr) {
            if (mat.refed) {
                QQuick3DObjectPrivate::get(mat.material)->derefSceneManager();
                mat.refed = false;
            }
        }
        mat.material->disconnect(self, SLOT(onMaterialDestroyed(QObject*)));
    }
    self->m_materials.clear();
    self->markDirty(QQuick3DModel::MaterialsDirty);
}

void QQuick3DModel::onMorphTargetDestroyed(QObject *object)
{
    if (m_morphTargets.removeAll(static_cast<QQuick3DMorphTarget *>(object)) > 0) {
        markDirty(QQuick3DModel::MorphTargetsDirty);
        m_numMorphAttribs = 0;
    }
}

void QQuick3DModel::qmlAppendMorphTarget(QQmlListProperty<QQuick3DMorphTarget> *list, QQuick3DMorphTarget *morphTarget)
{
    if (morphTarget == nullptr)
        return;
    QQuick3DModel *self = static_cast<QQuick3DModel *>(list->object);
    if (self->m_numMorphAttribs >= 8) {
        qWarning("The number of morph attributes exceeds 8. This morph target will be ignored.");
        return;
    }
    self->m_morphTargets.push_back(morphTarget);
    self->m_numMorphAttribs += morphTarget->numAttribs();
    if (self->m_numMorphAttribs > 8)
        qWarning("The number of morph attributes exceeds 8. This morph target will be supported partially.");

    self->markDirty(QQuick3DModel::MorphTargetsDirty);

    if (morphTarget->parentItem() == nullptr) {
        // If the morphTarget has no parent, check if it has a hierarchical parent that's a QQuick3DObject
        // and re-parent it to that, e.g., inline morphTargets
        QQuick3DObject *parentItem = qobject_cast<QQuick3DObject *>(morphTarget->parent());
        if (parentItem) {
            morphTarget->setParentItem(parentItem);
        } else { // If no valid parent was found, make sure the morphTarget refs our scene manager
            const auto &scenManager = QQuick3DObjectPrivate::get(self)->sceneManager;
            if (scenManager)
                QQuick3DObjectPrivate::get(morphTarget)->refSceneManager(*scenManager);
            // else: If there's no scene manager, defer until one is set, see itemChange()
        }
    }

    // Make sure morphTargets are removed when destroyed
    connect(morphTarget, &QQuick3DMorphTarget::destroyed, self, &QQuick3DModel::onMorphTargetDestroyed);
}

QQuick3DMorphTarget *QQuick3DModel::qmlMorphTargetAt(QQmlListProperty<QQuick3DMorphTarget> *list, qsizetype index)
{
    QQuick3DModel *self = static_cast<QQuick3DModel *>(list->object);
    if (index >= self->m_morphTargets.size()) {
        qWarning("The index exceeds the range of valid morph targets.");
        return nullptr;
    }
    return self->m_morphTargets.at(index);
}

qsizetype QQuick3DModel::qmlMorphTargetsCount(QQmlListProperty<QQuick3DMorphTarget> *list)
{
    QQuick3DModel *self = static_cast<QQuick3DModel *>(list->object);
    return self->m_morphTargets.count();
}

void QQuick3DModel::qmlClearMorphTargets(QQmlListProperty<QQuick3DMorphTarget> *list)
{
    QQuick3DModel *self = static_cast<QQuick3DModel *>(list->object);
    for (const auto &morph : qAsConst(self->m_morphTargets)) {
        if (morph->parentItem() == nullptr)
            QQuick3DObjectPrivate::get(morph)->derefSceneManager();
        morph->disconnect(self, SLOT(onMorphTargetDestroyed(QObject*)));
    }
    self->m_morphTargets.clear();
    self->m_numMorphAttribs = 0;
    self->markDirty(QQuick3DModel::MorphTargetsDirty);
}

QT_END_NAMESPACE
