/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef ABSTRACTXMLPARSER_H
#define ABSTRACTXMLPARSER_H

#include <QXmlStreamReader>
#include <QFileInfo>
#include <QFile>
#include <QElapsedTimer>

QT_BEGIN_NAMESPACE

class AbstractXmlParser
{
public:
    virtual ~AbstractXmlParser() = default;

    QFileInfo *sourceInfo() { return &m_sourceInfo; }

    quint64 elapsedSinceSetSource() const { return m_parseTimer.elapsed(); }
    QString readerErrorString() const;

protected:
    bool setSource(const QString &filename);
    bool setSourceData(const QByteArray &data);
    QXmlStreamReader *reader();

private:
    QXmlStreamReader m_reader;
    QFileInfo m_sourceInfo;
    QFile m_sourceFile;
    QElapsedTimer m_parseTimer;
};

QT_END_NAMESPACE

#endif // ABSTRACTXMLPARSER_H
