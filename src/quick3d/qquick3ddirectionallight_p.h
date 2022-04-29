/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QSSGDIRECTIONALLIGHT_H
#define QSSGDIRECTIONALLIGHT_H

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

#include <QtQuick3D/private/qquick3dabstractlight_p.h>

#include <QColor>

QT_BEGIN_NAMESPACE

class Q_QUICK3D_EXPORT QQuick3DDirectionalLight : public QQuick3DAbstractLight
{
    Q_OBJECT

    QML_NAMED_ELEMENT(DirectionalLight)

public:
    explicit QQuick3DDirectionalLight(QQuick3DNode *parent = nullptr);
    ~QQuick3DDirectionalLight() override {}

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;

};

QT_END_NAMESPACE
#endif // QSSGDIRECTIONALLIGHT_H
