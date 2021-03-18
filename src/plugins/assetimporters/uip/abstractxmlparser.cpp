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

#include "abstractxmlparser.h"

#include <QtCore/QDebug>

QString AbstractXmlParser::readerErrorString() const
{
    return QObject::tr("Failed to parse %1: line %2: column %3: offset %4: %5")
            .arg(m_sourceInfo.fileName())
            .arg(m_reader.lineNumber())
            .arg(m_reader.columnNumber())
            .arg(m_reader.characterOffset())
            .arg(m_reader.errorString());
}

bool AbstractXmlParser::setSource(const QString &filename)
{
    if (m_sourceFile.isOpen())
        m_sourceFile.close();

    m_sourceFile.setFileName(filename);
    if (!m_sourceFile.exists()) {
        qWarning() << QObject::tr("Source file %1 does not exist").arg(filename);
        return false;
    }
    if (!m_sourceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << QObject::tr("Failed to open %1").arg(filename);
        return false;
    }

    m_parseTimer.start();

    m_sourceInfo = QFileInfo(filename);

    m_reader.setDevice(&m_sourceFile);

    return true;
}

bool AbstractXmlParser::setSourceData(const QByteArray &data)
{
    m_parseTimer.start();

    m_reader.clear();
    m_reader.addData(data);

    return true;
}

QXmlStreamReader *AbstractXmlParser::reader()
{
    return &m_reader;
}
