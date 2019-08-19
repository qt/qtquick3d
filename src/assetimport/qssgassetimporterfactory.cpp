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

#include "qssgassetimporterfactory_p.h"
#include "qssgassetimporterplugin_p.h"
#include "qssgassetimporter_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/private/qfactoryloader_p.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader, (QSSGAssetImporterFactoryInterface_iid, QLatin1String("/assetimporters"), Qt::CaseInsensitive))
#if QT_CONFIG(library)
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, directLoader, (QSSGAssetImporterFactoryInterface_iid, QLatin1String(""), Qt::CaseInsensitive))
#endif

QStringList QSSGAssetImporterFactory::keys(const QString &pluginPath)
{
    QStringList list;
    if (!pluginPath.isEmpty()) {
#if QT_CONFIG(library)
        QCoreApplication::addLibraryPath(pluginPath);
        list = directLoader()->keyMap().values();
        if (!list.isEmpty()) {
            const QString postFix = QStringLiteral(" (from ") + QDir::toNativeSeparators(pluginPath) + QLatin1Char(')');
            const QStringList::iterator end = list.end();
            for (QStringList::iterator it = list.begin(); it != end; ++it)
                (*it).append(postFix);
        }
#else
        qWarning("Cannot query QSSGAssetImporter plugins at %s: Library loading is disabled.",
                 pluginPath.toLocal8Bit().constData());
#endif
    }
    list.append(loader()->keyMap().values());
    return list;
}

QSSGAssetImporter *QSSGAssetImporterFactory::create(const QString &name, const QStringList &args, const QString &pluginPath)
{
    if (!pluginPath.isEmpty()) {
#if QT_CONFIG(library)
        QCoreApplication::addLibraryPath(pluginPath);
        if (QSSGAssetImporter *ret = qLoadPlugin<QSSGAssetImporter, QSSGAssetImporterPlugin>(directLoader(), name, args))
            return ret;
#else
        qWarning("Cannot load QSSGAssetImporter plugin from %s. Library loading is disabled.",
                 pluginPath.toLocal8Bit().constData());
#endif
    }
    if (QSSGAssetImporter *ret = qLoadPlugin<QSSGAssetImporter, QSSGAssetImporterPlugin>(loader(), name, args))
        return ret;
    return nullptr;
}

QT_END_NAMESPACE
