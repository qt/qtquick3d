// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lightmapfile.h"

#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include <QBuffer>
#include <QQmlEngine>

#include <QtQuick3DRuntimeRender/private/qssglightmapio_p.h>

#include "lightmapviewerhelpers.h"

LightmapFile::LightmapFile(QObject *parent) : QObject { parent } { }

QStringList LightmapFile::keys() const
{
    return m_keys;
}

void LightmapFile::loadData()
{
    QSharedPointer<QSSGLightmapLoader> loader = QSSGLightmapLoader::open(m_source.toLocalFile());
    auto keys = loader ? loader->getKeys() : QList<std::pair<QString, QSSGLightmapIODataTag>>();
    m_keys.clear();

    QVariantMap sceneMetadata;
    for (const auto &[key, tag] : std::as_const(keys)) {
        if (tag == QSSGLightmapIODataTag::SceneMetadata) {
            sceneMetadata = loader->readMap(key, tag);
            continue;
        } else if (tag == QSSGLightmapIODataTag::Metadata) {
            m_keys.append(key);
        }
    }

    emit keysChanged();

    const auto processed = LightmapViewerHelpers::processSceneMetadata(sceneMetadata);
    m_qtVersion = processed.qtVersion;
    m_bakeStart = processed.bakeStartTime;
    m_bakeDuration = processed.bakeDuration;
    m_options = processed.options;

    emit metadataChanged();
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
    loadData();
    emit sourceChanged();
}

QVariantList LightmapFile::metadataFor(const QString &key)
{
    if (key.isEmpty())
        return {};

    auto loader = QSSGLightmapLoader::open(m_source.toLocalFile());
    if (!loader)
        return {};

    const auto metadata = loader->readMap(key, QSSGLightmapIODataTag::Metadata);
    if (metadata.isEmpty())
        return {};

    return LightmapViewerHelpers::processMetadata(key, metadata);
}

QQuick3DTextureData *LightmapFile::textureDataFor(const QString &key, quint32 tag)
{
    if (key.isEmpty())
        return nullptr;


    auto loader = QSSGLightmapLoader::open(m_source.toLocalFile());
    if (!loader)
        return nullptr;

    const QVariantMap md = loader->readMap(key, QSSGLightmapIODataTag::Metadata);
    const int w = md.value(QStringLiteral("width")).toInt();
    const int h = md.value(QStringLiteral("height")).toInt();
    if (w <= 0 || h <= 0)
        return nullptr;

    const QSSGLightmapIODataTag actualTag = static_cast<QSSGLightmapIODataTag>(tag);

    QByteArray src;

    if (actualTag == QSSGLightmapIODataTag::Mask) {
        QByteArray src = loader->readU32Image(key, actualTag);
        if (src.size() != w * h * int(sizeof(quint32)))
            return nullptr;

        LightmapViewerHelpers::maskToBBGRColor(src, false);
        QByteArray dst;
        dst.resize(src.size());
        const int stride = w * 4;
        const uchar* s = reinterpret_cast<const uchar*>(src.constData());
        uchar* d = reinterpret_cast<uchar*>(dst.data());
        for (int y = 0; y < h; ++y) {
            const int srcRow = y;
            const int dstRow = (h - 1) - y;
            memcpy(d + dstRow * stride, s + srcRow * stride, size_t(stride));
        }

        auto *tex = new QQuick3DTextureData;
        QQmlEngine::setObjectOwnership(tex, QQmlEngine::CppOwnership);
        tex->setSize(QSize(w, h));
        tex->setFormat(QQuick3DTextureData::RGBA8);
        tex->setHasTransparency(false);
        tex->setTextureData(dst);
        return tex;
    }

    if (actualTag == QSSGLightmapIODataTag::Texture_Final
        || actualTag == QSSGLightmapIODataTag::Texture_Direct
        || actualTag == QSSGLightmapIODataTag::Texture_Indirect) {

        src = loader->readF32Image(key, actualTag);

        if (src.size() != w * h * int(4 * sizeof(float)))
            return nullptr;

        QByteArray dst;
        dst.resize(src.size());
        const float *srcF = reinterpret_cast<const float *>(src.constData());
        float *dstF = reinterpret_cast<float *>(dst.data());
        const int strideFloats = w * 4;
        for (int y = 0; y < h; ++y) {
            const int srcRow = y;
            const int dstRow = (h - 1) - y;
            memcpy(dstF + dstRow * strideFloats,
                   srcF + srcRow * strideFloats,
                   size_t(strideFloats) * sizeof(float));
        }

        auto *tex = new QQuick3DTextureData;
        QQmlEngine::setObjectOwnership(tex, QQmlEngine::CppOwnership);
        tex->setSize(QSize(w, h));
        tex->setFormat(QQuick3DTextureData::RGBA32F);
        tex->setHasTransparency(false);
        tex->setTextureData(dst);
        return tex;
    }

    return nullptr;
}

QVariantList LightmapFile::texturesAvailableFor(const QString &key) const
{
    if (key.isEmpty())
        return {};

    auto loader = QSSGLightmapLoader::open(m_source.toLocalFile());
    if (!loader)
        return {};

    QVariantList out;

    const auto keys = loader->getKeys();
    for (const auto &kp : keys) {
        if (kp.first != key) continue;

        const QSSGLightmapIODataTag tag = kp.second;
        if (tag== QSSGLightmapIODataTag::Mask || tag== QSSGLightmapIODataTag::Texture_Direct
            || tag == QSSGLightmapIODataTag::Texture_Indirect || tag == QSSGLightmapIODataTag::Texture_Final) {
            out << QVariantMap { {QStringLiteral("name"), LightmapViewerHelpers::lightmapTagToString(tag).replace("Texture_", "")},
                                 {QStringLiteral("value"), static_cast<quint32>(tag)}
            };
        }
    }
    return out;
}

QString LightmapFile::meshKeyFor(const QString &key) const
{
    auto loader = QSSGLightmapLoader::open(m_source.toLocalFile());
    if (!loader)
        return {};

    const auto metadata = loader->readMap(key, QSSGLightmapIODataTag::Metadata);
    if (metadata.isEmpty())
        return {};

    return metadata[QStringLiteral("mesh_key")].toString();
}

QVector3D LightmapFile::getAppliedScaleFor(const QString &key) const
{
    if (key.isEmpty())
        return QVector3D(1, 1, 1);

    if (key.isEmpty())
        return QVector3D(1, 1, 1);

    auto loader = QSSGLightmapLoader::open(m_source.toLocalFile());
    if (!loader)
        return QVector3D(1, 1, 1);

    auto scaleBuffer = loader->readData(key, QSSGLightmapIODataTag::OriginalScale);
    if (scaleBuffer.isEmpty()) {
        qDebug() << "scale empty";
        return QVector3D(1, 1, 1);
    }

    QDataStream stream(&scaleBuffer, QIODevice::ReadOnly);
    QVector3D scale;
    stream >> scale;
    return scale;
}

QUrl LightmapFile::imageUrlFor(const QString &key, quint32 tag, bool alpha) const
{
    return QStringLiteral("image://lightmaps/key=%1&tag=%2&file=%3&alpha=%4")
                                  .arg(key)
                                  .arg(LightmapViewerHelpers::lightmapTagToString(static_cast<QSSGLightmapIODataTag>(tag)))
                                  .arg(m_source.toString())
                                  .arg(alpha);
}
