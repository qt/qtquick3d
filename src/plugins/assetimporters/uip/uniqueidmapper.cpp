/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#include "uniqueidmapper.h"

#include <QtQuick3DAssetImport/private/qssgqmlutilities_p.h>

QT_BEGIN_NAMESPACE

UniqueIdMapper *UniqueIdMapper::instance()
{
    static UniqueIdMapper mapper;
    return &mapper;
}

void UniqueIdMapper::reset()
{
    m_uniqueIdMap.clear();
    m_uniqueIds.clear();
}

QByteArray UniqueIdMapper::queryId(const QByteArray &id)
{
    QByteArray idCopy = id;
    if (id.startsWith('#'))
        idCopy = idCopy.mid(1);

    QByteArray value = m_uniqueIdMap[idCopy];
    if (value.isEmpty()) {
        // try again after sanitizing
        value = m_uniqueIdMap[QSSGQmlUtilities::sanitizeQmlId(idCopy).toUtf8()];
        if (value.isEmpty())
            value = QSSGQmlUtilities::sanitizeQmlId(idCopy).toUtf8();
    }
    return value;
}

QByteArray UniqueIdMapper::queryId(const QString &id)
{
    return queryId(id.toUtf8());
}

QByteArray UniqueIdMapper::generateUniqueId(const QByteArray &id)
{
    int index = 0;
    QByteArray uniqueID = QSSGQmlUtilities::sanitizeQmlId(id).toLocal8Bit();
    while (m_uniqueIds.contains(uniqueID))
        uniqueID = uniqueID + QByteArrayLiteral("_") + QString::number(++index).toLocal8Bit();
    m_uniqueIds.insert(uniqueID);
    m_uniqueIdMap.insert(id, uniqueID);
    return uniqueID;
}

UniqueIdMapper::UniqueIdMapper()
{

}

UniqueIdMapper::~UniqueIdMapper()
{

}

QT_END_NAMESPACE
