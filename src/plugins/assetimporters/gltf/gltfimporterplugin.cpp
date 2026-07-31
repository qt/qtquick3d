// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "gltfimporterplugin.h"
#include "gltfimporter.h"

QT_BEGIN_NAMESPACE

QSSGAssetImporter *GltfImporterPlugin::create(const QString &key, const QStringList &paramList)
{
    Q_UNUSED(key);
    Q_UNUSED(paramList);
    return new GltfImporter();
}

QT_END_NAMESPACE
