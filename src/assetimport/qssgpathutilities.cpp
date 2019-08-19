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

#include "qssgpathutilities_p.h"

#include <QtCore/QVector>
#include <QtCore/QDataStream>

QT_BEGIN_NAMESPACE

using namespace QSSGPathUtilities;

void QSSGPathBuffer::save(QIODevice &outStream) const
{
    QDataStream out(&outStream);

    out << getFileTag();
    out << getFileVersion();
    out << quint32(commands.size());
    out << quint32(data.size());
    int commandSize = commands.size()*sizeof(PathCommand);
    out.writeRawData(reinterpret_cast<const char *>(commands.begin()), commandSize);

    // ensure the floats following the commands are aligned to a 4 byte boundary
    while (commandSize & (sizeof(float) - 1)) {
        out << quint8(0);
        ++commandSize;
    }
    out.writeRawData(reinterpret_cast<const char *>(data.begin()), data.size()*sizeof(float));
}

static quint32 align4(quint32 i)
{
    return (i + 3) & ~quint32(3);
}

QSSGPathBuffer *QSSGPathBuffer::load(QIODevice &inStream)
{
    QDataStream in(&inStream);

    quint64 fileTag;
    quint32 version;
    quint32 numCommands;
    quint32 numData;
    in >> fileTag;
    in >> version;
    in >> numCommands;
    in >> numData;
    if (fileTag != getFileTag()) {
        qCritical("Invalid file, not a path file");
        return nullptr;
    }
    if (version > getFileVersion()) {
        qCritical("Version number out of range.");
        return nullptr;
    }
    quint32 commandSize = numCommands * sizeof(PathCommand);
    quint32 dataSize = numData * sizeof(float);
    quint32 objectSize = sizeof(QSSGPathBuffer);
    quint32 allocSize = objectSize + commandSize + dataSize;
    char *rawData = reinterpret_cast<char *>(::malloc(allocSize));
    QSSGPathBuffer *retval = new (rawData) QSSGPathBuffer();
    char *commandBuffer = rawData + sizeof(QSSGPathBuffer);
    char *dataBuffer = commandBuffer + align4(commandSize);
    in.readRawData(commandBuffer, commandSize);
    in.readRawData(dataBuffer, dataSize);
    retval->commands = toDataView((PathCommand *)commandBuffer, numCommands);
    retval->data = toDataView((float *)dataBuffer, numData);
    return retval;
}

QT_END_NAMESPACE
