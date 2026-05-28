// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QTQUICK3DASSETUTILSGLOBAL_P_H
#define QTQUICK3DASSETUTILSGLOBAL_P_H

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

#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qhash.h>
#include <QtQuick3DAssetUtils/qtquick3dassetutilsexports.h>
#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>

QT_BEGIN_NAMESPACE

void Q_QUICK3DASSETUTILS_EXPORT qml_register_types_QtQuick3D_AssetUtils();

class QQuick3DObject;

// Key used by QSSRtObjectMap for runtime object lookup.
//
// Only 'name' participates in operator== and qHash, so QMultiHash buckets
// are partitioned by name. The 'path' field is intentionally excluded from
// hash/equality to enable a two-phase lookup: first retrieve all entries
// with a matching name via equal_range(), then filter by 'path' manually.
// Do NOT add 'path' to operator== or qHash — that would break the path-based
// lookup in QQuick3DRuntimeLoader::query().
class QSSGRuntimeObjectNameKey
{
public:
    QString name;
    QString path;

    friend bool operator==(const QSSGRuntimeObjectNameKey &lhs, const QSSGRuntimeObjectNameKey &rhs) noexcept
    {
        return lhs.name == rhs.name;
    }

    friend size_t qHash(const QSSGRuntimeObjectNameKey &key, size_t seed = 0) noexcept
    {
        return qHash(key.name, seed);
    }

};

using QSSGRuntimeObjectTypeKey = QSSGRenderGraphObject::BaseType;

using QSSGRuntimeObjectNameMap = QMultiHash<QSSGRuntimeObjectNameKey, QPointer<QQuick3DObject>>;
using QSSGRuntimeObjectTypeMap = QMultiHash<QSSGRuntimeObjectTypeKey, QPointer<QQuick3DObject>>;

QT_END_NAMESPACE

#endif // QTQUICK3DASSETUTILSGLOBAL_P_H
