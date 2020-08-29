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

#include <QtQuick3DUtils/private/qqsbcollection_p.h>

#include "genshaders.h"

#include "parser.h"

static int generateShaders(QVector<QString> &qsbcFiles,
                           const QVector<QStringView> &filePaths,
                           const QDir &sourceDir,
                           const QDir &outDir,
                           bool multilight,
                           bool verboseOutput)
{
    MaterialParser::SceneData sceneData;
    if (MaterialParser::parseQmlFiles(filePaths, sourceDir, sceneData, verboseOutput) == 0) {
        if (sceneData.hasData()) {
            GenShaders genShaders(sourceDir.canonicalPath());
            if (!genShaders.process(sceneData, qsbcFiles, outDir, multilight))
                return -1;
        }
    }
    return 0;
}

static int writeResourceFile(const QString &resourceFile,
                             const QVector<QString> &qsbcFiles,
                             const QDir &outDir)
{
    if (qsbcFiles.isEmpty())
        return -1;

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
        for (const auto &f : qsbcFiles)
            writer.writeTextElement("file", f);
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

    QCommandLineOption sourceDirOption({QChar(u's'), QLatin1String("source-dir")}, QLatin1String("Source directory."), QLatin1String("dir"));
    cmdLineparser.addOption(sourceDirOption);

    QCommandLineOption dumpQsbcFileOption({QChar(u'd'), QLatin1String("dump-qsbc")}, QLatin1String("Dump qsbc file content."));
    cmdLineparser.addOption(dumpQsbcFileOption);

    QCommandLineOption extractQsbFileOption({QChar(u'e'), QLatin1String("extract-qsb")}, QLatin1String("Extract qsb from collection."), QLatin1String("key:[desc|vert|frag]"));
    cmdLineparser.addOption(extractQsbFileOption);

    cmdLineparser.process(a);

    const bool isFilePathSet = cmdLineparser.isSet(fileInputOption);
    if (!isFilePathSet) {
        qWarning("No input file(s) provided!");
        a.exit(-1);
        return -1;
    }

    const QString &outputPath = cmdLineparser.value(outputDirOption);

    if (cmdLineparser.isSet(dumpQsbcFileOption)) {
        const auto f = cmdLineparser.value(fileInputOption);
        if (!f.isEmpty()) {
            QQsbCollection::dumpQsbcInfo(f);
            a.exit(0);
            return 0;
        }
    }

    static const auto printBytes = [](const QByteArray &ba) {
        for (const auto &b : ba)
            printf("%c", b);
    };

    if (cmdLineparser.isSet(extractQsbFileOption)) {
        const auto f = cmdLineparser.value(fileInputOption);
        const auto k = cmdLineparser.value(extractQsbFileOption);
        const auto kl = QStringView(k).split(u':');

        bool ok = false;
        const auto &keyView = kl.at(0);
        const size_t key = keyView.toULong(&ok);
        if (ok) {
            enum ExtractWhat : quint8 { Desc = 0x1, Vert = 0x2, Frag = 0x4 };
            quint8 what = 0;
            if (kl.size() > 1) {
                const auto &rest = kl.at(1);
                const auto &options = rest.split(u'|');
                for (const auto &o : options) {
                    if (o == QLatin1String("desc"))
                        what |= ExtractWhat::Desc;
                    if (o == QLatin1String("vert"))
                        what |= ExtractWhat::Vert;
                    if (o == QLatin1String("frag"))
                        what |= ExtractWhat::Frag;
                }
            }
            QQsbCollection qsbc(f);
            if (qsbc.map(QQsbCollection::Read)) {
                const auto entries = qsbc.getEntries();
                const auto foundIt = entries.constFind(QQsbCollection::Entry{key});
                if (foundIt != entries.cend()) {
                    QByteArray desc;
                    QShader vertShader;
                    QShader fragShader;
                    qsbc.extractQsbEntry(*foundIt, &desc, &vertShader, &fragShader);
                    if (what == 0)
                        qDebug("Entry with key %zu found.", key);
                    if (what & ExtractWhat::Desc)
                        printBytes(desc);
                    if (what & ExtractWhat::Vert)
                        printBytes(qUncompress(vertShader.serialized()));
                    if (what & ExtractWhat::Frag)
                        printBytes(qUncompress(fragShader.serialized()));
                } else {
                    qWarning("Entry with key %zu could not be found.", key);
                }
                qsbc.unmap();
            }
            a.exit(0);
            return 0;
        }

        qWarning("Command %s failed with input: %s and %s.", qPrintable(extractQsbFileOption.valueName()), qPrintable(f), qPrintable(k));
        a.exit(-1);
        return -1;
    }

    QString resourceFile = cmdLineparser.value(resourceFileOption);
    if (resourceFile.isEmpty())
        resourceFile = QStringLiteral("genshaders.qrc");

    // Best effort, this should be improved
    QString sourceDirValue = cmdLineparser.value(sourceDirOption);
    QDir sourceDir(sourceDirValue);
    if (!sourceDir.exists(sourceDirValue)) {
        qWarning("No source directory provided!");
        a.exit(-1);
        return -1;
    }

    QDir outDir;
    if (!outputPath.isEmpty()) {
        if (outDir.exists(outputPath) || (!outDir.exists(outputPath) && outDir.mkpath(outputPath))) {
            outDir.setPath(outputPath);
            qDebug("Writing files to %s", qPrintable(outDir.canonicalPath()));
        } else {
            qDebug("Unable to change or create output folder %s", qPrintable(outputPath));
            return -1;
        }
    }

    const QString fileInputValue = cmdLineparser.value(fileInputOption);
    const bool verboseOutput = cmdLineparser.isSet(verboseOutputOption);
    const bool genShaders = cmdLineparser.isSet(generateShadersOption);
    const bool multilight = false;

    QVector<QString> qsbcFiles;

    const auto &filePaths = QStringView(fileInputValue).split(u' ');
    int ret = 0;
    if (fileInputValue.size()) {
        if (genShaders) {
            ret = generateShaders(qsbcFiles, filePaths, sourceDir, outDir, multilight, verboseOutput);
        } else {
            MaterialParser::SceneData sceneData;
            ret = MaterialParser::parseQmlFiles(filePaths, sourceDir, sceneData, verboseOutput);
            (void)sceneData;
        }
    }

    if (ret == 0)
        writeResourceFile(resourceFile, qsbcFiles, outDir);

    a.exit(ret);
    return ret;
}
