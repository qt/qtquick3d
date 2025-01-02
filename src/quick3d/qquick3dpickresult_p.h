// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DPICKRESULT_P_H
#define QQUICK3DPICKRESULT_P_H

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

#include <QtQuick3D/qquick3dobject.h>
#include <QObject>
#include <QtGui/QVector2D>
#include <QtGui/QVector3D>
#include "qquick3dmodel_p.h"
#include "qquick3dcamera_p.h"

QT_BEGIN_NAMESPACE
class QQuick3DModel;

class Q_QUICK3D_EXPORT QQuick3DPickResult
{
    Q_GADGET
    Q_PROPERTY(QQuick3DModel* objectHit READ objectHit CONSTANT)
    Q_PROPERTY(float distance READ distance CONSTANT)
    Q_PROPERTY(QVector2D uvPosition READ uvPosition CONSTANT)
    Q_PROPERTY(QVector3D scenePosition READ scenePosition CONSTANT)
    Q_PROPERTY(QVector3D position READ position CONSTANT)
    Q_PROPERTY(QVector3D normal READ normal CONSTANT)
    Q_PROPERTY(QVector3D sceneNormal READ sceneNormal CONSTANT)
    Q_PROPERTY(int instanceIndex READ instanceIndex CONSTANT)

public:

    QQuick3DPickResult();
    explicit QQuick3DPickResult(QQuick3DModel *hitObject,
                                float distanceFromCamera,
                                const QVector2D &uvPosition,
                                const QVector3D &scenePosition,
                                const QVector3D &position,
                                const QVector3D &normal,
                                int instanceIndex);
    QQuick3DModel *objectHit() const;
    float distance() const;
    QVector2D uvPosition() const;
    QVector3D scenePosition() const;
    QVector3D position() const;
    QVector3D normal() const;
    QVector3D sceneNormal() const;
    int instanceIndex() const;

private:
    QQuick3DModel *m_objectHit;
    float m_distance;
    QVector2D m_uvPosition;
    QVector3D m_scenePosition;
    QVector3D m_position;
    QVector3D m_normal;
    int m_instanceIndex;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQuick3DPickResult)

#endif // QQUICK3DPICKRESULT_P_H
