// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
