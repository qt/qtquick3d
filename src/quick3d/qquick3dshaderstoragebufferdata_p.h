// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICK3DSHADERSTORAGEBUFFERDATA_P_H
#define QQUICK3DSHADERSTORAGEBUFFERDATA_P_H

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

#include <QtCore/private/qobject_p.h>
#include <QtQuick3D/qquick3dshaderstoragebufferdata.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3D_EXPORT QQuick3DShaderStorageBufferDataPrivate : public QObjectPrivate
{
public:
    QByteArray bufferData;
};

QT_END_NAMESPACE

#endif
