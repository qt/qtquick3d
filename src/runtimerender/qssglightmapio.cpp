// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qssglightmapio_p.h"

#include <private/qssgrenderloadedtexture_p.h>
#include <private/qssgassert_p.h>

#include <QDataStream>
#include <QDebug>
#include <QtEndian>
#include <QFile>

#include <algorithm>
#include <cstring>

QT_BEGIN_NAMESPACE

constexpr int cNameLength = 112;

struct IndexEntry
{
    char name[cNameLength]; // UTF-8 null-padded, max 111 chars
    qint64 offset; // 8 bytes
    qint64 size; // 8 bytes
};

struct QSSGLightmapIOPrivate
{
    QByteArray readKey(const QByteArray &key) const;
    bool writeHeader() const;
    bool writeData(const QString &key, const QByteArray &data);
    bool writeFooter() const;
    bool decodeHeaders();
    QList<QString> getKeys() const;

    QSharedPointer<QIODevice> stream;
    QMap<QByteArray, IndexEntry> entries; // Maps from name -> entry
    qint64 entryCount = -1;
    qint64 indexOffset = -1;
    qint64 fileVersion = -1;
    qint64 fileSize = -1;
};

static_assert(offsetof(IndexEntry, name) == 0, "Unexpected alignment");
static_assert(offsetof(IndexEntry, offset) == cNameLength, "Unexpected alignment");
static_assert(offsetof(IndexEntry, size) == cNameLength + 8, "Unexpected alignment");
static_assert(sizeof(IndexEntry) == cNameLength + 16, "Unexpected size");

constexpr char fileSignature[] = "QTLTMP";

static QByteArray keyToByteArray(const QString &key)
{
    QByteArray keyBuffer = key.toUtf8();
    if (keyBuffer.size() > cNameLength) {
        qWarning() << "QSSGLightmapIO: Key too big, truncating";
        keyBuffer.slice(keyBuffer.size() - cNameLength, cNameLength);
    }
    return keyBuffer;
}

static QByteArray mapToByteArray(const QVariantMap &map)
{
    QByteArray byteArray;
    QDataStream out(&byteArray, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_9);
    out.setByteOrder(QDataStream::LittleEndian);
    out << QVariant(map);
    return byteArray;
}

static void convertEndian(QByteArray &buffer, int sizeOfDataType)
{
    if (QSysInfo::ByteOrder == QSysInfo::LittleEndian) {
        return; // No need to flip
    } else if (QSysInfo::ByteOrder == QSysInfo::BigEndian) {
        // Need to flip
    } else {
        qWarning() << "QSSGLightmapLoader::convertEndian: Unknown endianness";
        return;
    }

    Q_ASSERT(buffer.size() % sizeOfDataType == 0);
    if (buffer.size() % sizeOfDataType != 0) {
        qWarning() << "QSSGLightmapLoader::convertEndian: Unexpected buffer size";
        return;
    }

    if (sizeOfDataType == 1)
        return;

    const int buffSize = buffer.size();
    for (int offset = 0; offset < buffSize; offset += sizeOfDataType) {
        for (int i = 0; i < sizeOfDataType / 2; ++i) {
            std::swap(buffer[offset + i], buffer[offset + sizeOfDataType - 1 - i]);
        }
    }
}

static QVariantMap byteArrayToMap(QByteArray input)
{
    QVariant variant;
    QDataStream in(input);
    in.setVersion(QDataStream::Qt_6_9);
    in.setByteOrder(QDataStream::LittleEndian);
    in >> variant;
    return variant.toMap();
}

bool QSSGLightmapIOPrivate::decodeHeaders()
{
    Q_ASSERT(stream);

    if (!stream->isOpen())
        stream->open(QIODeviceBase::OpenModeFlag::ReadOnly);

    if (!stream->isOpen()) {
        qWarning() << "QSSGLightmapIO: Stream is not openable";
        return false;
    }

    if (!stream->isReadable()) {
        qWarning() << "QSSGLightmapIO: Stream is not readable";
        return false;
    }

    // [Magic Bytes][Version]
    constexpr int headerSize = 6 * sizeof(char) + sizeof(qint32);
    // [Entry Count][Offset Index]
    constexpr int footerSize = 2 * sizeof(qint64);

    const qint64 fileSize = stream->size();
    if (fileSize < headerSize + footerSize) {
        qWarning() << "QSSGLightmapIO: File too small to contain header and footer";
        return false;
    }

    stream->seek(0);
    QByteArray headerData = stream->read(headerSize);
    if (headerData.size() != qsizetype(headerSize)) {
        qWarning() << "Failed to read header";
        return false;
    }

    // Verify file signature
    if (QByteArrayView(headerData.constData(), 6) != QByteArray::fromRawData(fileSignature, 6)) {
        qWarning() << "QSSGLightmapIO: Invalid file signature";
        return false;
    }

    qint32 fileVersion = -1;

    // Read file version (4 bytes after 6 magic bytes)
    const char *versionPtr = headerData.constData() + 6;
    fileVersion = qFromLittleEndian<qint32>(versionPtr);

    if (fileVersion != 1) {
        qWarning() << "QSSGLightmapIO: Invalid file version";
        return false;
    }

    // Seek to the last bytes (footer)
    if (!stream->seek(fileSize - footerSize)) {
        qWarning() << "Failed to seek to footer";
        return false;
    }

    QByteArray footerData = stream->read(footerSize);
    if (footerData.size() != qsizetype(footerSize)) {
        qWarning() << "Failed to read footer";
        return false;
    }

    const char *footerPtr = footerData.constData();
    const qint64 indexOffset = qFromLittleEndian<qint64>(footerPtr);
    const qint64 entryCount = qFromLittleEndian<qint64>(footerPtr + sizeof(qint64));

    this->entryCount = entryCount;
    this->indexOffset = indexOffset;
    this->fileVersion = fileVersion;
    this->fileSize = fileSize;

    return true;
}

