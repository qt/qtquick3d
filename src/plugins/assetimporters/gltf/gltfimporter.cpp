// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "gltfimporter.h"
#include "gltfsceneconverter.h"

#include <QtQuick3DAssetUtils/private/qssgqmlutilities_p.h>
#include <QtQuick3DAssetUtils/private/qssgsceneedit_p.h>
#include <QtQuick3DGltf/private/qssggltfparser_p.h>

#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qjsondocument.h>
#include <QtQml/qqmlfile.h>

QT_BEGIN_NAMESPACE

GltfImporter::GltfImporter()
{
    QFile optionFile(QStringLiteral(":/gltfimporter/options.json"));
    if (optionFile.open(QIODevice::ReadOnly)) {
        const QByteArray options = optionFile.readAll();
        m_options = QJsonDocument::fromJson(options).object();
    }
}

QString GltfImporter::name() const
{
    return QStringLiteral("gltf");
}

QStringList GltfImporter::inputExtensions() const
{
    if (qEnvironmentVariableIsSet("QT_QUICK3D_DISABLE_NATIVE_GLTF"))
        return {};
    return { QStringLiteral("gltf"), QStringLiteral("glb") };
}

QString GltfImporter::outputExtension() const
{
    return QStringLiteral(".qml");
}

QString GltfImporter::type() const
{
    return QStringLiteral("Scene");
}

QString GltfImporter::typeDescription() const
{
    return QObject::tr("glTF 2.0 Scene");
}

int GltfImporter::priority() const
{
    // Above the Assimp plugin, which also claims gltf/glb
    return 10;
}

QJsonObject GltfImporter::importOptions() const
{
    return m_options;
}

static QString importImp(const QUrl &url, const QJsonObject &options, QSSGSceneDesc::Scene &targetScene)
{
    // Resolved the same way as in QSSGAssetImportManager
    const QString filePath = QQmlFile::isLocalFile(url) ? QQmlFile::urlToLocalFileOrQrc(url) : url.path();

    const QFileInfo sourceFile(filePath);
    if (!sourceFile.exists())
        return QLatin1String("File not found");

    QSSGGltfParser parser;
    QSSGGltfDocument document;
    if (!parser.parseFile(filePath, &document))
        return parser.errorMessage();

    GltfSceneConverter converter;
    const QString error = converter.convert(document, options, sourceFile, targetScene);
    if (!error.isEmpty())
        return error;

    QSSGQmlUtilities::applyEdit(&targetScene, options);

    return QString();
}

QString GltfImporter::import(const QUrl &url, const QJsonObject &options, QSSGSceneDesc::Scene &scene)
{
    return importImp(url, options, scene);
}

QString GltfImporter::import(const QString &sourceFile, const QDir &savePath, const QJsonObject &options,
                             QStringList *generatedFiles)
{
    QString errorString;

    QSSGSceneDesc::Scene scene;

    const auto sourceUrl = QUrl::fromLocalFile(sourceFile);
    errorString = importImp(sourceUrl, options, scene);

    if (!errorString.isEmpty()) {
        // Scene has no destructor
        scene.cleanup();
        return errorString;
    }

    QFileInfo sourceFileInfo(sourceFile);

    QString targetFileName = savePath.absolutePath() + QDir::separator() +
            QSSGQmlUtilities::qmlComponentName(sourceFileInfo.completeBaseName()) +
            QStringLiteral(".qml");
    QFile targetFile(targetFileName);
    if (!targetFile.open(QIODevice::WriteOnly)) {
        errorString += QStringLiteral("Could not write to file: ") + targetFileName;
    } else {
        QTextStream output(&targetFile);
        QSSGQmlUtilities::writeQml(scene, output, savePath, options);
        if (generatedFiles)
            generatedFiles->append(targetFileName);
    }
    scene.cleanup();

    return errorString;
}

QT_END_NAMESPACE
