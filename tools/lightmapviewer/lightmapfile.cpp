// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lightmapfile.h"

#include <QFile>
#include <QTextStream>
#include <QtQuick3DRuntimeRender/private/qssglightmapio_p.h>
#include "lightmapviewerhelpers.h"

LightmapFile::LightmapFile(QObject *parent) : QObject { parent } { }

QStringList LightmapFile::dataList() const
{
    return m_dataList;
}

void LightmapFile::loadData()
{
    QSharedPointer<QSSGLightmapLoader> loader = QSSGLightmapLoader::open(m_source.toLocalFile());
    auto keys = loader ? loader->getKeys() : QList<std::pair<QString, QSSGLightmapIODataTag>>();
    m_dataList.clear();
    m_dataList.reserve(keys.size());
    for (const auto &[key, tag] : std::as_const(keys)) {
        QString tagString = LightmapViewerHelpers::lightmapTagToString(tag);
        m_dataList.push_back(key + QStringLiteral("$") + tagString);
    }
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