bool QSSGLightmapIOPrivate::writeHeader() const
{
    Q_ASSERT(stream);
    if (!stream->isOpen())
        stream->open(QIODeviceBase::OpenModeFlag::WriteOnly);

    if (!stream->isOpen()) {
        qWarning() << "QSSGLightmapIO: File is not openable";
        return false;
    }

    if (!stream->isWritable()) {
        qWarning() << "QSSGLightmapIO: File is not writable";
        return false;
    }

    stream->seek(0);

    // Write file signature
    if (stream->write(fileSignature, sizeof(fileSignature) - 1) != sizeof(fileSignature) - 1) {
        qWarning() << "QSSGLightmapIO: Failed to write file signature";
        stream->close();
        return false;
    }

    // Write file version
    const qint32 version = qToLittleEndian<qint32>(1);
    if (stream->write(reinterpret_cast<const char *>(&version), sizeof(version)) != sizeof(version)) {
        qWarning() << "QSSGLightmapIO: Failed to write file version";
        stream->close();
        return false;
    }

    return true;
}

bool QSSGLightmapIOPrivate::writeData(const QString &key, const QByteArray &data)
{
    Q_ASSERT(stream->isOpen() && stream->isWritable());
    QByteArray keyBytes = keyToByteArray(key);
    Q_ASSERT(!entries.contains(keyBytes));

    IndexEntry &entry = entries[keyBytes];
    std::memcpy(entry.name, keyBytes.constData(), keyBytes.size());
    entry.offset = stream->pos();
    entry.size = data.size();

    if (stream->write(data) != data.size()) {
        qWarning() << "QSSGLightmapIO: Failed to write entry data";
        return false;
    }

    return true;
}

bool QSSGLightmapIOPrivate::writeFooter() const
{
    Q_ASSERT(stream);
    Q_ASSERT(stream->isOpen());
    Q_ASSERT(stream->isWritable());

    // The file should be seeked to the end of the data segment so we can just
    // add indices and footer now
    const qint64 indexOffset = qToLittleEndian<qint64>(stream->pos());
    const qint64 indexCount = qToLittleEndian<qint64>(entries.size());
    for (const IndexEntry &entry : std::as_const(entries)) {
        const qint64 offset = qToLittleEndian<qint64>(entry.offset);
        const qint64 size = qToLittleEndian<qint64>(entry.size);
        if (stream->write(entry.name, cNameLength) != cNameLength
            || stream->write(reinterpret_cast<const char *>(&offset), sizeof(offset)) != sizeof(offset)
            || stream->write(reinterpret_cast<const char *>(&size), sizeof(size)) != sizeof(size)) {
            qWarning() << "QSSGLightmapIO: Failed to write entry";
            return false;
        }
    }

    // Write footer
    if (stream->write(reinterpret_cast<const char *>(&indexOffset), sizeof(indexOffset)) != sizeof(indexOffset)
        || stream->write(reinterpret_cast<const char *>(&indexCount), sizeof(indexCount)) != sizeof(indexCount)) {
        qWarning() << "QSSGLightmapIO: Failed to write footer";
        return false;
    }
    stream->close();
    return true;
}

