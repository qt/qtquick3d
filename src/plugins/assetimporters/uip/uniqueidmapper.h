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

#ifndef UNIQUEIDMAPPER_H
#define UNIQUEIDMAPPER_H

#include <QHash>
#include <QSet>

QT_BEGIN_NAMESPACE

class UniqueIdMapper
{
public:
    static UniqueIdMapper *instance();
    void reset();

    QByteArray queryId(const QByteArray &id);
    QByteArray queryId(const QString &id);
    QByteArray generateUniqueId(const QByteArray &id);

private:
    UniqueIdMapper();
    ~UniqueIdMapper();

    QSet<QString> m_uniqueIds;
    QHash<QByteArray, QByteArray> m_uniqueIdMap;
};

QT_END_NAMESPACE

#endif // UNIQUEIDMAPPER_H
