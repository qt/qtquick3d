// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSSGGLTFACCESSORREADER_P_H
#define QSSGGLTFACCESSORREADER_P_H

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

#include <QtQuick3DGltf/private/qssggltfdocument_p.h>

QT_BEGIN_NAMESPACE

namespace QSSGGltfAccessorReader {

Q_QUICK3DGLTF_EXPORT QByteArray readPacked(const QSSGGltfDocument &document, int accessorIndex);
Q_QUICK3DGLTF_EXPORT QList<float> readAsFloats(const QSSGGltfDocument &document, int accessorIndex);
Q_QUICK3DGLTF_EXPORT QList<quint32> readIndices(const QSSGGltfDocument &document, int accessorIndex);

} // namespace QSSGGltfAccessorReader

QT_END_NAMESPACE

#endif // QSSGGLTFACCESSORREADER_P_H
