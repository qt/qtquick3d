// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DEXTENSIONHELPERS_P_H
#define QQUICK3DEXTENSIONHELPERS_P_H

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

#endif // QQUICK3DEXTENSIONHELPERS_P_H
