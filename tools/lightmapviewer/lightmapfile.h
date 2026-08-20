// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QList>
#include <QUrl>
#include <QDateTime>
#include <QSize>

#include <QtQuick3D/private/qquick3dtexturedata_p.h>

class LightmapFile : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList keys READ keys NOTIFY keysChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged FINAL)
    Q_PROPERTY(QString qtVersion READ qtVersion NOTIFY metadataChanged FINAL)
    Q_PROPERTY(QString bakeStart READ bakeStart NOTIFY metadataChanged FINAL)
    Q_PROPERTY(QString bakeDuration READ bakeDuration NOTIFY metadataChanged FINAL)
    Q_PROPERTY(QVariantList options READ options NOTIFY metadataChanged FINAL)

public:
    explicit LightmapFile(QObject *parent = nullptr);

    QStringList keys() const;

    Q_INVOKABLE QVariantList metadataFor(const QString &key);
    Q_INVOKABLE QQuick3DTextureData *textureDataFor(const QString &key, quint32 tag);
    Q_INVOKABLE QVariantList texturesAvailableFor(const QString &key) const;
    Q_INVOKABLE QString meshKeyFor(const QString &key) const;
    Q_INVOKABLE QVector3D getAppliedScaleFor(const QString &key) const;
    Q_INVOKABLE QUrl imageUrlFor(const QString &key, quint32 tag, bool alpha) const;

    QUrl source() const;
    void setSource(const QUrl &newSource);

    QString qtVersion() const { return m_qtVersion; }
    QString bakeStart() const { return m_bakeStart; }
    QString bakeDuration() const { return m_bakeDuration; }
    QVariantList options() const { return m_options; }

signals:
    void keysChanged();
    void sourceChanged();
    void metadataChanged();

private:
    void loadData();

    QStringList m_keys;
    QUrl m_source;
    QString m_qtVersion;
    QString m_bakeStart;
    QString m_bakeDuration;
    QVariantList m_options;
};
