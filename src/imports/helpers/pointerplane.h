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

#ifndef POINTERPLANE
#define POINTERPLANE

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

#include <QVector3D>
#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3D/private/qtquick3dglobal_p.h>

class PointerPlane : public QQuick3DNode
{
    Q_OBJECT

public:
    PointerPlane(QQuick3DNode *parent = nullptr);

    Q_INVOKABLE QVector3D getIntersectPos(const QVector3D &rayPos0, const QVector3D &rayPos1, const QVector3D &planePos, const QVector3D &planeNormal) const;
    Q_INVOKABLE QVector3D getIntersectPosFromSceneRay(const QVector3D &rayPos0, const QVector3D &rayPos1) const;
    Q_INVOKABLE QVector3D getIntersectPosFromView(QQuick3DViewport *view, const QPointF &posInView) const;

private:
    Q_DISABLE_COPY(PointerPlane)
};

QML_DECLARE_TYPE(PointerPlane)

#endif // POINTERPLANE
