// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSSGGLTFRESOURCERESOLVER_P_H
#define QSSGGLTFRESOURCERESOLVER_P_H

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

#include <QtQuick3DGltf/qtquick3dgltfexports.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

namespace QSSGGltfResourceResolver {

Q_QUICK3DGLTF_EXPORT bool isDataUri(QStringView uri);
Q_QUICK3DGLTF_EXPORT QByteArray decodeDataUri(QStringView uri, QString *errorMessage = nullptr);
Q_QUICK3DGLTF_EXPORT QString resolveFilePath(QStringView uri, const QString &baseDir);
Q_QUICK3DGLTF_EXPORT QByteArray loadUri(QStringView uri, const QString &baseDir, QString *errorMessage = nullptr);

} // namespace QSSGGltfResourceResolver

QT_END_NAMESPACE

#endif // QSSGGLTFRESOURCERESOLVER_P_H
