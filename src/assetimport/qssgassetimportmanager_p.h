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

#ifndef QSSGASSETIMPORTMANAGER_H
#define QSSGASSETIMPORTMANAGER_H

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
#include <QtCore/QVector>
#include <QtCore/QMap>
#include <QtCore/QDir>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE

class QSSGAssetImporter;
class QQuick3DNode;
namespace QSSGSceneDesc { struct Scene; }

struct QSSGAssetImporterPluginInfo
{
    QString name;
    QStringList inputExtensions;
    QString outputExtension;
    QString type;
    QVariantMap importOptions;
    QString typeDescription;
};

class Q_QUICK3DASSETIMPORT_EXPORT QSSGAssetImportManager : public QObject
{
    Q_OBJECT
public:
    explicit QSSGAssetImportManager(QObject *parent = nullptr);
    ~QSSGAssetImportManager();

    enum class ImportState : quint8
    {
        Success,
        IoError,
        Unsupported
    };

    // ### Temp API
    ImportState importFile(const QString &filename,
                           const QDir &outputPath,
                           QString *error = nullptr);
    ImportState importFile(const QString &filename,
                           const QDir &outputPath,
                           const QVariantMap &options = QVariantMap(),
                           QString *error = nullptr);
    ImportState importFile(const QUrl &url,
                           QSSGSceneDesc::Scene &scene,
                           QString *error = nullptr);
    QVariantMap getOptionsForFile(const QString &filename);
    QHash<QString, QVariantMap> getAllOptions() const;
    QHash<QString, QStringList> getSupportedExtensions() const;
    QList<QSSGAssetImporterPluginInfo> getImporterPluginInfos() const;

private:
    QVector<QSSGAssetImporter *> m_assetImporters;
    QMap<QString, QSSGAssetImporter *> m_extensionsMap;
};

QT_END_NAMESPACE

#endif // QSSGASSETIMPORTMANAGER_H
