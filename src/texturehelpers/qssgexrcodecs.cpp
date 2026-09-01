// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include <QtQuick3DTextureHelpers/private/qssgexrcodecs_p.h>

#include <QtCore/qnumeric.h>

#include <zlib.h>

#include <cstring>

QT_BEGIN_NAMESPACE

namespace QSSGExr {

bool undoPredictorAndDeinterleave(QSpan<char> out, QSpan<char> scratch, QString *error)
{
    const qsizetype n = out.size();
    if (scratch.size() < n) {
        *error = QStringLiteral("predictor scratch is smaller than the chunk");
        return false;
    }
    if (n <= 0)
        return true;

    quint8 *t = reinterpret_cast<quint8 *>(scratch.data());
    for (qsizetype i = 1; i < n; ++i)
        t[i] = quint8(quint32(t[i - 1]) + quint32(t[i]) - 128u);

    const qsizetype half = (n + 1) / 2;
    const quint8 *t1 = t;
    const quint8 *t2 = t + half;
    quint8 *s = reinterpret_cast<quint8 *>(out.data());
    for (qsizetype i = 0; i < n; i += 2) {
        s[i] = *t1++;
        if (i + 1 < n)
            s[i + 1] = *t2++;
    }

    return true;
}

bool inflateExact(QByteArrayView in, QSpan<char> out, QString *error)
{
    if (in.isEmpty() || out.isEmpty()) {
        *error = QStringLiteral("empty compressed chunk");
        return false;
    }
    if (quint64(in.size()) > std::numeric_limits<uInt>::max() || quint64(out.size()) > std::numeric_limits<uInt>::max()) {
        *error = QStringLiteral("compressed chunk is too large");
        return false;
    }

    z_stream zs = {};
    if (inflateInit(&zs) != Z_OK) {
        *error = QStringLiteral("could not initialise zlib");
        return false;
    }

    zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(in.data()));
    zs.avail_in = uInt(in.size());
    zs.next_out = reinterpret_cast<Bytef *>(out.data());
    zs.avail_out = uInt(out.size());

    const int ret = inflate(&zs, Z_FINISH);
    const bool complete = ret == Z_STREAM_END;
    const bool exact = zs.avail_out == 0 && zs.avail_in == 0;
    inflateEnd(&zs);

    if (!complete || !exact) {
        *error = QStringLiteral("corrupt zlib stream in chunk");
        return false;
    }
    return true;
}

bool rleDecode(QByteArrayView in, QSpan<char> out, QString *error)
{
    const qint8 *ip = reinterpret_cast<const qint8 *>(in.data());
    const qint8 *iend = ip + in.size();
    char *op = out.data();
    qsizetype produced = 0;

    while (ip < iend) {
        const qint8 tag = *ip++;
        if (tag < 0) {
            const qsizetype count = -qsizetype(tag);
            if (count > iend - ip || count > out.size() - produced) {
                *error = QStringLiteral("corrupt RLE stream in chunk");
                return false;
            }
            memcpy(op + produced, ip, size_t(count));
            ip += count;
            produced += count;
        } else {
            const qsizetype count = qsizetype(tag) + 1;
            if (ip >= iend || count > out.size() - produced) {
                *error = QStringLiteral("corrupt RLE stream in chunk");
                return false;
            }
            memset(op + produced, *reinterpret_cast<const char *>(ip), size_t(count));
            ++ip;
            produced += count;
        }
    }

    if (produced != out.size()) {
        *error = QStringLiteral("RLE stream does not fill the chunk");
        return false;
    }
    return true;
}

} // namespace QSSGExr

QT_END_NAMESPACE
