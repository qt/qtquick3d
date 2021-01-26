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

QT_BEGIN_NAMESPACE
class QQuick3DModel;

class Q_QUICK3D_EXPORT QQuick3DPickResult
{
    Q_GADGET
    Q_PROPERTY(QQuick3DModel* objectHit READ objectHit CONSTANT)
    Q_PROPERTY(float distance READ distance CONSTANT)
    Q_PROPERTY(QVector2D uvPosition READ uvPosition CONSTANT)
    Q_PROPERTY(QVector3D scenePosition READ scenePosition CONSTANT)

public:

    QQuick3DPickResult();
    explicit QQuick3DPickResult(QQuick3DModel *hitObject,
                                float distanceFromCamera,
                                const QVector2D &uvPosition,
                                const QVector3D &scenePosition);
    QQuick3DPickResult (const QQuick3DPickResult &obj);
    QQuick3DPickResult& operator=(const QQuick3DPickResult&) = default;

    ~QQuick3DPickResult();

    QQuick3DModel *objectHit() const;
    float distance() const;
    QVector2D uvPosition() const;
    QVector3D scenePosition() const;

private:
    QQuick3DModel *m_objectHit;
    float m_distance;
    QVector2D m_uvPosition;
    QVector3D m_scenePosition;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQuick3DPickResult)

#endif // QQUICK3DPICKRESULT_P_H
