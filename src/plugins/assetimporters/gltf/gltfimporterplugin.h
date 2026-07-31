// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#pragma once

#include <QtQuick3DAssetImport/private/qssgassetimporterplugin_p.h>

QT_BEGIN_NAMESPACE

class GltfImporterPlugin : public QSSGAssetImporterPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QSSGAssetImporterFactoryInterface_iid FILE "gltf.json")

public:
    QSSGAssetImporter *create(const QString &key, const QStringList &paramList) override;
};

QT_END_NAMESPACE
