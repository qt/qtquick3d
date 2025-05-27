// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGLIGHTMAPIO_H
#define QSSGLIGHTMAPIO_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qtquick3druntimerenderexports.h"

#include <QByteArray>
#include <QMap>
#include <QSharedPointer>
#include <QIODevice>
#include <QVariantMap>

QT_BEGIN_NAMESPACE

struct QSSGLoadedTexture;
struct QSSGRenderTextureFormat;
struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGLightmapLoader
{
    ~QSSGLightmapLoader();

    static QSharedPointer<QSSGLightmapLoader> open(const QSharedPointer<QIODevice> &stream);
    static QSharedPointer<QSSGLightmapLoader> open(const QString &path);

    QByteArray readF32Image(const QString &key) const;
    QByteArray readU32Image(const QString &key) const;
    QByteArray readData(const QString &key) const;
    QVariantMap readMetadata(const QString &key) const;

    QList<QString> getKeys() const;
    static QSSGLoadedTexture *createTexture(QSharedPointer<QIODevice> stream, const QSSGRenderTextureFormat &format, const QString &key);

private:
    QSSGLightmapLoader();
    Q_DISABLE_COPY(QSSGLightmapLoader)

    struct QSSGLightmapIOPrivate *d = nullptr;
};

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGLightmapWriter
{
    ~QSSGLightmapWriter();

    static QSharedPointer<QSSGLightmapWriter> open(const QSharedPointer<QIODevice> &stream);
    static QSharedPointer<QSSGLightmapWriter> open(const QString &path);

    bool writeF32Image(const QString &key, const QByteArray &imageFP32);
    bool writeU32Image(const QString &key, const QByteArray &imageU32);
    bool writeData(const QString &key, const QByteArray &buffer);
    bool writeMetadata(const QString &key, const QVariantMap &metadata);

    bool close() const;

private:
    Q_DISABLE_COPY(QSSGLightmapWriter)
    QSSGLightmapWriter();

    struct QSSGLightmapIOPrivate *d = nullptr;
};

QT_END_NAMESPACE

#endif // QSSGLIGHTMAPIO_H
