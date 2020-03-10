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

#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermodel_p.h>

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
    supported through the asset import tool \l {Balsam}.

    The built-in primitives can be loaded by setting the \c source property to one of these values:
    \c {#Rectangle, #Sphere, #Cube, #Cylinder or #Cone}.

    \qml
    Model {
        source: "#Sphere"
    }
    \endqml

    \section1 Materials

    A model can consist of several sub-meshes, each of which can have its own material.
    The sub-mess uses a material from the \l{materials} list, corresponding to its index.
    If the number of materials is less than the sub-meshes, the last material in the list is used
    for subsequent sub-meshes.

    There are currently three different materials that can be used with the model item,
    the \l {PrincipledMaterial}, the \l {DefaultMaterial}, and the \l {CustomMaterial}.
    In addition the \l {Qt Quick 3D Materials QML Types}{Material Library} provides a set of
    pre-made materials that can be used.
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
    auto matList = materials();
    qmlClearMaterials(&matList);
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
    \qmlproperty enumeration Model::tessellationMode

    This property defines what method to use to dynamically generate additional
    geometry for the model. Tessellation is useful if you are using a
    displacement map with your geometry, or if you wish to generate a smoother
    silhouette when zooming in.

    \value Model.NoTessellation No tessellation is used. This is the default.
    \value Model.Linear Tessellation uses linear generation.
    \value Model.Phong Tessellation uses Phong generation.
    \value Model.NPatch Tessellation uses NPatch generation.
*/

QQuick3DModel::QSSGTessellationModeValues QQuick3DModel::tessellationMode() const
{
    return m_tessellationMode;
}

/*!
    \qmlproperty real Model::edgeTessellation

    This property defines the edge multiplier to the tessellation generator.
*/

float QQuick3DModel::edgeTessellation() const
{
    return m_edgeTessellation;
}

/*!
    \qmlproperty real Model::innerTessellation

    This property defines the inner multiplier to the tessellation generator.
*/

float QQuick3DModel::innerTessellation() const
{
    return m_innerTessellation;
}

/*!
    \qmlproperty bool Model::isWireframeMode

    When this property is \c true and the tessellationMode is not
    Model.NoTessellation, a wireframe is displayed to highlight the additional
    geometry created by the tessellation generator.
*/

bool QQuick3DModel::isWireframeMode() const
{
    return m_isWireframeMode;
}

/*!
    \qmlproperty List<QtQuick3D::Material> Model::materials

    This property contains a list of materials used to render the provided
    geometry. To render anything, there must be at least one material. Normally
    there should be one material for each sub-mesh included in the source
    geometry.
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

    When this property is \c true, shadows can be cast onto this item. So the
    shadow map is applied to this model by the renderer.
*/

bool QQuick3DModel::receivesShadows() const
{
    return m_receivesShadows;
}

/*!
    \qmlproperty bool Model::pickable

    This property controls whether the model is pickable or not. By default models are not pickable
    and therefore not included when \l {View3D::pick} {picking} against the scene.
*/
bool QQuick3DModel::pickable() const
{
    return m_pickable;
}

/*!
    \qmlproperty Geometry Model::geometry

    Specify custom geometry for the model. The Model::source must be empty when custom geometry
    is used.
*/
QQuick3DGeometry *QQuick3DModel::geometry() const
{
    return m_geometry;
}

/*!
    \qmlproperty Bounds Model::bounds

    This holds the bounds of the model. It can be read from the model that is set as a \l source.

    \note Bounds might not be immediately available since the source might have not been loaded.

    \readonly
*/
QQuick3DBounds3 QQuick3DModel::bounds() const
{
    return m_bounds;
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

void QQuick3DModel::setTessellationMode(QQuick3DModel::QSSGTessellationModeValues tessellationMode)
{
    if (m_tessellationMode == tessellationMode)
        return;

    m_tessellationMode = tessellationMode;
    emit tessellationModeChanged();
    markDirty(TessellationModeDirty);
}

void QQuick3DModel::setEdgeTessellation(float edgeTessellation)
{
    if (qFuzzyCompare(m_edgeTessellation, edgeTessellation))
        return;

    m_edgeTessellation = edgeTessellation;
    emit edgeTessellationChanged();
    markDirty(TessellationEdgeDirty);
}

void QQuick3DModel::setInnerTessellation(float innerTessellation)
{
    if (qFuzzyCompare(m_innerTessellation, innerTessellation))
        return;

    m_innerTessellation = innerTessellation;
    emit innerTessellationChanged();
    markDirty(TessellationInnerDirty);
}

void QQuick3DModel::setIsWireframeMode(bool isWireframeMode)
{
    if (m_isWireframeMode == isWireframeMode)
        return;

    m_isWireframeMode = isWireframeMode;
    emit isWireframeModeChanged();
    markDirty(WireframeDirty);
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
    if (m_geometry)
        QObject::disconnect(m_geometryConnection);
    m_geometry = geometry;
    m_geometryConnection
            = QObject::connect(m_geometry, &QQuick3DGeometry::geometryNodeDirty, [this]() {
        markDirty(GeometryDirty);
    });
    emit geometryChanged();
    markDirty(GeometryDirty);
}

void QQuick3DModel::setBounds(const QVector3D &min, const QVector3D &max)
{
    if (!qFuzzyCompare(m_bounds.m_maximum, max)
            || !qFuzzyCompare(m_bounds.m_minimum, min))  {
        m_bounds.m_maximum = max;
        m_bounds.m_minimum = min;
        emit boundsChanged();
    }
}

static QSSGRenderGraphObject *getMaterialNodeFromQSSGMaterial(QQuick3DMaterial *material)
{
    QQuick3DObjectPrivate *p = QQuick3DObjectPrivate::get(material);
    return p->spatialNode;
}

void QQuick3DModel::itemChange(ItemChange change, const ItemChangeData &value)
{
    if (change == QQuick3DObject::ItemSceneChange) {
        if (const auto &sceneManager = value.sceneManager) {
            sceneManager->dirtyBoundingBoxList.append(this);
            if (m_geometry)
                QQuick3DObjectPrivate::refSceneManager(m_geometry, sceneManager);
            for (const auto &mat : qAsConst(m_materials)) {
                if (!mat->parentItem() && !QQuick3DObjectPrivate::get(mat)->sceneManager)
                    QQuick3DObjectPrivate::refSceneManager(mat, sceneManager);
            }
        } else {
            if (m_geometry)
                QQuick3DObjectPrivate::derefSceneManager(m_geometry);
        }
    }
}

QSSGRenderGraphObject *QQuick3DModel::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node) {
        markAllDirty();
        node = new QSSGRenderModel();
    }

    QQuick3DNode::updateSpatialNode(node);

    auto modelNode = static_cast<QSSGRenderModel *>(node);
    if (m_dirtyAttributes & SourceDirty)
        modelNode->meshPath = QSSGRenderMeshPath::create(translateSource());
    if (m_dirtyAttributes & TessellationModeDirty)
        modelNode->tessellationMode = TessellationModeValues(m_tessellationMode);
    if (m_dirtyAttributes & TessellationEdgeDirty)
        modelNode->edgeTessellation = m_edgeTessellation;
    if (m_dirtyAttributes & TessellationInnerDirty)
        modelNode->innerTessellation = m_innerTessellation;
    if (m_dirtyAttributes & WireframeDirty)
        modelNode->wireframeMode = m_isWireframeMode;
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
                for (auto material : m_materials) {
                    QSSGRenderGraphObject *graphObject = getMaterialNodeFromQSSGMaterial(material);
                    if (graphObject)
                        modelNode->materials.append(graphObject);
                }
            } else {
                // Hard mode, go through each material and see if they match
                if (modelNode->materials.size() != m_materials.size())
                    modelNode->materials.resize(m_materials.size());
                for (int i = 0; i < m_materials.size(); ++i) {
                    QSSGRenderGraphObject *graphObject = getMaterialNodeFromQSSGMaterial(m_materials[i]);
                    if (modelNode->materials[i] != graphObject)
                        modelNode->materials[i] = graphObject;
                }
            }
        } else {
            // No materials
            modelNode->materials.clear();
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

    m_dirtyAttributes = 0;

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

    return QQmlFile::urlToLocalFileOrQrc(m_source) + fragment;
}

