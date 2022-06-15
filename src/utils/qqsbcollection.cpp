// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qqsbcollection_p.h"

#include <QtGui/private/qrhi_p.h>

QT_BEGIN_NAMESPACE

static const char *borderText() { return "--------------------------------------------------------------------------------"; }
static constexpr quint64 MagicaDS = 0x3933333335346337;
static constexpr qint64 HeaderSize = sizeof(quint64 /*startOffs*/) + sizeof(MagicaDS) + sizeof(decltype(QQsbCollection::Version::One));

bool QQsbCollection::map(MapMode mode)
{
    if (device.isOpen()) {
        // Make sure Truncate is set if we're writing.
        if ((device.openMode() & QIODevice::WriteOnly) != 0) {
            if ((device.openMode() & QIODevice::Truncate) == 0) {
                qWarning("Open mode needs to have Truncate set for writing!");
                return false;
            }
            if ((device.openMode() & QIODevice::Text) != 0) {
                qWarning("Open mode can't have Text mode set!");
                return false;
            }
        }
    } else if (!device.open(QIODevice::OpenMode(mode))) {
        qWarning("Unable to open device!");
        return false;
    }

    if (mode == Write)
        return true;

    Q_ASSERT(mode == Read);

    bool ret = false;
    const qint64 size = device.size();
    if (device.seek(size - HeaderSize)) {
        QDataStream ds(&device);
        ds.setVersion(QDataStream::Qt_6_0);
        quint64 fileId = 0;
        qint64 start = 0;
        ds >> start >> version >> fileId;
        if (fileId == MagicaDS && version == Version::One) {
            if (start >= 0 && start < size && device.seek(start)) {
                ds >> entries;
                ret = true;
            }
        }
    }

    if (!ret)
        unmap();

    return ret;
}

void QQsbCollection::unmap()
{
    if (device.isOpen() && ((device.openMode() & Write) == Write)) {
        if (!entries.isEmpty()) {
            if (!device.atEnd()) {
                device.seek(device.size() - 1);
                Q_ASSERT(device.atEnd());
            }
            QDataStream ds(&device);
            const auto start = device.pos();
            ds << entries << start << decltype(version)(Version::One) << MagicaDS;
        } else {
            if (devOwner == DeviceOwner::Self)
                file.remove();
        }
    }
    device.close();
    entries.clear();
}

bool QQsbCollection::extractQsbEntry(QQsbCollection::Entry entry, QByteArray *outDesc, QQsbShaderFeatureSet *featureSet, QShader *outVertShader, QShader *outFragShader)
{
    if (device.isOpen() && device.isReadable()) {
        if (entry.isValid()) {
            const int size = device.size();
            const int offset = entry.offset;
            if (size > offset && device.seek(offset)) {
                QDataStream ds(&device);
                ds.setVersion(QDataStream::Qt_6_0);
                QByteArray desc;
                QQsbShaderFeatureSet fs;
                QByteArray vertData;
                QByteArray fragData;
                ds >> desc >> fs >> vertData >> fragData;
                if (outDesc)
                    *outDesc = desc;
                if (outVertShader)
                    *outVertShader = QShader::fromSerialized(vertData);
                if (outFragShader)
                    *outFragShader = QShader::fromSerialized(fragData);
                if (featureSet)
                    *featureSet = fs;
                return true;
            }
        } else {
            qWarning("Entry not found id(%zu), offset(%lld)", entry.hkey, entry.offset);
        }
    } else {
        qWarning("Unable to open file for reading");
    }

    return false;
}

QQsbCollection::QQsbCollection(const QString &filePath)
    : file(filePath)
    , device(file)
{
}

QQsbCollection::QQsbCollection(QIODevice &dev)
    : device(dev)
    , devOwner(DeviceOwner::Extern)
{

}

QQsbCollection::~QQsbCollection()
{
    if (!entries.isEmpty() || device.isOpen())
        unmap();
}

void QQsbCollection::dumpQsbcInfoImp(QQsbCollection &qsbc)
{
    if (qsbc.map(QQsbCollection::Read)) {
        const auto entries = qsbc.getEntries();
        qDebug("Number of entries in collection: %zu\n", size_t(entries.size()));
        int i = 0;
        qDebug("Qsbc version: %uc", qsbc.version);
        for (const auto &e : qAsConst(entries)) {
            qDebug("%s\n"
                   "Entry %d\n%s\n"
                   "Key: %zu\n"
                   "Offset: %llu", borderText(), i++, borderText(), e.hkey, e.offset);

            QByteArray descr;
            QQsbShaderFeatureSet featureSet;
            QShader vertShader;
            QShader fragShader;
            if (qsbc.extractQsbEntry(e, &descr, &featureSet, &vertShader, &fragShader)) {
                qDebug() << descr << Qt::endl
                         << featureSet << Qt::endl
                         << vertShader << Qt::endl
                         << fragShader;
            } else {
                qWarning("Extracting Qsb entry failed!");
            }
        }
    }
    qsbc.unmap();
}

void QQsbCollection::dumpQsbcInfo(const QString &file)
{
    QQsbCollection qsbc(file);
    dumpQsbcInfoImp(qsbc);
}

void QQsbCollection::dumpQsbcInfo(QIODevice &device)
{
    QQsbCollection qsbc(device);
    dumpQsbcInfoImp(qsbc);
}

QQsbCollection::Entry QQsbCollection::addQsbEntry(const QByteArray &description, const QQsbShaderFeatureSet &featureSet, const QShader &vert, const QShader &frag, size_t hkey)
{
    if (hkey && !entries.contains(Entry{hkey})) {
        if (map(MapMode::Write)) {
            if (vert.isValid() && frag.isValid()) {
                QDataStream ds(&device);
                ds.setVersion(QDataStream::Qt_6_0);
                const auto offset = device.pos();
                ds << description << featureSet << vert.serialized() << frag.serialized();
                return *entries.insert({hkey, offset });
            }
        }
    }

    return Entry();
}

QString QQsbCollection::fileName() const
{
    return (devOwner == DeviceOwner::Self) ? file.fileName() : QString();
}

void QQsbCollection::setFileName(const QString &fileName)
{
    if (devOwner == DeviceOwner::Extern)
        return;

    Q_ASSERT(&device == &file);
    if (file.isOpen() && file.fileName() != fileName) {
        qWarning("Setting filename while collection is still mapped!");
        unmap();
    }

    file.setFileName(fileName);
}

QDataStream &operator<<(QDataStream &stream, const QQsbCollection::Entry &entry)
{
    return (stream << quint64(entry.hkey) << entry.offset);
}

QDataStream &operator>>(QDataStream &stream, QQsbCollection::Entry &entry)
{
    quint64 hkey;
    qint64 offset;
    stream >> hkey >> offset;
    entry = { size_t(hkey), offset };
    return stream;
}

size_t qHash(const QQsbCollection::Entry &entry, size_t) { return entry.hkey; }
bool operator==(const QQsbCollection::Entry &l, const QQsbCollection::Entry &r) { return (l.hkey == r.hkey); }

QT_END_NAMESPACE
