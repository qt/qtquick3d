/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "resourceserver.h"
#include "resourceclient.h"

#include "materialadapter.h"

#include <QtQuick3D>

#include <QtCore/qcommandlineparser.h>
#include <QtCore/qcommandlineoption.h>

// For component loading case
#include <QtQuick3D/private/qquick3dcustommaterial_p.h>

int main(int argc, char *argv[])
{
    qputenv("QT_QUICK3D_EDITORMODE", "1");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);
    app.setApplicationName("Material Editor");
    app.setOrganizationName("QtProject");
    app.setOrganizationDomain("qt-project.org");
    app.setApplicationVersion(QLatin1String(QT_VERSION_STR));

    QSurfaceFormat::setDefaultFormat(QQuick3D::idealSurfaceFormat(4));

    QCommandLineParser cmdLineparser;
    cmdLineparser.addHelpOption();

    QCommandLineOption modeOption({QChar(u'm'), QLatin1String("mode")}, QLatin1String("sets editor mode"), QLatin1String("c"));
    cmdLineparser.addOption(modeOption);

    QCommandLineOption changeDirOption({QChar(u'C'), QLatin1String("directory")}, QLatin1String("Change the working directory"), QLatin1String("dir"));
    cmdLineparser.addOption(changeDirOption);

    QCommandLineOption verboseOutputOption({QChar(u'v'), QLatin1String("verbose")}, QLatin1String("Turn on verbose output."));
    cmdLineparser.addOption(verboseOutputOption);

    cmdLineparser.process(app);

    bool workingDirSet = false;
    if (cmdLineparser.isSet(changeDirOption)) {
        const auto value = cmdLineparser.value(changeDirOption);
        QFileInfo fi(value);
        if (!fi.isDir()) {
            qWarning("%s : %s - Not a directory", qPrintable(app.applicationName()), qPrintable(value));
            return -1;
        }
        workingDirSet = true;
        QDir::setCurrent(value);
    }

    QString resourceFile;
    auto args = cmdLineparser.positionalArguments();
    for (const auto &arg : qAsConst(args)) {
        if (arg.endsWith(".qml"))
            resourceFile = arg;
    }

    if (!resourceFile.isEmpty() && QFileInfo::exists(resourceFile)) {
        QFileInfo resourceFileInfo(resourceFile);
        if (resourceFileInfo.isFile()) {
            if (!workingDirSet)
                QDir::setCurrent(resourceFileInfo.dir().path());

            resourceFile = resourceFileInfo.canonicalFilePath();
        }
    }

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    engine.load(url);
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
