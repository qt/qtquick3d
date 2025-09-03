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

QString LightmapViewerHelpers::lightmapTagToString(QSSGLightmapIODataTag tag)
{
    switch (tag) {
    case QSSGLightmapIODataTag::Unset:
        return QStringLiteral("Unset");
        break;
    case QSSGLightmapIODataTag::Mask:
        return QStringLiteral("Mask");
        break;
    case QSSGLightmapIODataTag::Texture_Final:
        return QStringLiteral("Texture_Final");
        break;
    case QSSGLightmapIODataTag::Texture_Direct:
        return QStringLiteral("Texture_Direct");
        break;
    case QSSGLightmapIODataTag::Texture_Indirect:
        return QStringLiteral("Texture_Indirect");
        break;
    case QSSGLightmapIODataTag::Metadata:
        return QStringLiteral("Metadata");
        break;
    case QSSGLightmapIODataTag::Mesh:
        return QStringLiteral("Mesh");
        break;
    default:
        break;
    }
    return QStringLiteral("Invalid");
}

QSSGLightmapIODataTag LightmapViewerHelpers::stringToLightmapTag(const QString &tag)
{
    if (tag == QStringLiteral("Unset"))
        return QSSGLightmapIODataTag::Unset;
    if (tag == QStringLiteral("Mask"))
        return QSSGLightmapIODataTag::Mask;
    if (tag == QStringLiteral("Texture_Final"))
        return QSSGLightmapIODataTag::Texture_Final;
    if (tag == QStringLiteral("Texture_Direct"))
        return QSSGLightmapIODataTag::Texture_Direct;
    if (tag == QStringLiteral("Texture_Indirect"))
        return QSSGLightmapIODataTag::Texture_Indirect;
    if (tag == QStringLiteral("Metadata"))
        return QSSGLightmapIODataTag::Metadata;
    if (tag == QStringLiteral("Mesh"))
        return QSSGLightmapIODataTag::Mesh;
    if (tag == QStringLiteral("Unset"))
        return QSSGLightmapIODataTag::Unset;

    qWarning() << "Could not match tag for: " << tag;
    return QSSGLightmapIODataTag::Unset;
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

    if (extract) {
        QDir dir;
        for (const char *path : { "meshes", "images", "images/masks", "images/direct", "images/indirect", "images/final" }) {
            if (!dir.mkpath(path)) {
                qInfo() << "Failed to create folders";
                return false;
            }
        }
    }

    int numImagesSaved = 0;
    int numMeshesSaved = 0;

    QList<std::pair<QString, QSSGLightmapIODataTag>> keys = loader->getKeys();

    if (print)
        qInfo() << "-- Keys --";

    QVector<QString> meshKeys;

    for (const auto &[key, tag] : std::as_const(keys)) {
        QString tagString = LightmapViewerHelpers::lightmapTagToString(tag);

        if (print)
            qInfo() << key << ":" << tagString;

        if (tag == QSSGLightmapIODataTag::Mesh)
            meshKeys.push_back(key);
    }

    if (print)
        qInfo() << "-- Values --";

    // Extract meshes
    if (extract) {
        for (const QString &key : meshKeys) {
            const QByteArray meshData = loader->readData(key, QSSGLightmapIODataTag::Mesh);
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

    for (const auto &[key, tag] : std::as_const(keys)) {
        if (tag != QSSGLightmapIODataTag::Metadata)
            continue;

        int width = 0;
        int height = 0;

        if (tag == QSSGLightmapIODataTag::Metadata) {
            QVariantMap map = loader->readMetadata(key);
            if (print) {
                qInfo() << key << ":";
                qInfo().noquote() << QJsonDocument(QJsonObject::fromVariantMap(map)).toJson(QJsonDocument::Indented).trimmed();
            }
            width = map[QStringLiteral("width")].toInt();
            height = map[QStringLiteral("height")].toInt();
        }

        if (extract) {
            if (keys.contains(std::make_pair(key, QSSGLightmapIODataTag::Mask))) {
                QByteArray data = loader->readU32Image(key, QSSGLightmapIODataTag::Mask);
                maskToBBGRColor(data);
                QImage img = QImage(reinterpret_cast<uchar *>(data.data()), width, height, QImage::Format_RGBA8888);
                img.save(QString("images/masks/" + key + ".png"));
                ++numImagesSaved;
            }
            for (const auto &[texTag, dir] : std::array {
                         std::pair { QSSGLightmapIODataTag::Texture_Direct, QStringLiteral("direct") },
                         std::pair { QSSGLightmapIODataTag::Texture_Indirect, QStringLiteral("indirect") },
                         std::pair { QSSGLightmapIODataTag::Texture_Final, QStringLiteral("final") },
                 }) {
                if (!keys.contains(std::make_pair(key, texTag)))
                    continue;
                QByteArray data = loader->readF32Image(key, texTag);
                QImage img = QImage(reinterpret_cast<uchar *>(data.data()), width, height, QImage::Format_RGBA32FPx4);
                img.save(QString("images/" + dir + "/" + key + ".png"));
                ++numImagesSaved;
            }
        }
    }

    if (extract) {
        qInfo() << "Saved" << numImagesSaved << "images to 'images' and " << numMeshesSaved << "meshes to 'meshes'";
    }

    return success;
}
