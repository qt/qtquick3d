/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QSSGCUSTOMCAMERA_H
#define QSSGCUSTOMCAMERA_H

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

#include <QtQuick3D/private/qquick3dcamera_p.h>

QT_BEGIN_NAMESPACE

struct QSSGRenderCamera;
class Q_QUICK3D_EXPORT QQuick3DCustomCamera : public QQuick3DCamera
{
    Q_OBJECT
    Q_PROPERTY(QMatrix4x4 projection READ projection WRITE setProjection NOTIFY projectionChanged)

public:
    QQuick3DCustomCamera();

    QMatrix4x4 projection() const;

public Q_SLOTS:
    void setProjection(const QMatrix4x4 &projection);

Q_SIGNALS:
    void projectionChanged();

protected:
    bool checkSpatialNode(QSSGRenderCamera *camera) override;

private:
    QMatrix4x4 m_projection;
};

QT_END_NAMESPACE

#endif // QSSGCUSTOMCAMERA_H
