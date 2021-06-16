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

#ifndef QSSGMODEL_H
#define QSSGMODEL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dmaterial_p.h>
#include <QtQuick3D/private/qquick3dgeometry_p.h>
#include <QtQuick3D/private/qquick3dinstancing_p.h>
#include <QtQuick3D/private/qquick3dskeleton_p.h>
#include <QtQuick3D/private/qquick3dmorphtarget_p.h>
#include <QtQuick3DUtils/private/qssgbounds3_p.h>

#include <QtQml/QQmlListProperty>

#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QUrl>

#include <QtGui/QVector4D>

QT_BEGIN_NAMESPACE

class Q_QUICK3D_EXPORT QQuick3DBounds3
{
    Q_GADGET
    Q_PROPERTY(QVector3D minimum READ minimum CONSTANT)
    Q_PROPERTY(QVector3D maximum READ maximum CONSTANT)

public:
    QVector3D minimum() const
    {
        return bounds.minimum;
    }

    QVector3D maximum() const
    {
        return bounds.maximum;
    }

    // For compatibility we start with zero values.
    QSSGBounds3 bounds { QVector3D{}, QVector3D{} };
};

class Q_QUICK3D_EXPORT QQuick3DModel : public QQuick3DNode
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(bool castsShadows READ castsShadows WRITE setCastsShadows NOTIFY castsShadowsChanged)
    Q_PROPERTY(bool receivesShadows READ receivesShadows WRITE setReceivesShadows NOTIFY receivesShadowsChanged)
    Q_PROPERTY(QQmlListProperty<QQuick3DMaterial> materials READ materials)
    Q_PROPERTY(QQmlListProperty<QQuick3DMorphTarget> morphTargets READ morphTargets NOTIFY morphTargetsChanged)
    Q_PROPERTY(bool pickable READ pickable WRITE setPickable NOTIFY pickableChanged)
    Q_PROPERTY(QQuick3DGeometry *geometry READ geometry WRITE setGeometry NOTIFY geometryChanged)
    Q_PROPERTY(QQuick3DInstancing *instancing READ instancing WRITE setInstancing NOTIFY instancingChanged)
    Q_PROPERTY(QQuick3DNode *instanceRoot READ instanceRoot WRITE setInstanceRoot NOTIFY instanceRootChanged)
    Q_PROPERTY(QQuick3DSkeleton *skeleton READ skeleton WRITE setSkeleton NOTIFY skeletonChanged)
    Q_PROPERTY(QList<QMatrix4x4> inverseBindPoses READ inverseBindPoses WRITE setInverseBindPoses NOTIFY inverseBindPosesChanged)
    Q_PROPERTY(QQuick3DBounds3 bounds READ bounds NOTIFY boundsChanged)
    Q_PROPERTY(float depthBias READ depthBias WRITE setDepthBias NOTIFY depthBiasChanged)

    QML_NAMED_ELEMENT(Model)

public:
    explicit QQuick3DModel(QQuick3DNode *parent = nullptr);
    ~QQuick3DModel() override;

    QUrl source() const;
    bool castsShadows() const;
    bool receivesShadows() const;
    bool pickable() const;
    QQuick3DGeometry *geometry() const;
    QQuick3DSkeleton *skeleton() const;
    QList<QMatrix4x4> inverseBindPoses() const;
    QQuick3DBounds3 bounds() const;
    float depthBias() const;

    QQmlListProperty<QQuick3DMaterial> materials();
    QQmlListProperty<QQuick3DMorphTarget> morphTargets();

    QQuick3DInstancing *instancing() const;
    QQuick3DNode *instanceRoot() const;

public Q_SLOTS:
    void setSource(const QUrl &source);
    void setCastsShadows(bool castsShadows);
    void setReceivesShadows(bool receivesShadows);
    void setPickable(bool pickable);
    void setGeometry(QQuick3DGeometry *geometry);
    void setSkeleton(QQuick3DSkeleton *skeleton);
    void setInverseBindPoses(const QList<QMatrix4x4> &poses);
    void setBounds(const QVector3D &min, const QVector3D &max);
    void setInstancing(QQuick3DInstancing *instancing);
    void setInstanceRoot(QQuick3DNode *instanceRoot);
    void setDepthBias(float bias);

Q_SIGNALS:
    void sourceChanged();
    void castsShadowsChanged();
    void receivesShadowsChanged();
    void pickableChanged();
    void geometryChanged();
    void skeletonChanged();
    void inverseBindPosesChanged();
    void boundsChanged();
    void instancingChanged();
    void instanceRootChanged();
    void morphTargetsChanged();
    void depthBiasChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void markAllDirty() override;
    void itemChange(ItemChange, const ItemChangeData &) override;

private Q_SLOTS:
    void onMaterialDestroyed(QObject *object);
    void onMorphTargetDestroyed(QObject *object);

private:
    enum QSSGModelDirtyType {
        SourceDirty =            0x00000001,
        MaterialsDirty =         0x00000002,
        ShadowsDirty =           0x00000004,
        PickingDirty =           0x00000008,
        GeometryDirty =          0x00000010,
        SkeletonDirty =          0x00000020,
        PoseDirty =              0x00000040,
        InstancesDirty =         0x00000080,
        MorphTargetsDirty =      0x00000100,
        PropertyDirty =          0x00000200,
    };

    QString translateSource();
    QUrl m_source;

    quint32 m_dirtyAttributes = 0xffffffff; // all dirty by default
    void markDirty(QSSGModelDirtyType type);
    void updateSceneManager(QQuick3DSceneManager *sceneManager);

    static void qmlAppendMaterial(QQmlListProperty<QQuick3DMaterial> *list, QQuick3DMaterial *material);
    static QQuick3DMaterial *qmlMaterialAt(QQmlListProperty<QQuick3DMaterial> *list, qsizetype index);
    static qsizetype qmlMaterialsCount(QQmlListProperty<QQuick3DMaterial> *list);
    static void qmlClearMaterials(QQmlListProperty<QQuick3DMaterial> *list);

    struct Material {
        QQuick3DMaterial *material;
        bool refed;
    };
    QVector<Material> m_materials;

    static void qmlAppendMorphTarget(QQmlListProperty<QQuick3DMorphTarget> *list, QQuick3DMorphTarget *morphTarget);
    static QQuick3DMorphTarget *qmlMorphTargetAt(QQmlListProperty<QQuick3DMorphTarget> *list, qsizetype index);
    static qsizetype qmlMorphTargetsCount(QQmlListProperty<QQuick3DMorphTarget> *list);
    static void qmlClearMorphTargets(QQmlListProperty<QQuick3DMorphTarget> *list);
    QVector<QQuick3DMorphTarget *> m_morphTargets;
    size_t m_numMorphAttribs = 0;
    QQuick3DGeometry *m_geometry = nullptr;
    QQuick3DBounds3 m_bounds;
    QQuick3DSkeleton *m_skeleton = nullptr;
    QList<QMatrix4x4> m_inverseBindPoses;
    QQuick3DInstancing *m_instancing = nullptr;
    QQuick3DNode *m_instanceRoot = nullptr;
    QMetaObject::Connection m_geometryConnection;
    QMetaObject::Connection m_skeletonConnection;
    QMetaObject::Connection m_instancingConnection;
    float m_depthBias = 0.0f;
    bool m_castsShadows = true;
    bool m_receivesShadows = true;
    bool m_pickable = false;

    QHash<QByteArray, QMetaObject::Connection> m_connections;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQuick3DBounds3)

#endif // QSSGMODEL_H
