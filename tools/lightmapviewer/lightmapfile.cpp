// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lightmapfile.h"

#include <QFile>
#include <QTextStream>
#include <QtQuick3DRuntimeRender/private/qssglightmapio_p.h>

LightmapFile::LightmapFile(QObject *parent) : QObject { parent } { }

QStringList LightmapFile::dataList() const
{
    return m_dataList;
}

void LightmapFile::loadData()
{
    QSharedPointer<QSSGLightmapLoader> loader = QSSGLightmapLoader::open(m_source.toLocalFile());
    m_dataList = loader ? loader->getKeys() : QStringList();
    emit dataListChanged();
}

QUrl LightmapFile::source() const
{
    return m_source;
}

void LightmapFile::setSource(const QUrl &newSource)
{
    if (m_source == newSource)
        return;
    m_source = newSource;
    emit sourceChanged();
}