QByteArray QSSGLightmapIOPrivate::readKey(const QByteArray &key) const
{
    Q_ASSERT(stream);
    Q_ASSERT(entryCount >= 0);
    Q_ASSERT(indexOffset >= 0);
    Q_ASSERT(fileVersion >= 0);
    Q_ASSERT(fileSize >= 0);

    // Perform binary search by reading one IndexEntry at a time
    const char *keyData = key.constData();
    qint64 low = 0;
    qint64 high = entryCount;
    IndexEntry entry;

    bool found = false;
    qint64 matchOffset = 0;
    qint64 matchSize = 0;

    while (low < high) {
        qint64 mid = (low + high) / 2;
        qint64 offset = indexOffset + mid * sizeof(IndexEntry);

        if (!stream->seek(offset)) {
            qWarning() << "Failed to seek to index entry";
            return {};
        }

        if (stream->read(reinterpret_cast<char *>(&entry), sizeof(IndexEntry)) != sizeof(IndexEntry)) {
            qWarning() << "Failed to read index entry";
            return {};
        }

        int cmp = std::strncmp(entry.name, keyData, cNameLength);
        if (cmp < 0) {
            low = mid + 1;
        } else if (cmp > 0) {
            high = mid;
        } else {
            // Found
            found = true;
            matchOffset = qFromLittleEndian(entry.offset);
            matchSize = qFromLittleEndian(entry.size);
            break;
        }
    }

    if (!found) {
        qWarning() << "Key not found:" << key;
        return {};
    }

    if (matchOffset + matchSize > fileSize) {
        qWarning() << "Asset data out of bounds";
        return {};
    }

    // Seek and read asset
    if (!stream->seek(matchOffset)) {
        qWarning() << "Failed to seek to asset data";
        return {};
    }

    QByteArray assetData = stream->read(matchSize);
    if (assetData.size() != static_cast<int>(matchSize)) {
        qWarning() << "Failed to read full asset data";
        return {};
    }

    return assetData;
}

QList<QString> QSSGLightmapIOPrivate::getKeys() const
{
    Q_ASSERT(stream);
    Q_ASSERT(entryCount >= 0);
    Q_ASSERT(indexOffset >= 0);
    Q_ASSERT(fileVersion >= 0);
    Q_ASSERT(fileSize >= 0);

    QList<QString> keys;
    keys.resize(entryCount);

    IndexEntry entry;

    for (int i = 0; i < entryCount; ++i) {
        const qint64 offset = indexOffset + i * sizeof(IndexEntry);

        if (!stream->seek(offset)) {
            qWarning() << "Failed to seek to index entry";
            return {};
        }

        if (stream->read(reinterpret_cast<char *>(&entry), sizeof(IndexEntry)) != sizeof(IndexEntry)) {
            qWarning() << "Failed to read index entry";
            return {};
        }

        keys[i] = QString::fromUtf8(entry.name);
    }

    return keys;
}

/////////////////////////////// Loader /////////////////////////////////

QSSGLightmapLoader::QSSGLightmapLoader() : d(new QSSGLightmapIOPrivate) { };

QSSGLightmapLoader::~QSSGLightmapLoader()
{
    if (d->stream)
        d->stream->close();
    delete d;
}

QSharedPointer<QSSGLightmapLoader> QSSGLightmapLoader::open(const QSharedPointer<QIODevice> &stream)
{
    if (!stream) {
        qWarning() << "Failed to open lightmap file";
        return nullptr;
    }

    auto loader = QSharedPointer<QSSGLightmapLoader>(new QSSGLightmapLoader);
    loader->d->stream = stream;

    if (!loader->d->decodeHeaders()) {
        if (loader->d->stream->isOpen())
            loader->d->stream->close();
        return {};
    }

    return loader;
}

QSharedPointer<QSSGLightmapLoader> QSSGLightmapLoader::open(const QString &path)
{
    if (QSharedPointer<QIODevice> source = QSSGInputUtil::getStreamForFile(path))
        return open(source);

    return nullptr;
}

QByteArray QSSGLightmapLoader::readF32Image(const QString &key) const
{
    QByteArray buffer = d->readKey(keyToByteArray(key));
    convertEndian(buffer, sizeof(float));
    return buffer;
}

QByteArray QSSGLightmapLoader::readU32Image(const QString &key) const
{
    QByteArray buffer = d->readKey(keyToByteArray(key));
    convertEndian(buffer, sizeof(quint32));
    return buffer;
}

QByteArray QSSGLightmapLoader::readData(const QString &key) const
{
    return d->readKey(keyToByteArray(key));
}

QVariantMap QSSGLightmapLoader::readMetadata(const QString &key) const
{
    QByteArray metadataBuffer = d->readKey(keyToByteArray(key + QStringLiteral("_metadata")));
    QVariantMap metadata = byteArrayToMap(metadataBuffer);
    return metadata;
}

QList<QString> QSSGLightmapLoader::getKeys() const
{
    return d->getKeys();
}

inline int calculateLine(int width, int bitdepth)
{
    return ((width * bitdepth) + 7) / 8;
}
inline int calculatePitch(int line)
{
    return (line + 3) & ~3;
}

