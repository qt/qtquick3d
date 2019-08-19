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

#ifndef QSSGNODE_H
#define QSSGNODE_H

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

#include <QtQuick3D/private/qquick3dobject_p.h>

#include <QtGui/QVector3D>
#include <QtGui/QMatrix4x4>

#include <QtQuick3DRuntimeRender/private/qssgrendereulerangles_p.h>

QT_BEGIN_NAMESPACE
struct QSSGRenderNode;
class Q_QUICK3D_EXPORT QQuick3DNode : public QQuick3DObject
{
    Q_OBJECT
    Q_PROPERTY(float x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(float y READ y WRITE setY NOTIFY yChanged)
    Q_PROPERTY(float z READ z WRITE setZ NOTIFY zChanged)
    Q_PROPERTY(QVector3D rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
    Q_PROPERTY(QVector3D position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(QVector3D scale READ scale WRITE setScale NOTIFY scaleChanged)
    Q_PROPERTY(QVector3D pivot READ pivot WRITE setPivot NOTIFY pivotChanged)
    Q_PROPERTY(float opacity READ localOpacity WRITE setLocalOpacity NOTIFY localOpacityChanged)
    Q_PROPERTY(qint32 boneId READ skeletonId WRITE setSkeletonId NOTIFY skeletonIdChanged)
    Q_PROPERTY(RotationOrder rotationOrder READ rotationOrder WRITE setRotationOrder NOTIFY rotationOrderChanged)
    Q_PROPERTY(Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(QVector3D forward READ forward)
    Q_PROPERTY(QVector3D up READ up)
    Q_PROPERTY(QVector3D right READ right)
    Q_PROPERTY(QVector3D globalPosition READ globalPosition)
    Q_PROPERTY(QVector3D globalRotation READ globalRotation)
    Q_PROPERTY(QVector3D globalScale READ globalScale)
    Q_PROPERTY(QMatrix4x4 globalTransform READ globalTransform NOTIFY globalTransformChanged)

public:
    enum RotationOrder {
        XYZ = EulOrdXYZs,
        YZX = EulOrdYZXs,
        ZXY = EulOrdZXYs,
        XZY = EulOrdXZYs,
        YXZ = EulOrdYXZs,
        ZYX = EulOrdZYXs,
        XYZr = EulOrdXYZr,
        YZXr = EulOrdYZXr,
        ZXYr = EulOrdZXYr,
        XZYr = EulOrdXZYr,
        YXZr = EulOrdYXZr,
        ZYXr = EulOrdZYXr
    };
    Q_ENUM(RotationOrder)

    enum Orientation { LeftHanded = 0, RightHanded };
    Q_ENUM(Orientation)
    QQuick3DNode();
    ~QQuick3DNode() override;

    float x() const;
    float y() const;
    float z() const;
    QVector3D rotation() const;
    QVector3D position() const;
    QVector3D scale() const;
    QVector3D pivot() const;
    float localOpacity() const;
    qint32 skeletonId() const;
    RotationOrder rotationOrder() const;
    Orientation orientation() const;
    bool visible() const;

    inline QQuick3DNode *parentNode() const;

    QVector3D forward() const;
    QVector3D up() const;
    QVector3D right() const;

    QVector3D globalPosition() const;
    QVector3D globalRotation() const;
    QVector3D globalScale() const;
    QMatrix4x4 globalTransform() const;
    QMatrix4x4 globalTransformLeftHanded() const;
    QMatrix4x4 globalTransformRightHanded() const;

    QQuick3DObject::Type type() const override;

public Q_SLOTS:
    void setX(float x);
    void setY(float y);
    void setZ(float z);
    void setRotation(QVector3D rotation);
    void setPosition(QVector3D position);
    void setScale(QVector3D scale);
    void setPivot(QVector3D pivot);
    void setLocalOpacity(float opacity);
    void setSkeletonId(qint32 boneid);
    void setRotationOrder(RotationOrder rotationorder);
    void setOrientation(Orientation orientation);
    void setVisible(bool visible);

Q_SIGNALS:
    void xChanged(float x);
    void yChanged(float y);
    void zChanged(float z);
    void rotationChanged(QVector3D rotation);
    void positionChanged(QVector3D position);
    void scaleChanged(QVector3D scale);
    void pivotChanged(QVector3D pivot);
    void localOpacityChanged(float opacity);
    void skeletonIdChanged(qint32 boneid);
    void rotationOrderChanged(RotationOrder rotationorder);
    void orientationChanged(Orientation orientation);
    void visibleChanged(bool visible);
    void globalTransformChanged(QMatrix4x4 transform);

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;

private:
    QVector3D m_rotation;
    QVector3D m_position;
    QVector3D m_scale{ 1.0f, 1.0f, 1.0f };
    QVector3D m_pivot;
    float m_opacity = 1.0f;
    qint32 m_boneid = -1;
    RotationOrder m_rotationorder = YXZ;
    Orientation m_orientation = LeftHanded;
    bool m_visible = true;
    QMatrix4x4 m_globalTransformRightHanded;

    QMatrix4x4 calculateLocalTransformRightHanded();
    void calculateGlobalVariables();

    friend QQuick3DSceneManager;
};

QT_END_NAMESPACE

#endif // QSSGNODE_H
