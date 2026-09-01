// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

// A FLOAT sample is two 16-bit words to PIZ.

#include <QtQuick3DTextureHelpers/private/qssgexrcodecs_p.h>

#include <QtCore/qendian.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qvarlengtharray.h>

#include <cstring>

QT_BEGIN_NAMESPACE

namespace QSSGExr {

namespace {

constexpr int HufEncSize = UShortRange + 1;
constexpr int HufDecBits = 14;
constexpr int HufDecSize = 1 << HufDecBits;
constexpr int HufDecMask = HufDecSize - 1;

constexpr int ShortZeroCodeRun = 59;
constexpr int LongZeroCodeRun = 63;
constexpr int ShortestLongRun = 2 + LongZeroCodeRun - ShortZeroCodeRun;
constexpr int MaxCodeLength = ShortZeroCodeRun - 1;
constexpr int MaxDecodableCodeLength = 57; // see BitReader::ensure()

class BitReader
{
public:
    BitReader(const char *begin, const char *end) : m_begin(begin), m_p(begin), m_end(end) { }

    bool getBits(int n, quint32 *out)
    {
        Q_ASSERT(n > 0 && n <= 24);
        if (!ensure(n))
            return false;
        *out = quint32(peek(n));
        drop(n);
        return true;
    }

    // Gated at 56 so that a refill cannot shift out unread bits.
    bool ensure(int n)
    {
        while (m_lc < n && m_lc <= 56 && m_p < m_end) {
            m_acc = (m_acc << 8) | quint8(*m_p++);
            m_lc += 8;
        }
        return m_lc >= n;
    }

    bool refillOne()
    {
        if (m_p >= m_end)
            return false;
        m_acc = (m_acc << 8) | quint8(*m_p++);
        m_lc += 8;
        return true;
    }

    quint64 peek(int n) const
    {
        Q_ASSERT(n > 0 && n <= m_lc && m_lc <= 64);
        return (m_acc >> (m_lc - n)) & ((quint64(1) << n) - 1);
    }

    void drop(int n) { m_lc -= n; }

    void dropTrailing(int n)
    {
        m_acc >>= n;
        m_lc -= n;
    }

