/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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

#include "qssgrenderinputstreamfactory_p.h"

#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QUrl>
#include <QtCore/QMutexLocker>

#include <limits>

QT_BEGIN_NAMESPACE

namespace {
class QSSGInputStream : public QFile
{
public:
    explicit QSSGInputStream(const QString &inPath) : QFile(inPath), m_path(inPath) {}
    ~QSSGInputStream() override = default;
    QString path() const { return m_path; }

private:
    QString m_path;
};


QString normalizePathForQtUsage(const QString &path)
{
    // path can be a file path or a qrc URL string.

    QString filePath = QDir::cleanPath(path);

    if (filePath.startsWith(QLatin1String("qrc:/")))
        return filePath.mid(3);

    return filePath;
}

const QString Q3DSTUDIO_TAG = QStringLiteral("qt3dstudio");

}

QSSGInputStreamFactory::~QSSGInputStreamFactory() {}

QSSGInputStreamFactory::QSSGInputStreamFactory()
{
    // Add the top-level qrc directory
    if (!QDir::searchPaths(Q3DSTUDIO_TAG).contains(QLatin1String(":/")))
        QDir::addSearchPath(Q3DSTUDIO_TAG, QStringLiteral(":/"));
}

void QSSGInputStreamFactory::addSearchDirectory(const QString &inDirectory)
{
    QMutexLocker factoryLocker(&m_mutex);
    QString localDir = normalizePathForQtUsage(inDirectory);
    QDir directory(localDir);
    if (!directory.exists()) {
        qCritical("Adding search directory: %s", inDirectory.toUtf8().constData());
        return;
    }

    if (!QDir::searchPaths(Q3DSTUDIO_TAG).contains(localDir))
        QDir::addSearchPath(Q3DSTUDIO_TAG, localDir);
}

QSharedPointer<QIODevice> QSSGInputStreamFactory::getStreamForFile(const QString &inFilename, bool inQuiet)
{
    QMutexLocker factoryLocker(&m_mutex);
    QString localFile = normalizePathForQtUsage(inFilename);
    QFileInfo fileInfo = QFileInfo(localFile);
    QIODevice *inputStream = nullptr;
    // Try to match the file with the search paths
    if (!fileInfo.exists())
        fileInfo.setFile(QStringLiteral("qt3dstudio:") + localFile);

    if (fileInfo.exists()) {
        QSSGInputStream *file = new QSSGInputStream(fileInfo.absoluteFilePath());
        if (file->open(QIODevice::ReadOnly))
            inputStream = file;
    }

    if (!inputStream && !inQuiet) {
        // Print extensive debugging information.
        qCritical("Failed to find file: %s", inFilename.toLatin1().data());
        qCritical("Searched path: %s", QDir::searchPaths(Q3DSTUDIO_TAG).join(',').toLatin1().constData());
    }
    return QSharedPointer<QIODevice>(inputStream);
}

bool QSSGInputStreamFactory::getPathForFile(const QString &inFilename, QString &outFile, bool inQuiet)
{
    QSharedPointer<QIODevice> theStream = getStreamForFile(inFilename, inQuiet);
    if (theStream) {
        QSSGInputStream *theRealStream = static_cast<QSSGInputStream *>(theStream.data());
        outFile = theRealStream->path();
        return true;
    }
    return false;
}

QT_END_NAMESPACE
