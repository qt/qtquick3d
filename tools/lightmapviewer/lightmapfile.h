// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef LIGHTMAPFILE_H
#define LIGHTMAPFILE_H

#include <QObject>
#include <QList>
#include <QUrl>

class LightmapFile : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList dataList READ dataList NOTIFY dataListChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged FINAL)

public:
    explicit LightmapFile(QObject *parent = nullptr);

    QStringList dataList() const;

    Q_INVOKABLE void loadData();

    QUrl source() const;
    void setSource(const QUrl &newSource);

signals:
    void dataListChanged();

    void sourceChanged();

private:
    QStringList m_dataList;
    QUrl m_source;
};

#endif // LIGHTMAPFILE_H
