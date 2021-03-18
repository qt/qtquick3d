/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qssgassetimportmanager_p.h"

#include "qssgassetimporter_p.h"
#include "qssgassetimporterfactory_p.h"

#include <QtCore/QFile>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

QSSGAssetImportManager::QSSGAssetImportManager(QObject *parent) : QObject(parent)
{
    // load assetimporters
    const QStringList keys = QSSGAssetImporterFactory::keys();
    for (const auto &key : keys) {
        auto importer = QSSGAssetImporterFactory::create(key, QStringList());
        m_assetImporters.append(importer);
        // Add to extension map
        for (const auto &extension : importer->inputExtensions()) {
            m_extensionsMap.insert(extension, importer);
        }
    }
}

QSSGAssetImportManager::~QSSGAssetImportManager()
{
    for (auto importer : m_assetImporters) {
        delete importer;
    }
}

// Compatibility with old API
QSSGAssetImportManager::ImportState QSSGAssetImportManager::importFile(const QString &filename,
                                                                       const QDir &outputPath,
                                                                       QString *error)
{
    return importFile(filename, outputPath, QVariantMap(), error);
}

QSSGAssetImportManager::ImportState QSSGAssetImportManager::importFile(const QString &filename,
                                                                       const QDir &outputPath,
                                                                       const QVariantMap &options,
                                                                       QString *error)
{
    QFileInfo fileInfo(filename);

    // Is this a real file?
    if (!fileInfo.exists()) {
        if (error)
            *error = QStringLiteral("file does not exist");
        return ImportState::IoError;
    }

    // Do we have a importer to load the file?
    const auto extension = fileInfo.suffix().toLower();
    auto importer = m_extensionsMap.value(extension, nullptr);
    if (!importer) {
        if (error)
            *error = QStringLiteral("unsupported file extension %1").arg(extension);
        return ImportState::Unsupported;
    }

    QStringList generatedFiles;
    auto errorString = importer->import(fileInfo.absoluteFilePath(), outputPath, options, &generatedFiles);

    if (!errorString.isEmpty()) {
        if (error) {
            *error = QStringLiteral("%1").arg(errorString);
        }

        return ImportState::IoError;
    }

    // debug output
    for (const auto &file : generatedFiles)
        qDebug() << "generated file: " << file;

    return ImportState::Success;
}

QVariantMap QSSGAssetImportManager::getOptionsForFile(const QString &filename)
{
    QFileInfo fileInfo(filename);

    QVariantMap options;

    // Is this a real file?
    if (fileInfo.exists()) {
        // Do we have a importer to load the file?
        const auto extension = fileInfo.suffix().toLower();
        auto importer = m_extensionsMap.value(extension, nullptr);
        if (importer)
            options = importer->importOptions();
    }

    return options;
}

QHash<QString, QVariantMap> QSSGAssetImportManager::getAllOptions() const
{
    QHash<QString, QVariantMap> options;
    for (const auto importer : m_assetImporters)
        options.insert(importer->inputExtensions().join(':'), importer->importOptions());
    return options;
}

QHash<QString, QStringList> QSSGAssetImportManager::getSupportedExtensions() const
{
    QHash<QString, QStringList> extensionMap;
    for (const auto importer : qAsConst(m_assetImporters))
        extensionMap.insert(importer->typeDescription(), importer->inputExtensions());
    return extensionMap;
}

QT_END_NAMESPACE
