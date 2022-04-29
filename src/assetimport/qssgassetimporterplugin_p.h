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

#ifndef QSSGASSETIMPORTERPLUGIN_P_H
#define QSSGASSETIMPORTERPLUGIN_P_H

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

#include <QtQuick3DAssetImport/private/qtquick3dassetimportglobal_p.h>

#include <QtCore/QObject>

#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>

QT_BEGIN_NAMESPACE

class QSSGAssetImporter;

#define QSSGAssetImporterFactoryInterface_iid "org.qt-project.QtDemon.AssetImporter.5.12"

class Q_QUICK3DASSETIMPORT_EXPORT QSSGAssetImporterPlugin : public QObject
{
    Q_OBJECT
public:
    virtual QSSGAssetImporter *create(const QString &key, const QStringList &paramList) = 0;
};

QT_END_NAMESPACE

#endif // QSSGASSETIMPORTERPLUGIN_P_H
