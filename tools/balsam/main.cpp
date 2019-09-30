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

#include <QtCore/QCoreApplication>
#include <QtCore/QCommandLineParser>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QVariant>

#include <QtQuick3DAssetImport/private/qssgassetimportmanager_p.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Setup command line arguments
    QCommandLineParser cmdLineParser;
    cmdLineParser.addHelpOption();
    cmdLineParser.addPositionalArgument(QLatin1String("sourceFilename"), QObject::tr("Asset file to be imported"));
    QCommandLineOption outputPathOption({"outputPath", "o"},
                                        QObject::tr("Sets the location to place the generated file(s). Default is the current directory"),
                                        QObject::tr("outputPath"), QDir::currentPath());
    cmdLineParser.addOption(outputPathOption);
    cmdLineParser.process(app);

    QStringList assetFileNames = cmdLineParser.positionalArguments();
    QDir outputDirectory = QDir::currentPath();
    if (cmdLineParser.isSet(outputPathOption)) {
        outputDirectory = QDir(cmdLineParser.value(outputPathOption));
        if (!outputDirectory.exists()) {
            if (!outputDirectory.mkpath(QStringLiteral("."))) {
                qWarning() << "Failed to create export directory: " << outputDirectory;
            }
        }
    }

    // if there is nothing to do return early
    if (assetFileNames.isEmpty())
        return 0;

    QSSGAssetImportManager assetImporter;

    // Convert each assetFile is possible
    for (const auto &assetFileName : assetFileNames) {
        QString errorString;
        QVariantMap options = assetImporter.getOptionsForFile(assetFileName);
        if (assetImporter.importFile(assetFileName, outputDirectory, options, &errorString) != QSSGAssetImportManager::ImportState::Success)
            qWarning() << "Failed to import file with error: " << errorString;
    }

    return 0;
}
