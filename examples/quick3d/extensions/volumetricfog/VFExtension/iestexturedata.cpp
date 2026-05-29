// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "iestexturedata.h"
#include <QFile>
#include <QTextStream>
#include <QtMath>
#include <QPair>
#include <QSize>
#include <QQmlContext>

IESTextureData::IESTextureData(QQuick3DObject *parent)
    : QQuick3DTextureData(parent)
{
    setFormat(QQuick3DTextureData::R16F);
    setHasTransparency(false);
}

QList<QUrl> IESTextureData::sources() const { return m_sources; }

void IESTextureData::setSources(const QList<QUrl> &sources)
{
    if (m_sources != sources) {
        m_sources = sources;
        parseIESFiles();
        emit sourcesChanged();
    }
}

void IESTextureData::parseIESFiles()
{
    if (m_sources.isEmpty()) { qWarning() << "No IES files provided"; return; }

    QVector<IESData> dataList;
    dataList.reserve(m_sources.size());
    for (const QUrl &source : std::as_const(m_sources)) {
        IESData data = parseIESFile(source);
        if (data.numVerticalAngles > 0 && data.numHorizontalAngles > 0)
            dataList.append(data);
    }

    if (dataList.isEmpty()) { qWarning() << "No valid IES files could be parsed"; return; }
    generateTexture(dataList);
}

IESTextureData::IESData IESTextureData::parseIESFile(const QUrl &source)
{
    QUrl resolvedSource = qmlContext(this)->parentContext()->resolvedUrl(source);
    QString path = resolvedSource.scheme() == "qrc" ? ":" + resolvedSource.path() :
                       resolvedSource.toLocalFile().isEmpty() ? resolvedSource.path() : resolvedSource.toLocalFile();
    QFile file(path);
    IESData data;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open IES file:" << resolvedSource << path;
        return data;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    while (!in.atEnd() && !in.readLine().trimmed().startsWith("TILT="));
    if (in.atEnd()) return data;

    auto params = readFloats(in, 10);
    if (params.size() < 10) return data;

    data.numVerticalAngles   = params[3];
    data.numHorizontalAngles = params[4];
    data.candelaMultiplier   = params[2];

    readFloats(in, 3);
    data.verticalAngles   = readFloats(in, data.numVerticalAngles);
    data.horizontalAngles = readFloats(in, data.numHorizontalAngles);

    data.candelaValues.resize(data.numHorizontalAngles);
    data.maxCandela = 0.0f;
    for (int h = 0; h < data.numHorizontalAngles; ++h) {
        data.candelaValues[h] = readFloats(in, data.numVerticalAngles);
        for (const float &v : data.candelaValues.at(h))
            data.maxCandela = qMax(data.maxCandela, v * data.candelaMultiplier);
    }
    return data;
}

QVector<float> IESTextureData::readFloats(QTextStream &in, int count)
{
    QVector<float> vals;
    vals.reserve(count);
    while (vals.size() < count && !in.atEnd()) {
        auto sd = in.readLine().split(' ', Qt::SkipEmptyParts);
        for (const QString &s : std::as_const(sd)) {
            bool ok;
            float f = s.toFloat(&ok);
            if (ok && vals.size() < count) vals.append(f);
        }
    }
    return vals;
}

static void octToDir(float u, float v, float &dx, float &dy, float &dz)
{
    float ox = u * 2.0f - 1.0f;
    float oy = v * 2.0f - 1.0f;
    float oz = 1.0f - qAbs(ox) - qAbs(oy);
    if (oz < 0.0f) {
        float tx = ox, ty = oy;
        ox = (1.0f - qAbs(ty)) * (tx >= 0.0f ? 1.0f : -1.0f);
        oy = (1.0f - qAbs(tx)) * (ty >= 0.0f ? 1.0f : -1.0f);
    }
    float len = qSqrt(ox*ox + oy*oy + oz*oz);
    dx = ox / len;
    dy = oy / len;
    dz = oz / len;
}

