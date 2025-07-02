// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lightmapviewerhelpers.h"

#include <QtQuick3DRuntimeRender/private/qssglightmapio_p.h>

#include <QRgb>
#include <QVector>
#include <QImage>
#include <QFile>
#include <QJsonObject>
#include <QDir>

static QRgb numberToBBGRColor(quint32 i, quint32 N, bool useAlpha)
{
    if (i < 1 || i > N) {
        return qRgba(0, 0, 0, useAlpha ? 0 : 0xff);
    }

    int range = N - 1; // exclude 0
    double t = static_cast<double>(i - 1) / range;

    quint8 r = 0, g = 0, b = 0;

    if (t < 0.5) {
        // Blue -> Green
        double t2 = t / 0.5; // normalize 0..1
        g = 255 * t2;
        b = 255 * (1.0 - t2);
    } else {
        // Green -> Red
        double t2 = (t - 0.5) / 0.5;
        r = 255 * t2;
        g = 255 * (1.0 - t2);
    }

    return qRgba(r, g, b, 0xff);
}

void LightmapViewerHelpers::maskToBBGRColor(QByteArray &array, bool useAlpha)
{
    QVector<quint32> uints;
    uints.resize(array.size() / sizeof(quint32));
    memcpy(uints.data(), array.data(), array.size());

    quint32 maxN = 0;
    for (quint32 v : uints) {
        maxN = qMax(maxN, v);
    }
    for (quint32 &vRef : uints) {
        vRef = numberToBBGRColor(vRef, maxN, useAlpha);
    }
    memcpy(array.data(), uints.data(), array.size());
}

bool LightmapViewerHelpers::processLightmap(const QString &filename, bool print, bool extract)
{
    bool success = true;
    QSharedPointer<QSSGLightmapLoader> loader = QSSGLightmapLoader::open(filename);

    if (!loader) {
        return false;
    }

    if (QDir dir; !dir.exists("meshes") && (!dir.mkpath("meshes") || !dir.mkpath("images"))) {
        qInfo() << "Failed to create folders";
        return false;
    }

    int numImagesSaved = 0;
    int numMeshesSaved = 0;

    QList<QString> keys = loader->getKeys();

    if (print)
        qInfo() << "-- Keys --";

    QVector<QString> baseKeys;
    QVector<QString> meshKeys;

    for (const QString &key : std::as_const(keys)) {
        if (print)
            qInfo() << key;

        if (key.endsWith(QByteArrayLiteral("_metadata"))) {
            baseKeys.push_back(key.chopped(9));
        } else if (key.startsWith(QByteArrayLiteral("_mesh"))) {
            meshKeys.push_back(key);
        }
    }

    if (print)
        qInfo() << "-- Values --";

    // Extract meshes
    if (extract) {
        for (const QString &key : meshKeys) {
            const QByteArray meshData = loader->readData(key);
            QFile meshFile(QString("meshes/" + key + ".mesh"));
            if (meshFile.open(QFile::WriteOnly)) {
                meshFile.write(meshData);
                meshFile.close();
                ++numMeshesSaved;
            } else {
                success = false;
                qInfo() << key << "->" << "FAILED TO WRITE";
            }
        }
    }

    for (const QString &key : baseKeys) {
        const QString key_metadata = key + QStringLiteral("_metadata");
        const QString key_mask = key + QStringLiteral("_mask");
        const QString key_direct = key + QStringLiteral("_direct");
        const QString key_indirect = key + QStringLiteral("_indirect");
        const QString key_final = key + QStringLiteral("_final");

        int width = 0;
        int height = 0;

        if (keys.contains(key_metadata)) {
            QVariantMap map = loader->readMetadata(key);
            if (print) {
                qInfo() << key_metadata << ":";
                qInfo().noquote() << QJsonDocument(QJsonObject::fromVariantMap(map)).toJson(QJsonDocument::Indented).trimmed();
            }
            width = map[QStringLiteral("width")].toInt();
            height = map[QStringLiteral("height")].toInt();
        } else {
            success = false;
            qInfo() << key << ": expected metadata key, skipping.";
            continue;
        }

        if (extract) {
            if (keys.contains(key_mask)) {
                QByteArray data = loader->readU32Image(key_mask);
                maskToBBGRColor(data);
                QImage img = QImage(reinterpret_cast<uchar *>(data.data()), width, height, QImage::Format_RGBA8888);
                img.save(QString("images/" + key_mask + ".png"));
                ++numImagesSaved;
            }
            for (const QString &imageKey : { key_direct, key_indirect, key_final }) {
                if (keys.contains(imageKey)) {
                    QByteArray data = loader->readF32Image(imageKey);
                    QImage img = QImage(reinterpret_cast<uchar *>(data.data()), width, height, QImage::Format_RGBA32FPx4);
                    img.save(QString("images/" + imageKey + ".png"));
                    ++numImagesSaved;
                }
            }
        }
    }

    if (extract) {
        qInfo() << "Saved" << numImagesSaved << "images to 'images' and " << numMeshesSaved << "meshes to 'meshes'";
    }

    return success;
}
