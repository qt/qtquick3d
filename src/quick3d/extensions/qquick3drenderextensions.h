// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DRENDEREXTENSIONS_H
#define QQUICK3DRENDEREXTENSIONS_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QtQuick3D API, with limited compatibility guarantees.
// Usage of this API may make your code source and binary incompatible with
// future versions of Qt.
//

#include <QtCore/qobject.h>
#include <QtQuick3D/qquick3dobject.h>

QT_BEGIN_NAMESPACE

class QSSGRenderer;
class QSSGLayerRenderData;

class Q_QUICK3D_EXPORT QQuick3DRenderExtension : public QQuick3DObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(RenderExtension)
    QML_UNCREATABLE("RenderExtension is an abstract type")
    QML_ADDED_IN_VERSION(6, 6)
public:
    explicit QQuick3DRenderExtension(QQuick3DObject *parent = nullptr);
    virtual ~QQuick3DRenderExtension();
};

QT_END_NAMESPACE

#endif // QQUICK3DRENDEREXTENSIONS_H
