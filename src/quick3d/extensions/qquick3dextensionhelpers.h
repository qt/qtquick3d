// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DEXTENSIONHELPERS_H
#define QQUICK3DEXTENSIONHELPERS_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QtQuick3D API, with limited compatibility guarantees.
// Usage of this API may make your code source and binary incompatible with
// future versions of Qt.
//

#include <QtQuick3D/qtquick3dglobal.h>

QT_BEGIN_NAMESPACE

using QSSGNodeId = quintptr;
using QSSGResourceId = quintptr;

class QQuick3DObject;

class Q_QUICK3D_EXPORT QQuick3DExtensionHelpers
{
public:
    QQuick3DExtensionHelpers();

    [[nodiscard]] static QSSGNodeId getNodeId(const QQuick3DObject &node);
    [[nodiscard]] static QSSGResourceId getResourceId(const QQuick3DObject &resource);
};

QT_END_NAMESPACE

#endif // QQUICK3DEXTENSIONHELPERS_H
