/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef ASSIMPIMPORTERPLUGIN_H
#define ASSIMPIMPORTERPLUGIN_H

#include <QtQuick3DAssetImport/private/qssgassetimporterplugin_p.h>

QT_BEGIN_NAMESPACE

class AssimpImporterPlugin : public QSSGAssetImporterPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QSSGAssetImporterFactoryInterface_iid FILE "assimp.json")

public:
    QSSGAssetImporter *create(const QString &key, const QStringList &paramList) override;
};

QT_END_NAMESPACE


#endif // ASSIMPIMPORTERPLUGIN_H
