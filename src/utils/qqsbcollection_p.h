/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

#ifndef QQSBCOLLECTION_H
#define QQSBCOLLECTION_H

#include <QtQuick3DUtils/private/qtquick3dutilsglobal_p.h>

#include <QtCore/qfile.h>
#include <QtCore/qset.h>

#include <QtGui/private/qshader_p.h>

QT_BEGIN_NAMESPACE

class QRhiShaderStage;
typedef QMap<QByteArray, bool> QQsbShaderFeatureSet;

class Q_QUICK3DUTILS_EXPORT QQsbCollection
{
public:
    enum Version : quint8
    {
        Unknown,
        One = 0x10
        // NOTE: Remember QDataStream version
    };

    enum MapMode
    {
        Read = QIODevice::ReadOnly,
        Write = (QIODevice::WriteOnly | QIODevice::Truncate)
    };

    struct Entry
    {
        Entry() = default;
        explicit Entry(size_t key) : hkey(key) {}
        Entry(size_t key, qint64 offset_) : hkey(key), offset(offset_) {}
        inline bool isValid() const { return (hkey && offset >= 0); }
        size_t hkey = 0;
        qint64 offset = -1;
    };

    using EntryMap = QSet<Entry>;

    explicit QQsbCollection(const QString &filePath);
    explicit QQsbCollection(QIODevice &dev);
    ~QQsbCollection();

    bool map(MapMode mode);
    void unmap();

    EntryMap getEntries() const { return entries; }
    bool extractQsbEntry(Entry entry, QByteArray *outDesc, QQsbShaderFeatureSet *featureSet, QShader *outVertShader, QShader *outFragShader);

    Entry addQsbEntry(const QByteArray &description, const QQsbShaderFeatureSet &featureSet, const QShader &vert, const QShader &frag, size_t hkey);
    bool removeQsbEntry();

    QString fileName() const;
    void setFileName(const QString &fileName);

    static void dumpQsbcInfo(const QString &device);
    static void dumpQsbcInfo(QIODevice &device);

private:
    Q_DISABLE_COPY(QQsbCollection);
    static void dumpQsbcInfoImp(QQsbCollection &qsbc);

    enum class DeviceOwner : quint8
    {
        Self,
        Extern
    };
    QFile file;
    QIODevice &device;
    EntryMap entries;
    DeviceOwner devOwner = DeviceOwner::Self;
    quint8 version = Version::Unknown;
};

Q_DECLARE_TYPEINFO(QQsbCollection::Entry, Q_PRIMITIVE_TYPE);

Q_QUICK3DUTILS_EXPORT QDataStream &operator<<(QDataStream &stream, const QQsbCollection::Entry &entry);
Q_QUICK3DUTILS_EXPORT QDataStream &operator>>(QDataStream &stream, QQsbCollection::Entry &entry);

Q_QUICK3DUTILS_EXPORT size_t qHash(const QQsbCollection::Entry &entry, size_t);
Q_QUICK3DUTILS_EXPORT bool operator==(const QQsbCollection::Entry &l, const QQsbCollection::Entry &r);

QT_END_NAMESPACE

#endif // QQSBCOLLECTION_H
