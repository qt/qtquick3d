/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QCoreApplication>

#include <QXmlStreamWriter>
#include <qlist.h>

#include <QCommandLineOption>
#include <QCommandLineParser>

#include <QtCore/qfile.h>
#include <QtCore/qdir.h>

#include "genshaders.h"

#include "parser.h"

static QByteArray keyFile() { return QByteArrayLiteral("genshaders.keys"); }

static void generateShaders(QVector<QByteArray> &shaderFiles,
                            QSet<QByteArray> &keyFiles,
                            const QVector<QStringRef> &filePaths,
                            const QString &workingDir,
                            bool multilight,
                            bool verboseOutput)
{
    MaterialParser::SceneData sceneData;
    MaterialParser::parseQmlFiles(filePaths, sceneData, verboseOutput);

    GenShaders genShaders(workingDir);
    genShaders.process(sceneData, shaderFiles, keyFiles, multilight);
}

static int writeKeysFile(const QString &keyFile, const QSet<QByteArray> &keyFiles, const QDir &outDir)
{
    const QString outFilename = outDir.canonicalPath() + QDir::separator() + keyFile;
    QFile outFile(outFilename);
    if (!outFile.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        qWarning() << "Unable to create output file " << outFilename;
        return -1;
    }
    QXmlStreamWriter writer(&outFile);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("keys");
    for (const auto &f : keyFiles)
        writer.writeTextElement("key", f);
    writer.writeEndElement();
    writer.writeEndDocument();
    outFile.close();

    return 0;
}

static int writeResourceFile(const QString &resourceFile,
                             const QString &keyFile,
                             const QSet<QByteArray> &keyFiles,
                             const QVector<QByteArray> &shaderFiles,
                             const QDir &outDir)
{
    const QString outFilename = outDir.canonicalPath() + QDir::separator() + resourceFile;
    QFile outFile(outFilename);
    if (!outFile.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        qWarning() << "Unable to create output file " << outFilename;
        return -1;
    }

    QXmlStreamWriter writer(&outFile);
    writer.setAutoFormatting(true);
    writer.writeStartElement("RCC");
        writer.writeStartElement("qresource");
        writer.writeAttribute("prefix", "/");
        for (const auto &f : shaderFiles)
            writer.writeTextElement("file", f);
        for (const auto &f : keyFiles)
            writer.writeTextElement("file", f);
        writer.writeTextElement("file", keyFile);
        writer.writeEndElement();
    writer.writeEndElement();
    outFile.close();

    return 0;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCommandLineParser cmdLineparser;
    // File options
    QCommandLineOption fileInputOption({QChar(u'f'), QLatin1String("files")},
                                       QLatin1String("QML file(s) to be parsed. Multiple files needs to be inside quotation marks."),
                                       QLatin1String("file"));
    cmdLineparser.addOption(fileInputOption);

    // Debug options
    QCommandLineOption verboseOutputOption({QChar(u'v'), QLatin1String("verbose")}, QLatin1String("Turn on verbose output."));
    cmdLineparser.addOption(verboseOutputOption);

    // Generator options
    QCommandLineOption generateShadersOption({QChar(u'g'), QLatin1String("generate")}, QLatin1String("Generate shaders."));
    cmdLineparser.addOption(generateShadersOption);

    QCommandLineOption outputDirOption({QChar(u'o'), QLatin1String("output-dir")}, QLatin1String("Output directory for generated files."), QLatin1String("file"));
    cmdLineparser.addOption(outputDirOption);

    QCommandLineOption resourceFileOption({QChar(u'r'), QLatin1String("resource-file")}, QLatin1String("Name of generated resource file."), QLatin1String("file"));
    cmdLineparser.addOption(resourceFileOption);

    QCommandLineOption workingDirOption({QChar(u'w'), QLatin1String("working-dir")}, QLatin1String("Working directory."), QLatin1String("dir"));
    cmdLineparser.addOption(workingDirOption);

    cmdLineparser.process(a);

    const bool isFilePathSet = cmdLineparser.isSet(fileInputOption);
    if (!isFilePathSet) {
        qWarning("No input file(s) provided!");
        a.exit(-1);
        return -1;
    }

    QDir outDir;
    if (cmdLineparser.isSet(outputDirOption)) {
        const QString &val = cmdLineparser.value(outputDirOption);
        QDir od(val);
        if (!od.exists()) {
            if (od.mkpath(val))
                outDir = od;
        }
    } else {
        outDir.setPath(QDir::currentPath());
    }

    QString resourceFile = cmdLineparser.value(resourceFileOption);
    if (resourceFile.isEmpty())
        resourceFile = QStringLiteral("genshaders.qrc");

    // Best effort, this should be improved
    QString workingDirValue = cmdLineparser.value(workingDirOption);
    QDir workingDir(workingDirValue);
    if (!workingDir.exists(workingDirValue)) {
        qWarning("No working directory provided!");
        a.exit(-1);
        return -1;
    }


    const QString fileInputValue = cmdLineparser.value(fileInputOption);
    const bool verboseOutput = cmdLineparser.isSet(verboseOutputOption);
    const bool genShaders = cmdLineparser.isSet(generateShadersOption);
    const bool multilight = false;

    const auto filePaths = fileInputValue.splitRef(QChar(u';'));
    int ret = 0;
    QVector<QByteArray> shaderFiles;
    QSet<QByteArray> keyFiles;

    if (fileInputValue.size()) {
        if (genShaders) {
            generateShaders(shaderFiles, keyFiles, filePaths, workingDir.canonicalPath(), multilight, verboseOutput);
        } else {
            MaterialParser::SceneData sceneData;
            ret = MaterialParser::parseQmlFiles(filePaths, sceneData, verboseOutput);
            (void)sceneData;
        }
    }

    writeKeysFile(keyFile(), keyFiles, outDir);

    writeResourceFile(resourceFile, keyFile(), keyFiles, shaderFiles, outDir);

    a.exit(ret);
    return ret;
}