QSSGLoadedTexture *QSSGLightmapLoader::createTexture(QSharedPointer<QIODevice> stream,
                                                     const QSSGRenderTextureFormat &format,
                                                     const QString &key)
{
    QSharedPointer<QSSGLightmapLoader> loader = QSSGLightmapLoader::open(stream);
    if (!loader)
        return nullptr;

    QVariantMap metadata = loader->readMetadata(key);
    if (metadata.isEmpty())
        return nullptr;

    bool ok = true;
    const int w = metadata[QStringLiteral("width")].toInt(&ok);
    Q_ASSERT(ok);
    const int h = metadata[QStringLiteral("height")].toInt(&ok);
    Q_ASSERT(ok);
    QSize pixelSize = QSize(w, h);
    QByteArray imageFP32 = loader->readF32Image(key + QStringLiteral("_final"));
    if (imageFP32.isEmpty())
        return nullptr;

    // Setup Output container
    const int bytesPerPixel = format.getSizeofFormat();
    const int bitCount = bytesPerPixel * 8;
    const int pitch = calculatePitch(calculateLine(pixelSize.width(), bitCount));
    const size_t dataSize = pixelSize.height() * pitch;
    QSSG_CHECK_X(dataSize <= std::numeric_limits<quint32>::max(), "Requested data size exceeds 4GB limit!");
    QSSGLoadedTexture *imageData = new QSSGLoadedTexture;
    imageData->dataSizeInBytes = quint32(dataSize);
    imageData->data = ::malloc(imageData->dataSizeInBytes);
    imageData->width = pixelSize.width();
    imageData->height = pixelSize.height();
    imageData->format = format;
    imageData->components = format.getNumberOfComponent();
    imageData->isSRGB = false;

    std::array<float, 4> *source = reinterpret_cast<std::array<float, 4> *>(imageFP32.data());
    quint8 *target = reinterpret_cast<quint8 *>(imageData->data);

    int idx = 0;
    float rgbaF32[4];

    for (int y = imageData->height - 1; y >= 0; --y) {
        for (int x = 0; x < imageData->width; x++) {
            const int i = y * imageData->width + x;
            const int lh = i * 4 * sizeof(float);
            const int rh = imageFP32.size();
            Q_ASSERT(lh < rh);
            std::array<float, 4> v = source[i];
            rgbaF32[0] = v[0];
            rgbaF32[1] = v[1];
            rgbaF32[2] = v[2];
            rgbaF32[3] = v[3];

            format.encodeToPixel(rgbaF32, target, idx * bytesPerPixel);
            ++idx;
        }
    }

    return imageData;
}

///////////////////////////// Writer ///////////////////////////////////

QSSGLightmapWriter::QSSGLightmapWriter() : d(new QSSGLightmapIOPrivate) { }

QSSGLightmapWriter::~QSSGLightmapWriter()
{
    if (d->stream->isOpen()) {
        qWarning() << "Lightmap file open on destruction, closing";
        close();
    }
    delete d;
}

QSharedPointer<QSSGLightmapWriter> QSSGLightmapWriter::open(const QString &path)
{
    return open(QSharedPointer<QIODevice>(new QFile(path)));
}

QSharedPointer<QSSGLightmapWriter> QSSGLightmapWriter::open(const QSharedPointer<QIODevice> &stream)
{
    if (!stream) {
        qWarning() << "Failed to open lightmap file";
        return nullptr;
    }

    QSharedPointer<QSSGLightmapWriter> writer = QSharedPointer<QSSGLightmapWriter>(new QSSGLightmapWriter);
    if (!stream->isOpen() && !stream->open(QIODeviceBase::WriteOnly)) {
        qWarning() << "Failed to open lightmap file";
        return nullptr;
    }

    writer->d->stream = stream;
    if (!writer->d->writeHeader())
        return nullptr;

    return writer;
}

bool QSSGLightmapWriter::writeF32Image(const QString &key, const QByteArray &imageFP32)
{
    QByteArray buffer = QByteArray(imageFP32.constData(), imageFP32.size());
    convertEndian(buffer, sizeof(float));
    return d->writeData(key, buffer);
}

bool QSSGLightmapWriter::writeU32Image(const QString &key, const QByteArray &imageU32)
{
    QByteArray buffer = QByteArray(imageU32.constData(), imageU32.size());
    convertEndian(buffer, sizeof(quint32));
    return d->writeData(key, buffer);
}

bool QSSGLightmapWriter::writeData(const QString &key, const QByteArray &buffer)
{
    return d->writeData(key, buffer);
}

bool QSSGLightmapWriter::writeMetadata(const QString &key, const QVariantMap &metadata)
{
    return d->writeData(key + QStringLiteral("_metadata"), mapToByteArray(metadata));
}

bool QSSGLightmapWriter::close() const
{
    return d->writeFooter();
}

QT_END_NAMESPACE
