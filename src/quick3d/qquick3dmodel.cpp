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
#include "qquick3dobject_p_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderreferencedmaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermodel_p.h>

#include <QtQml/QQmlFile>

QT_BEGIN_NAMESPACE

/*!
   \qmltype Model
   \inherits Node
   \instantiates QQuick3DModel
   \inqmlmodule QtQuick3D
   \brief Lets you load a 3D model data.
*/

QQuick3DModel::QQuick3DModel() {}

QQuick3DModel::~QQuick3DModel() {}

QQuick3DObject::Type QQuick3DModel::type() const
{
    return QQuick3DObject::Model;
}

/*!
 * \qmlproperty url Model::source
 *
 * This property defines the location of the mesh file containing the geometry
 * of this Model
 *
*/

QUrl QQuick3DModel::source() const
{
    return m_source;
}

/*!
 * \qmlproperty int Model::skeletonRoot
 *
 * This property defines the root bone ID for this Models skeleton.
 *
 * \note Skeletal animations are not yet available, so this property does\
 * nothing
*/

int QQuick3DModel::skeletonRoot() const
{
    return m_skeletonRoot;
}

/*!
  \qmlproperty enumeration Model::tesselationMode

  This property defines what method to use to dynamically generate additional
  geometry for the model. Tessellation is useful if you are using a
  displacement map with your geometry, or if you wish to generate a smoother
  silhouette when zooming in.

  \list
  \li \c Model::NoTess
  \li \c Model::TessLinear
  \li \c Model::TessPhong
  \li \c Model::TessNPatch
  \endlist

*/

QQuick3DModel::QSSGTessModeValues QQuick3DModel::tesselationMode() const
{
    return m_tesselationMode;
}

/*!
 * \qmlproperty real Model::edgeTess
 *
 * This property defines the edge multiplier to the tesselation generator.
 *
*/

float QQuick3DModel::edgeTess() const
{
    return m_edgeTess;
}

/*!
 * \qmlproperty real Model::edgeInner
 *
 * This property defines the inner multiplier to the tesselation generator.
 *
*/

float QQuick3DModel::innerTess() const
{
    return m_innerTess;
}

/*!
 * \qmlproperty bool Model::isWireframeMode
 *
 * When this property is /c true, and the Model::tesselationMode is not
 * Model::NoTess, then a wireframe is displayed to highlight the additional
 * geometry created by the tesselation generator.
 *
*/

bool QQuick3DModel::isWireframeMode() const
{
    return m_isWireframeMode;
}

/*!
 * \qmlproperty List<QtQuick3D::Material> Model::materials
 *
 * This property contains a list of materials used to render the provided
 * geometry. To render anything, there must be at least one material. Normally
 * there should be one material for each sub-mesh included in the source
 * geometry.
 *
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
 * \qmlproperty bool Model::castsShadows
 *
 * When this property is enabled, the geometry of this model is used in the
 * when rendering to shadow maps.
 *
*/

bool QQuick3DModel::castsShadows() const
{
    return m_castsShadows;
}

/*!
 * \qmlproperty bool Model::receivesShadows
 *
 * When this property is enabled, shadows can be cast onto this item. So the
 * shadow map is applied to this model by the renderer.
 *
*/

bool QQuick3DModel::receivesShadows() const
{
    return m_receivesShadows;
}

void QQuick3DModel::setSource(const QUrl &source)
{
    if (m_source == source)
        return;

    m_source = source;
    emit sourceChanged(m_source);
    markDirty(SourceDirty);
}

void QQuick3DModel::setSkeletonRoot(int skeletonRoot)
{
    if (m_skeletonRoot == skeletonRoot)
        return;

    m_skeletonRoot = skeletonRoot;
    emit skeletonRootChanged(m_skeletonRoot);
    markDirty(SkeletonRootDirty);
}

void QQuick3DModel::setTesselationMode(QQuick3DModel::QSSGTessModeValues tesselationMode)
{
    if (m_tesselationMode == tesselationMode)
        return;

    m_tesselationMode = tesselationMode;
    emit tesselationModeChanged(m_tesselationMode);
    markDirty(TesselationModeDirty);
}

void QQuick3DModel::setEdgeTess(float edgeTess)
{
    if (qFuzzyCompare(m_edgeTess, edgeTess))
        return;

    m_edgeTess = edgeTess;
    emit edgeTessChanged(m_edgeTess);
    markDirty(TesselationEdgeDirty);
}

void QQuick3DModel::setInnerTess(float innerTess)
{
    if (qFuzzyCompare(m_innerTess, innerTess))
        return;

    m_innerTess = innerTess;
    emit innerTessChanged(m_innerTess);
    markDirty(TesselationInnerDirty);
}

void QQuick3DModel::setIsWireframeMode(bool isWireframeMode)
{
    if (m_isWireframeMode == isWireframeMode)
        return;

    m_isWireframeMode = isWireframeMode;
    emit isWireframeModeChanged(m_isWireframeMode);
    markDirty(WireframeDirty);
}

void QQuick3DModel::setCastsShadows(bool castsShadows)
{
    if (m_castsShadows == castsShadows)
        return;

    m_castsShadows = castsShadows;
    emit castsShadowsChanged(m_castsShadows);
    markDirty(ShadowsDirty);
}

void QQuick3DModel::setReceivesShadows(bool receivesShadows)
{
    if (m_receivesShadows == receivesShadows)
        return;

    m_receivesShadows = receivesShadows;
    emit receivesShadowsChanged(m_receivesShadows);
    markDirty(ShadowsDirty);
}

static QSSGRenderGraphObject *getMaterialNodeFromQSSGMaterial(QQuick3DMaterial *material)
{
    QQuick3DObjectPrivate *p = QQuick3DObjectPrivate::get(material);
    return p->spatialNode;
}

QSSGRenderGraphObject *QQuick3DModel::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node)
        node = new QSSGRenderModel();

    QQuick3DNode::updateSpatialNode(node);

    auto modelNode = static_cast<QSSGRenderModel *>(node);
    if (m_dirtyAttributes & SourceDirty)
        modelNode->meshPath = QSSGRenderMeshPath::create(translateSource());
    if (m_dirtyAttributes & SkeletonRootDirty)
        modelNode->skeletonRoot = m_skeletonRoot;
    if (m_dirtyAttributes & TesselationModeDirty)
        modelNode->tessellationMode = TessModeValues(m_tesselationMode);
    if (m_dirtyAttributes & TesselationEdgeDirty)
        modelNode->edgeTess = m_edgeTess;
    if (m_dirtyAttributes & TesselationInnerDirty)
        modelNode->innerTess = m_innerTess;
    if (m_dirtyAttributes & WireframeDirty)
        modelNode->wireframeMode = m_isWireframeMode;

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

void QQuick3DModel::qmlAppendMaterial(QQmlListProperty<QQuick3DMaterial> *list, QQuick3DMaterial *material)
{
    if (material == nullptr)
        return;
    QQuick3DModel *self = static_cast<QQuick3DModel *>(list->object);
    self->m_materials.push_back(material);
    self->markDirty(QQuick3DModel::MaterialsDirty);

    if(material->parentItem() == nullptr)
        material->setParentItem(self);
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
    self->m_materials.clear();
    self->markDirty(QQuick3DModel::MaterialsDirty);
}

QT_END_NAMESPACE
