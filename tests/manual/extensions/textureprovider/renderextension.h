// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef RENDEREXTENSION_H
#define RENDEREXTENSION_H

#include <QtQuick3D/QQuick3DRenderExtension>
#include <QQmlEngine>

class RenderExtension : public QQuick3DRenderExtension
{
    Q_OBJECT
    QML_ELEMENT
public:
    RenderExtension();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
};

#endif // RENDEREXTENSION_H
