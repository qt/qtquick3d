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

#ifndef QSSGCAMERA_H
#define QSSGCAMERA_H

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

QT_BEGIN_NAMESPACE

struct QSSGRenderCamera;
class Q_QUICK3D_EXPORT QQuick3DCamera : public QQuick3DNode
{
    Q_OBJECT
    Q_PROPERTY(float clipNear READ clipNear WRITE setClipNear NOTIFY clipNearChanged)
    Q_PROPERTY(float clipFar READ clipFar WRITE setClipFar NOTIFY clipFarChanged)
    Q_PROPERTY(float fieldOfView READ fieldOfView WRITE setFieldOfView NOTIFY fieldOfViewChanged)
    Q_PROPERTY(bool isFieldOfViewHorizontal READ isFieldOfViewHorizontal WRITE setIsFieldOfViewHorizontal NOTIFY isFieldOfViewHorizontalChanged)
    Q_PROPERTY(QSSGCameraProjectionMode projectionMode READ projectionMode WRITE setProjectionMode NOTIFY projectionModeChanged)
    Q_PROPERTY(bool enableFrustumCulling READ enableFrustumCulling WRITE setEnableFrustumCulling NOTIFY enableFrustumCullingChanged)
    // Frustum Mode
    Q_PROPERTY(float frustumTop READ frustumTop WRITE setFrustumTop NOTIFY frustumTopChanged)
    Q_PROPERTY(float frustumBottom READ frustumBottom WRITE setFrustumBottom NOTIFY frustumBottomChanged)
    Q_PROPERTY(float frustumRight READ frustumRight WRITE setFrustumRight NOTIFY frustumRightChanged)
    Q_PROPERTY(float frustumLeft READ frustumRight WRITE setFrustumLeft NOTIFY frustumLeftChanged)
    // Custom Mode
    Q_PROPERTY(QMatrix4x4 customProjection READ customProjection WRITE setCustomProjection NOTIFY customProjectionChanged)


public:

    enum QSSGCameraProjectionMode {
        Perspective,
        Orthographic,
        Frustum,
        Custom
    };
    Q_ENUM(QSSGCameraProjectionMode)

    QQuick3DCamera();

    float clipNear() const;
    float clipFar() const;
    float fieldOfView() const;
    bool isFieldOfViewHorizontal() const;
    QQuick3DObject::Type type() const override;
    QSSGCameraProjectionMode projectionMode() const;
    bool enableFrustumCulling() const;

    Q_INVOKABLE QVector3D worldToViewport(const QVector3D &worldPos) const;
    Q_INVOKABLE QVector3D viewportToWorld(const QVector3D &viewportPos) const;

    QSSGRenderCamera *getCameraNode() const;

    float frustumTop() const;
    float frustumBottom() const;
    float frustumRight() const;
    float frustumLeft() const;

    QMatrix4x4 customProjection() const;

public Q_SLOTS:
    void setClipNear(float clipNear);
    void setClipFar(float clipFar);
    void setFieldOfView(float fieldOfView);
    void setIsFieldOfViewHorizontal(bool isFieldOFViewHorizontal);
    void setProjectionMode(QSSGCameraProjectionMode projectionMode);
    void setEnableFrustumCulling(bool enableFrustumCulling);

    void setFrustumTop(float frustumTop);
    void setFrustumBottom(float frustumBottom);
    void setFrustumRight(float frustumRight);
    void setFrustumLeft(float frustumLeft);

    void setCustomProjection(QMatrix4x4 customProjection);

Q_SIGNALS:
    void clipNearChanged(float clipNear);
    void clipFarChanged(float clipFar);
    void fieldOfViewChanged(float fieldOfView);
    void isFieldOfViewHorizontalChanged(bool isFieldOfViewHorizontal);
    void projectionModeChanged(QSSGCameraProjectionMode projectionMode);
    void enableFrustumCullingChanged(bool enableFrustumCulling);

    void frustumTopChanged(float frustumTop);
    void frustumBottomChanged(float frustumBottom);
    void frustumRightChanged(float frustumRight);
    void frustumLeftChanged(float frustumLeft);

    void customProjectionChanged(QMatrix4x4 customProjection);

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;

private:
    float m_clipNear = 10.0f;
    float m_clipFar = 10000.0f;
    float m_fieldOfView = 60.0f;
    bool m_isFieldOfViewHorizontal = false;

    QSSGRenderCamera *m_cameraNode = nullptr;
    QSSGCameraProjectionMode m_projectionMode = QSSGCameraProjectionMode::Perspective;
    bool m_enableFrustumCulling = true;

    float m_frustumTop = 0.0f;
    float m_frustumBottom = 0.0f;
    float m_frustumRight = 0.0f;
    float m_frustumLeft = 0.0f;

    QMatrix4x4 m_customProjection;
};

QT_END_NAMESPACE

#endif // QSSGCAMERA_H
