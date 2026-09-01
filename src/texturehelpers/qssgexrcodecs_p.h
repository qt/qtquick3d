// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QSSGEXRCODECS_P_H
#define QSSGEXRCODECS_P_H

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

#include <QtCore/qbytearrayview.h>
#include <QtCore/qlist.h>
#include <QtCore/qspan.h>
#include <QtCore/qstring.h>

#include <array>
#include <limits>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QSSGExr {

constexpr int UShortRange = std::numeric_limits<quint16>::max() + 1;
constexpr int BitmapSize = UShortRange >> 3;

bool undoPredictorAndDeinterleave(QSpan<char> out, QSpan<char> scratch, QString *error);

bool inflateExact(QByteArrayView in, QSpan<char> out, QString *error);

bool rleDecode(QByteArrayView in, QSpan<char> out, QString *error);

struct PizChannel
{
    qint64 wordsPerSample = 1;
    qint64 byteOffsetInPixel = 0;
};

// A zero len marks a bucket of longer codes in longSymbols.
struct HufDecEntry
{
    quint32 lit = 0;
    quint32 longOfs = 0;
    quint32 longCount = 0;
    quint8 len = 0;
};

class PizScratch
{
public:
    PizScratch();

    std::array<quint16, UShortRange> lut {};
    std::array<quint8, BitmapSize> bitmap {};
    std::vector<qint64> hcode; // (code << 6) | length
    std::vector<HufDecEntry> dec;
    std::vector<quint32> longSymbols;
    std::vector<quint32> longSymbolsFilled;
    std::vector<quint16> planar;
};

bool pizDecode(QByteArrayView in,
               QSpan<char> out,
               const QList<PizChannel> &channels,
               qint64 nx,
               qint64 ny,
               qint64 bytesPerPixel,
               PizScratch *scratch,
               QString *error);

// Exposed for the tests.
bool hufUncompress(QByteArrayView in, QSpan<quint16> out, PizScratch *scratch, QString *error);
void wav2Decode(QSpan<quint16> base, qint64 offset, qint64 nx, qint64 ox, qint64 ny, qint64 oy, quint16 maxValue);

} // namespace QSSGExr

QT_END_NAMESPACE

#endif // QSSGEXRCODECS_P_H