void QQuick3DModel::markDirty(QQuick3DModel::QSSGModelDirtyType type)
{
    if (!(m_dirtyAttributes & quint32(type))) {
        m_dirtyAttributes |= quint32(type);
        update();
    }
}

void QQuick3DModel::onMaterialDestroyed(QObject *object)
{
    if (m_materials.removeAll(static_cast<QQuick3DMaterial *>(object)) > 0)
        markDirty(QQuick3DModel::MaterialsDirty);
}

void QQuick3DModel::qmlAppendMaterial(QQmlListProperty<QQuick3DMaterial> *list, QQuick3DMaterial *material)
{
    if (material == nullptr)
        return;
    QQuick3DModel *self = static_cast<QQuick3DModel *>(list->object);
    self->m_materials.push_back(material);
    self->markDirty(QQuick3DModel::MaterialsDirty);

    if (material->parentItem() == nullptr) {
        // If the material has no parent, check if it has a hierarchical parent that's a QQuick3DObject
        // and re-parent it to that, e.g., inline materials
        QQuick3DObject *parentItem = qobject_cast<QQuick3DObject *>(material->parent());
        if (parentItem) {
            material->setParentItem(parentItem);
        } else { // If no valid parent was found, make sure the material refs our scene manager
            const auto &scenManager = QQuick3DObjectPrivate::get(self)->sceneManager;
            if (scenManager)
                QQuick3DObjectPrivate::get(material)->refSceneManager(scenManager);
            // else: If there's no scene manager, defer until one is set, see itemChange()
        }
    }

    // Make sure materials are removed when destroyed
    connect(material, &QQuick3DMaterial::destroyed, self, &QQuick3DModel::onMaterialDestroyed);
}

QQuick3DMaterial *QQuick3DModel::qmlMaterialAt(QQmlListProperty<QQuick3DMaterial> *list, int index)
{
    QQuick3DModel *self = static_cast<QQuick3DModel *>(list->object);
    return self->m_materials.at(index);
}

int QQuick3DModel::qmlMaterialsCount(QQmlListProperty<QQuick3DMaterial> *list)
{
    QQuick3DModel *self = static_cast<QQuick3DModel *>(list->object);
    return self->m_materials.count();
}

void QQuick3DModel::qmlClearMaterials(QQmlListProperty<QQuick3DMaterial> *list)
{
    QQuick3DModel *self = static_cast<QQuick3DModel *>(list->object);
    for (const auto &mat : qAsConst(self->m_materials)) {
        if (mat->parentItem() == nullptr)
            QQuick3DObjectPrivate::get(mat)->derefSceneManager();
        mat->disconnect(self, SLOT(onMaterialDestroyed(QObject*)));
    }
    self->m_materials.clear();
    self->markDirty(QQuick3DModel::MaterialsDirty);
}

QT_END_NAMESPACE