void IESTextureData::generateTexture(const QVector<IESData> &dataList)
{
    const int sliceW = 254;
    const int sliceH = 254;
    const int padding = 1;
    const int paddedW = sliceW + padding * 2;
    const int count = dataList.size();
    const int totalW = paddedW * count;
    const int totalH = sliceH + padding * 2;

    QByteArray texData;
    texData.resize(totalW * totalH * sizeof(qfloat16));
    qfloat16 *ptr = reinterpret_cast<qfloat16*>(texData.data());
    memset(texData.data(), 0, texData.size());

    const float RAD2DEG = 180.0f / static_cast<float>(M_PI);

    for (int z = 0; z < count; ++z) {
        const IESData &data = dataList[z];
        float maxVal = data.maxCandela > 0 ? data.maxCandela : 1.0f;

        int xOff = z * paddedW + padding;
        int yOff = padding;

        auto writeTexel = [&](int tx, int ty, float intensity) {
            int idx = ty * totalW + tx;
            ptr[idx] = qfloat16(qBound(0.0f, intensity / maxVal, 1.0f));
        };

        for (int py = 0; py < sliceH; ++py) {
            float v = (py + 0.5f) / sliceH;
            for (int px = 0; px < sliceW; ++px) {
                float u = (px + 0.5f) / sliceW;

                float dx, dy, dz;
                octToDir(u, v, dx, dy, dz);

                float theta = qAcos(qBound(-1.0f, dz, 1.0f)) * RAD2DEG;
                float phi = qAtan2(dy, dx) * RAD2DEG;
                if (phi < 0.0f) phi += 360.0f;

                writeTexel(xOff + px, yOff + py,
                           getIntensity(data, theta, phi));
            }
        }

        for (int py = 0; py < sliceH; ++py) {
            ptr[(yOff + py) * totalW + (xOff - 1)]        = ptr[(yOff + py) * totalW + (xOff + sliceW - 1)];
            ptr[(yOff + py) * totalW + (xOff + sliceW)]   = ptr[(yOff + py) * totalW + xOff];
        }

        int rowLen = paddedW;
        for (int px = -1; px < sliceW + 1; ++px) {
            ptr[(yOff - 1)       * totalW + (xOff + px)] = ptr[(yOff + sliceH - 1) * totalW + (xOff + px)];
            ptr[(yOff + sliceH)  * totalW + (xOff + px)] = ptr[yOff               * totalW + (xOff + px)];
        }
        (void)rowLen;
    }

    setTextureData(texData);
    setSize(QSize(totalW, totalH));
}

float IESTextureData::getIntensity(const IESData &data, float theta, float phi) const
{
    theta = qBound(data.verticalAngles.first(), theta, data.verticalAngles.last());

    while (phi < 0)    phi += 360;
    while (phi >= 360) phi -= 360;
    if (phi < 90)       phi = 180 - phi;
    else if (phi > 270) phi = 540 - phi;
    phi = qBound(data.horizontalAngles.first(), phi, data.horizontalAngles.last());

    auto lerp = [&](const QVector<float> &arr, float val) -> QPair<int, float> {
        for (int i = 0; i < arr.size() - 1; ++i)
            if (val <= arr[i + 1])
                return qMakePair(i, (val - arr[i]) / (arr[i + 1] - arr[i]));
        return qMakePair(int(arr.size() - 1), 0.0f);
    };

    auto [v0, vt] = lerp(data.verticalAngles,   theta);
    auto [h0, ht] = lerp(data.horizontalAngles, phi);
    int v1 = qMin(v0 + 1, data.verticalAngles.size()   - 1);
    int h1 = qMin(h0 + 1, data.horizontalAngles.size() - 1);

    float i00 = data.candelaValues[h0][v0] * data.candelaMultiplier;
    float i01 = data.candelaValues[h0][v1] * data.candelaMultiplier;
    float i10 = data.candelaValues[h1][v0] * data.candelaMultiplier;
    float i11 = data.candelaValues[h1][v1] * data.candelaMultiplier;

    return (i00 * (1 - vt) + i01 * vt) * (1 - ht) + (i10 * (1 - vt) + i11 * vt) * ht;
}
