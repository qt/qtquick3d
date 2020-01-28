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
#include <QtCore/QHash>

#include <QtCore/QJsonObject>

#include <QtQuick3DAssetImport/private/qssgassetimportmanager_p.h>

class OptionsManager {
public:
    OptionsManager() {

    }
    ~OptionsManager() {
        qDeleteAll(m_optionsMap.values());
        m_optionsMap.clear();
    }
    void generateCommandLineOptions(const QVariantMap &optionsMap)
    {
        QJsonObject options = QJsonObject::fromVariantMap(optionsMap);
        if (options.isEmpty() || !options.contains(QStringLiteral("options")))
            return;

        QJsonObject optionsObject = options.value(QStringLiteral("options")).toObject();
        for (const QString &optionsKey : optionsObject.keys()) {
            QJsonObject option = optionsObject.value(optionsKey).toObject();
            QString optionType = option.value(QStringLiteral("type")).toString();
            QString description = option.value(QStringLiteral("description")).toString();
            if (optionType == QStringLiteral("Boolean")) {
                // boolean flags
                m_optionsMap.insert(optionsKey, new QCommandLineOption(optionsKey, description));
                const QString disableKey = QStringLiteral("disable-") + optionsKey;
                m_optionsMap.insert(disableKey, new QCommandLineOption(QStringLiteral("disable-") + optionsKey));
            } else {
                // value types
                if (optionType == QStringLiteral("Real")) {
                    QString defaultValue = QString::number(option.value("value").toDouble());
                    QCommandLineOption *valueOption = new QCommandLineOption(optionsKey, description, optionsKey, defaultValue);
                    m_optionsMap.insert(optionsKey, valueOption);
                }
            }
        }
    }

    QVariantMap processCommandLineOptions(const QCommandLineParser &cmdLineParser, const QVariantMap &optionsMap) const
    {
        QJsonObject options = QJsonObject::fromVariantMap(optionsMap);
        if (options.isEmpty() || !options.contains(QStringLiteral("options")))
            return optionsMap;

        QJsonObject optionsObject = options.value(QStringLiteral("options")).toObject();
        for (const QString &optionsKey : optionsObject.keys()) {
            QJsonObject option = optionsObject.value(optionsKey).toObject();
            QString optionType = option.value(QStringLiteral("type")).toString();
            if (optionType == QStringLiteral("Boolean")) {
                const QString disableKey = QStringLiteral("disable-") + optionsKey;
                if (m_optionsMap[optionsKey] && cmdLineParser.isSet(*m_optionsMap[optionsKey]))
                    option["value"] = true;
                else if (m_optionsMap[disableKey] && cmdLineParser.isSet(*m_optionsMap[disableKey]))
                    option["value"] = false;
            } else if (optionType == QStringLiteral("Real")) {
                if (cmdLineParser.isSet(optionsKey))
                    option["value"] = cmdLineParser.value(optionsKey).toDouble();
            }
            // update values
            optionsObject[optionsKey] = option;
        }

        removeFlagConflicts(cmdLineParser, optionsObject);
        options["options"] = optionsObject;
        return optionsObject.toVariantMap();
    }
    void registerOptions(QCommandLineParser &parser) {
        for (const auto &cmdLineOption : m_optionsMap.values())
            parser.addOption(*cmdLineOption);
    }

    void removeFlagConflicts(const QCommandLineParser &cmdLineParser, QJsonObject &options) const
    {
        // "generateNormals" and "generateSmoothNormals" are mutually exclusive. "generateNormals"
        // takes precedence.
        QJsonObject opt;
        if (cmdLineParser.isSet(*m_optionsMap[QStringLiteral("generateNormals")])) {
            opt = options.value(QStringLiteral("generateSmoothNormals")).toObject();
            if (opt[QStringLiteral("value")] == true) {
                opt[QStringLiteral("value")] = false;
                options[QStringLiteral("generateSmoothNormals")] = opt;
                qWarning() << "\"--generateSmoothNormals\" disabled due to \"--generateNormals\".";
            }

        } else if (cmdLineParser.isSet(*m_optionsMap[QStringLiteral("generateSmoothNormals")])) {
            opt = options.value(QStringLiteral("generateNormals")).toObject();
            if (opt[QStringLiteral("value")] == true) {
                opt[QStringLiteral("value")] = false;
                options[QStringLiteral("generateNormals")] = opt;
                qWarning() << "\"--generateNormals\" disabled due to \"--generateSmoothNormals\".";
            }
        }

        // Ditto for "optimizeGraph" and "preTransformVertices".
        if (cmdLineParser.isSet(*m_optionsMap[QStringLiteral("optimizeGraph")])) {
            opt = options.value(QStringLiteral("preTransformVertices")).toObject();
            if (opt[QStringLiteral("value")] == true) {
                opt[QStringLiteral("value")] = false;
                options[QStringLiteral("preTransformVertices")] = opt;
                qWarning() << "\"--preTransformVertices\" disabled due to \"--optimizeGraph\".";
            }
        } else if (cmdLineParser.isSet(*m_optionsMap[QStringLiteral("preTransformVertices")])) {
            opt = options.value(QStringLiteral("optimizeGraph")).toObject();
            if (opt[QStringLiteral("value")] == true) {
                opt[QStringLiteral("value")] = false;
                options[QStringLiteral("optimizeGraph")] = opt;
                qWarning() << "\"--optimizeGraph\" disabled due to \"--preTransformVertices\".";
            }
        }
    }

private:
    QHash<QString, QCommandLineOption *> m_optionsMap;
};


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QSSGAssetImportManager assetImporter;
    OptionsManager optionsManager;

    // Setup command line arguments
    QCommandLineParser cmdLineParser;
    cmdLineParser.addHelpOption();
    cmdLineParser.addPositionalArgument(QLatin1String("sourceFilename"), QObject::tr("Asset file to be imported"));
    QCommandLineOption outputPathOption({"outputPath", "o"},
                                        QObject::tr("Sets the location to place the generated file(s). Default is the current directory"),
                                        QObject::tr("outputPath"), QDir::currentPath());
    cmdLineParser.addOption(outputPathOption);

    // Get Plugin options
    auto pluginOptions = assetImporter.getAllOptions();
    for (const auto &options : pluginOptions.values())
        optionsManager.generateCommandLineOptions(options);

    optionsManager.registerOptions(cmdLineParser);

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

    // Convert each assetFile is possible
    for (const auto &assetFileName : assetFileNames) {
        QString errorString;
        QVariantMap options = assetImporter.getOptionsForFile(assetFileName);
        options = optionsManager.processCommandLineOptions(cmdLineParser, options);
        if (assetImporter.importFile(assetFileName, outputDirectory, options, &errorString) != QSSGAssetImportManager::ImportState::Success)
            qWarning() << "Failed to import file with error: " << errorString;
    }

    return 0;
}