    int bitCount() const { return m_lc; }
    quint64 accumulator() const { return m_acc; }
    bool atEnd() const { return m_p >= m_end; }
    qsizetype bytesConsumed() const { return m_p - m_begin; }

private:
    const char *m_begin;
    const char *m_p;
    const char *m_end;
    quint64 m_acc = 0;
    int m_lc = 0;
};

quint16 buildReverseLut(const quint8 *bitmap, quint16 *lut)
{
    int k = 0;
    // 0 is always present, the encoder clears its bit.
    for (int i = 0; i < UShortRange; ++i) {
        if (i == 0 || (bitmap[i >> 3] & (1 << (i & 7))))
            lut[k++] = quint16(i);
    }
    const int n = k - 1;
    while (k < UShortRange)
        lut[k++] = 0;
    return quint16(n);
}

void hufCanonicalCodeTable(qint64 *hcode)
{
    qint64 n[64] = {};
    for (int i = 0; i < HufEncSize; ++i) {
        Q_ASSERT(hcode[i] >= 0 && hcode[i] <= MaxCodeLength);
        n[hcode[i]] += 1;
    }

    qint64 c = 0;
    for (int i = MaxCodeLength; i > 0; --i) {
        const qint64 nc = (c + n[i]) >> 1;
        n[i] = c;
        c = nc;
    }

    for (int i = 0; i < HufEncSize; ++i) {
        const int l = int(hcode[i]);
        if (l > 0)
            hcode[i] = l | (n[l]++ << 6);
    }
}

bool hufUnpackEncTable(BitReader &br, int im, int iM, qint64 *hcode, QString *error)
{
    memset(hcode, 0, sizeof(qint64) * HufEncSize);

    for (int i = im; i <= iM; ++i) {
        quint32 l = 0;
        if (!br.getBits(6, &l)) {
            *error = QStringLiteral("truncated PIZ code length table");
            return false;
        }

        if (l == quint32(LongZeroCodeRun)) {
            quint32 n = 0;
            if (!br.getBits(8, &n)) {
                *error = QStringLiteral("truncated PIZ code length table");
                return false;
            }
            const int zerun = int(n) + ShortestLongRun;
            if (i + zerun > iM + 1) {
                *error = QStringLiteral("PIZ code length run overruns the symbol range");
                return false;
            }
            for (int k = 0; k < zerun; ++k)
                hcode[i + k] = 0;
            i += zerun - 1;
        } else if (l >= quint32(ShortZeroCodeRun)) {
            const int zerun = int(l) - ShortZeroCodeRun + 2;
            if (i + zerun > iM + 1) {
                *error = QStringLiteral("PIZ code length run overruns the symbol range");
                return false;
            }
            for (int k = 0; k < zerun; ++k)
                hcode[i + k] = 0;
            i += zerun - 1;
        } else {
            hcode[i] = l;
        }
    }

    hufCanonicalCodeTable(hcode);
    return true;
}

bool hufBuildDecTable(const qint64 *hcode, int im, int iM, PizScratch *scratch, QString *error)
{
    std::vector<HufDecEntry> &dec = scratch->dec;
    std::vector<quint32> &longSymbols = scratch->longSymbols;
    std::fill(dec.begin(), dec.end(), HufDecEntry {});

    for (int i = im; i <= iM; ++i) {
        const qint64 hc = hcode[i];
        const int l = int(hc & 63);
        if (l == 0)
            continue;
        if (l > MaxDecodableCodeLength) {
            *error = QStringLiteral("invalid PIZ code length");
            return false;
        }
        const quint64 code = quint64(hc >> 6);
        if (code >> l) {
            *error = QStringLiteral("invalid PIZ code table");
            return false;
        }

        if (l > HufDecBits) {
            const quint32 idx = quint32(code >> (l - HufDecBits));
            Q_ASSERT(idx < quint32(HufDecSize));
            if (dec[idx].len != 0) {
                *error = QStringLiteral("conflicting PIZ codes");
                return false;
            }
            ++dec[idx].longCount;
        } else {
            const quint32 base = quint32(code << (HufDecBits - l));
            const quint32 count = 1u << (HufDecBits - l);
            Q_ASSERT(quint64(base) + count <= quint64(HufDecSize));
            for (quint32 j = 0; j < count; ++j) {
                HufDecEntry &e = dec[base + j];
                if (e.len != 0 || e.longCount != 0) {
                    *error = QStringLiteral("conflicting PIZ codes");
                    return false;
                }
                e.len = quint8(l);
                e.lit = quint32(i);
            }
        }
    }

    quint32 total = 0;
    for (HufDecEntry &e : dec) {
        e.longOfs = total;
        total += e.longCount;
    }
    longSymbols.assign(total, 0);

    std::vector<quint32> &filled = scratch->longSymbolsFilled;
    filled.assign(dec.size(), 0);
    for (int i = im; i <= iM; ++i) {
        const qint64 hc = hcode[i];
        const int l = int(hc & 63);
        if (l <= HufDecBits)
            continue;
        const quint64 code = quint64(hc >> 6);
        const quint32 idx = quint32(code >> (l - HufDecBits));
        longSymbols[dec[idx].longOfs + filled[idx]++] = quint32(i);
    }

    return true;
}

bool hufDecode(const qint64 *hcode,
               const std::vector<HufDecEntry> &dec,
               const std::vector<quint32> &longSymbols,
               QByteArrayView bits,
               qint64 nBits,
               int rlc,
               QSpan<quint16> out,
               QString *error)
{
    const qsizetype byteCount = (nBits + 7) / 8;
    Q_ASSERT(byteCount <= bits.size());
    BitReader br(bits.data(), bits.data() + byteCount);

    quint16 *o = out.data();
    qsizetype produced = 0;

    const auto emitSymbol = [&](quint32 sym) -> bool {
        if (sym == quint32(rlc)) {
            if (!br.ensure(8)) {
                *error = QStringLiteral("truncated PIZ run length");
                return false;
            }
            const quint32 count = quint32(br.peek(8));
            br.drop(8);
            if (produced == 0) {
                *error = QStringLiteral("PIZ run with nothing to repeat");
                return false;
            }
            if (count > quint32(out.size() - produced)) {
                *error = QStringLiteral("PIZ run overruns the chunk");
                return false;
            }
            const quint16 s = o[produced - 1];
            for (quint32 k = 0; k < count; ++k)
                o[produced++] = s;
            return true;
        }
        if (produced >= out.size()) {
            *error = QStringLiteral("PIZ stream overruns the chunk");
            return false;
        }
        if (sym > 0xffff) {
            *error = QStringLiteral("invalid PIZ symbol");
            return false;
        }
        o[produced++] = quint16(sym);
        return true;
    };

    while (!br.atEnd()) {
        br.refillOne();

        while (br.bitCount() >= HufDecBits) {
            const HufDecEntry &e = dec[br.peek(HufDecBits) & HufDecMask];
            if (e.len) {
                br.drop(e.len);
                if (!emitSymbol(e.lit))
                    return false;
            } else {
                if (e.longCount == 0) {
                    *error = QStringLiteral("invalid PIZ code");
                    return false;
                }
                bool found = false;
                for (quint32 j = 0; j < e.longCount; ++j) {
                    const quint32 sym = longSymbols[e.longOfs + j];
                    const int l = int(hcode[sym] & 63);
                    if (!br.ensure(l))
                        continue;
                    if (quint64(hcode[sym] >> 6) == br.peek(l)) {
                        br.drop(l);
                        if (!emitSymbol(sym))
                            return false;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    *error = QStringLiteral("invalid PIZ code");
                    return false;
                }
            }
        }
    }

    // The last code may run into the encoder's zero padding.
    br.dropTrailing(int((8 - (nBits & 7)) & 7));
    while (br.bitCount() > 0) {
        const int lc = br.bitCount();
        Q_ASSERT(lc < HufDecBits);
        const HufDecEntry &e = dec[(br.accumulator() << (HufDecBits - lc)) & HufDecMask];
        if (e.len == 0) {
            *error = QStringLiteral("invalid PIZ code in the trailing bits");
            return false;
        }
        br.drop(e.len);
        if (!emitSymbol(e.lit))
            return false;
    }

    if (produced != out.size()) {
        *error = QStringLiteral("PIZ stream does not fill the chunk");
        return false;
    }
    return true;
}

inline void wdec14(quint16 l, quint16 h, quint16 *a, quint16 *b)
{
    const int ls = qint16(l);
    const int hi = qint16(h);
    const int ai = ls + (hi & 1) + (hi >> 1);
    *a = quint16(qint16(ai));
    *b = quint16(qint16(ai - hi));
}

inline void wdec16(quint16 l, quint16 h, quint16 *a, quint16 *b)
{
    constexpr int ModMask = 0xffff;
    constexpr int AOffset = 1 << 15;
    const int m = l;
    const int d = h;
    const int bb = (m - (d >> 1)) & ModMask;
    const int aa = (d + bb - AOffset) & ModMask;
    *b = quint16(bb);
    *a = quint16(aa);
}

} // namespace

PizScratch::PizScratch()
{
    hcode.assign(HufEncSize, 0);
    dec.assign(HufDecSize, HufDecEntry {});
}

// Indices, since the reference's pointer arithmetic can point before the buffer.
void wav2Decode(QSpan<quint16> base, qint64 offset, qint64 nx, qint64 ox, qint64 ny, qint64 oy, quint16 maxValue)
{
    quint16 *in = base.data();
    const bool w14 = maxValue < (1 << 14);
    const qint64 n = qMin(nx, ny);

    qint64 p = 1;
    while (p <= n)
        p <<= 1;
    p >>= 1;
    qint64 p2 = p;
    p >>= 1;

    while (p >= 1) {
        const qint64 oy1 = oy * p;
        const qint64 oy2 = oy * p2;
        const qint64 ox1 = ox * p;
        const qint64 ox2 = ox * p2;

        qint64 py = offset;
        const qint64 ey = offset + oy * (ny - p2);
        quint16 i00 = 0, i01 = 0, i10 = 0, i11 = 0;

        for (; py <= ey; py += oy2) {
            qint64 px = py;
            const qint64 ex = py + ox * (nx - p2);
            for (; px <= ex; px += ox2) {
                const qint64 p01 = px + ox1;
                const qint64 p10 = px + oy1;
                const qint64 p11 = p10 + ox1;
                Q_ASSERT(px >= 0 && p11 < base.size());
                if (w14) {
                    wdec14(in[px], in[p10], &i00, &i10);
                    wdec14(in[p01], in[p11], &i01, &i11);
                    wdec14(i00, i01, &in[px], &in[p01]);
                    wdec14(i10, i11, &in[p10], &in[p11]);
                } else {
                    wdec16(in[px], in[p10], &i00, &i10);
                    wdec16(in[p01], in[p11], &i01, &i11);
                    wdec16(i00, i01, &in[px], &in[p01]);
                    wdec16(i10, i11, &in[p10], &in[p11]);
                }
            }

            if (nx & p) {
                const qint64 p10 = px + oy1;
                Q_ASSERT(px >= 0 && p10 < base.size());
                if (w14)
                    wdec14(in[px], in[p10], &i00, &in[p10]);
                else
                    wdec16(in[px], in[p10], &i00, &in[p10]);
                in[px] = i00;
            }
        }

        if (ny & p) {
            qint64 px = py;
            const qint64 ex = py + ox * (nx - p2);
            for (; px <= ex; px += ox2) {
                const qint64 p01 = px + ox1;
                Q_ASSERT(px >= 0 && p01 < base.size());
                if (w14)
                    wdec14(in[px], in[p01], &i00, &in[p01]);
                else
                    wdec16(in[px], in[p01], &i00, &in[p01]);
                in[px] = i00;
            }
        }

        p2 = p;
        p >>= 1;
    }
}

bool hufUncompress(QByteArrayView in, QSpan<quint16> out, PizScratch *scratch, QString *error)
{
    if (in.size() < 20) {
        *error = QStringLiteral("truncated PIZ Huffman header");
        return false;
    }

    const quint32 im = qFromLittleEndian<quint32>(in.data());
    const quint32 iM = qFromLittleEndian<quint32>(in.data() + 4);
    // Bytes 8..11, the packed table length, are unused.
    const quint32 nBits = qFromLittleEndian<quint32>(in.data() + 12);

    if (im > quint32(HufEncSize - 1) || iM > quint32(HufEncSize - 1) || im > iM) {
        *error = QStringLiteral("invalid PIZ symbol range");
        return false;
    }

    BitReader tableReader(in.data() + 20, in.data() + in.size());
    if (!hufUnpackEncTable(tableReader, int(im), int(iM), scratch->hcode.data(), error))
        return false;

    if (!hufBuildDecTable(scratch->hcode.data(), int(im), int(iM), scratch, error))
        return false;

    const qsizetype dataStart = 20 + tableReader.bytesConsumed();
    const qsizetype avail = in.size() - dataStart;
    if (avail < 0 || qint64(nBits) > qint64(avail) * 8) {
        *error = QStringLiteral("PIZ bit stream extends past the chunk");
        return false;
    }

    // The run length escape is iM, not a fixed constant.
    return hufDecode(scratch->hcode.data(), scratch->dec, scratch->longSymbols, in.sliced(dataStart), qint64(nBits), int(iM), out, error);
}

bool pizDecode(QByteArrayView in, QSpan<char> out, const QList<PizChannel> &channels, qint64 nx, qint64 ny, qint64 bytesPerPixel, PizScratch *scratch, QString *error)
{
    if (out.size() % 2 != 0) {
        *error = QStringLiteral("PIZ chunk size is not a whole number of 16-bit words");
        return false;
    }

    qsizetype pos = 0;
    if (in.size() < 4) {
        *error = QStringLiteral("truncated PIZ chunk");
        return false;
    }
    const quint16 minNonZero = qFromLittleEndian<quint16>(in.data());
    const quint16 maxNonZero = qFromLittleEndian<quint16>(in.data() + 2);
    pos = 4;

    scratch->bitmap.fill(0);
    if (minNonZero <= maxNonZero) {
        if (maxNonZero >= BitmapSize) {
            *error = QStringLiteral("PIZ bitmap range is out of bounds");
            return false;
        }
        const qsizetype n = qsizetype(maxNonZero) - minNonZero + 1;
        if (in.size() - pos < n) {
            *error = QStringLiteral("truncated PIZ bitmap");
            return false;
        }
        memcpy(scratch->bitmap.data() + minNonZero, in.data() + pos, size_t(n));
        pos += n;
    } else if (!(minNonZero == BitmapSize - 1 && maxNonZero == 0)) {
        // Only the all-zero bitmap inverts the range.
        *error = QStringLiteral("invalid PIZ bitmap range");
        return false;
    }

    const quint16 maxValue = buildReverseLut(scratch->bitmap.data(), scratch->lut.data());

    if (in.size() - pos < 4) {
        *error = QStringLiteral("truncated PIZ chunk");
        return false;
    }
    const qint64 huffLength = qint64(qFromLittleEndian<qint32>(in.data() + pos));
    pos += 4;
    if (huffLength < 0 || huffLength > in.size() - pos) {
        *error = QStringLiteral("PIZ Huffman block extends past the chunk");
        return false;
    }

    const qint64 totalWords = out.size() / 2;
    QVarLengthArray<qint64, 8> wordOffset(channels.size() + 1);
    wordOffset[0] = 0;
    for (qsizetype c = 0; c < channels.size(); ++c) {
        qint64 planeWords = 0;
        if (qMulOverflow(nx, ny, &planeWords) || qMulOverflow(planeWords, channels[c].wordsPerSample, &planeWords)
            || qAddOverflow(wordOffset[c], planeWords, &wordOffset[c + 1])) {
            *error = QStringLiteral("PIZ channel layout is too large");
            return false;
        }
    }
    if (wordOffset[channels.size()] != totalWords) {
        *error = QStringLiteral("PIZ channel layout does not match the chunk size");
        return false;
    }

    scratch->planar.assign(size_t(totalWords), 0);
    QSpan<quint16> planar(scratch->planar.data(), qsizetype(totalWords));

    if (!hufUncompress(in.sliced(pos, huffLength), planar, scratch, error))
        return false;

    for (qsizetype c = 0; c < channels.size(); ++c) {
        const qint64 words = channels[c].wordsPerSample;
        for (qint64 j = 0; j < words; ++j)
            wav2Decode(planar, wordOffset[c] + j, nx, words, ny, nx * words, maxValue);
    }

    // The chunk is little endian.
    const quint16 *lut = scratch->lut.data();
    for (qint64 i = 0; i < totalWords; ++i)
        planar[i] = qToLittleEndian(lut[planar[i]]);

    const qint64 rowStride = nx * bytesPerPixel;
    for (qint64 r = 0; r < ny; ++r) {
        for (qsizetype c = 0; c < channels.size(); ++c) {
            const qint64 words = channels[c].wordsPerSample;
            const qint64 runWords = nx * words;
            char *dst = out.data() + r * rowStride + channels[c].byteOffsetInPixel * nx;
            const quint16 *src = planar.data() + wordOffset[c] + r * runWords;
            Q_ASSERT(dst + runWords * 2 <= out.data() + out.size());
            memcpy(dst, src, size_t(runWords) * 2);
        }
    }

    return true;
}

} // namespace QSSGExr

QT_END_NAMESPACE
