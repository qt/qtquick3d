// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

// 3D noise volume for the volumetric cloud raymarch, generated at startup.
// Uses Schneider's recipe from "Real-time Volumetric Cloudscapes of Horizon
// Zero Dawn" (GPU Pro 7), as adopted by Wicked Engine, Frostbite, Decima:
//
//   R channel: Perlin-Worley — Perlin FBM remapped over an inverted Worley
//              FBM, giving high-contrast cumulus base shape.
//   G/B/A   : Multi-octave inverted Worley FBM at increasing frequencies,
//              providing fluffy cauliflower detail at three scales.
//
// All channels are tileable in X, Y, Z.

#include "cloudnoisetexturedata.h"

#include <QtCore/QByteArray>
#include <QtCore/QSize>
#include <QtCore/qfloat16.h>
#include <QtCore/qmath.h>
#include <QtConcurrent/QtConcurrent>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>

namespace {

constexpr int kSize = 64;

inline uint32_t hashCell(int x, int y, int z, uint32_t salt = 0)
{
    uint32_t h = uint32_t(x) * 73856093u ^ uint32_t(y) * 19349663u ^ uint32_t(z) * 83492791u ^ salt;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return h;
}

inline float hashCellFloat(int x, int y, int z, uint32_t salt = 0)
{
    return (hashCell(x, y, z, salt) & 0xFFFFFFu) / float(0xFFFFFFu);
}

inline int wrap(int v, int period)
{
    int r = v % period;
    return r < 0 ? r + period : r;
}

// Tileable 3D value noise (used as the building block of Perlin-style FBM).
// `period` is the grid resolution that the noise wraps over the [0,1] domain.
float valueNoise3T(float u, float v, float w, int period)
{
    const float x = u * period;
    const float y = v * period;
    const float z = w * period;

    const int x0 = wrap(int(std::floor(x)), period);
    const int y0 = wrap(int(std::floor(y)), period);
    const int z0 = wrap(int(std::floor(z)), period);
    const int x1 = wrap(x0 + 1, period);
    const int y1 = wrap(y0 + 1, period);
    const int z1 = wrap(z0 + 1, period);

    const float fx = x - std::floor(x);
    const float fy = y - std::floor(y);
    const float fz = z - std::floor(z);

    const float sx = fx * fx * (3.0f - 2.0f * fx);
    const float sy = fy * fy * (3.0f - 2.0f * fy);
    const float sz = fz * fz * (3.0f - 2.0f * fz);

    const float n000 = hashCellFloat(x0, y0, z0);
    const float n100 = hashCellFloat(x1, y0, z0);
    const float n010 = hashCellFloat(x0, y1, z0);
    const float n110 = hashCellFloat(x1, y1, z0);
    const float n001 = hashCellFloat(x0, y0, z1);
    const float n101 = hashCellFloat(x1, y0, z1);
    const float n011 = hashCellFloat(x0, y1, z1);
    const float n111 = hashCellFloat(x1, y1, z1);

    const auto lerp1 = [](float a, float b, float t) { return a + (b - a) * t; };
    return lerp1(lerp1(lerp1(n000, n100, sx), lerp1(n010, n110, sx), sy), lerp1(lerp1(n001, n101, sx), lerp1(n011, n111, sx), sy), sz);
}

float perlinFbm3T(float u, float v, float w, int basePeriod, int octaves)
{
    float value = 0.0f;
    float amp = 0.5f;
    float total = 0.0f;
    int period = basePeriod;
    for (int i = 0; i < octaves; ++i) {
        value += amp * valueNoise3T(u, v, w, period);
        total += amp;
        period *= 2;
        amp *= 0.5f;
    }
    return value / total;
}

// Tileable 3D Worley (F1) — returns distance to nearest feature point,
// scaled into ~[0,1].
float worley3T(float u, float v, float w, int gridSize, uint32_t salt = 0)
{
    const float x = u * gridSize;
    const float y = v * gridSize;
    const float z = w * gridSize;
    const int cx = int(std::floor(x));
    const int cy = int(std::floor(y));
    const int cz = int(std::floor(z));
    const float fx = x - cx;
    const float fy = y - cy;
    const float fz = z - cz;

    float minDistSq = 3.0f;
    for (int dz = -1; dz <= 1; ++dz) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                const int nx = wrap(cx + dx, gridSize);
                const int ny = wrap(cy + dy, gridSize);
                const int nz = wrap(cz + dz, gridSize);
                const float fpx = hashCellFloat(nx, ny, nz, salt + 0xA1u);
                const float fpy = hashCellFloat(nx, ny, nz, salt + 0xB2u);
                const float fpz = hashCellFloat(nx, ny, nz, salt + 0xC3u);
                const float diffx = float(dx) + fpx - fx;
                const float diffy = float(dy) + fpy - fy;
                const float diffz = float(dz) + fpz - fz;
                const float distSq = diffx * diffx + diffy * diffy + diffz * diffz;
                if (distSq < minDistSq)
                    minDistSq = distSq;
            }
        }
    }
    return std::min(std::sqrt(minDistSq) / 0.7f, 1.0f);
}

// Three-octave inverted Worley FBM. Inverted so high values are at cell
// centers (the "cloudy" interior), low values between cells.
// Octaves at base, 2× base, 4× base with weights 0.625/0.25/0.125 (sum=1).
// gridBase × 4 must be ≤ kSize to avoid aliasing.
float invWorleyFbm(float u, float v, float w, int gridBase, uint32_t salt)
{
    const float w1 = 1.0f - worley3T(u, v, w, gridBase, salt + 0x010u);
    const float w2 = 1.0f - worley3T(u, v, w, gridBase * 2, salt + 0x020u);
    const float w3 = 1.0f - worley3T(u, v, w, gridBase * 4, salt + 0x030u);
    return w1 * 0.625f + w2 * 0.25f + w3 * 0.125f;
}

inline float remap(float v, float oldMin, float oldMax, float newMin, float newMax)
{
    return newMin + (v - oldMin) / (oldMax - oldMin) * (newMax - newMin);
}

} // namespace

CloudNoiseTextureData::CloudNoiseTextureData(QQuick3DObject *parent) : QQuick3DTextureData(parent)
{
    setSize(QSize(kSize, kSize));
    setDepth(kSize);
    setFormat(QQuick3DTextureData::RGBA16F);

    QByteArray data;
    data.resize(qsizetype(kSize) * kSize * kSize * 4 * qsizetype(sizeof(qfloat16)));
    qfloat16 *out = reinterpret_cast<qfloat16 *>(data.data());

    // Worley grid bases for each channel. The 4× highest octave must stay
    // within kSize (64) to avoid Worley aliasing — so base ≤ 16.
    constexpr int gridR = 6; // base Worley used inside Perlin-Worley remap
    constexpr int gridG = 6; // mid cumulus puff frequency
    constexpr int gridB = 10; // higher detail
    constexpr int gridA = 16; // finest detail (16 × 4 = 64 = kSize limit)

    // Generate the volume one z-slice per thread. Slices write into disjoint
    // regions of the output buffer, so no synchronization is needed.
    std::vector<int> sliceIndices(kSize);
    std::iota(sliceIndices.begin(), sliceIndices.end(), 0);

    QtConcurrent::blockingMap(sliceIndices, [&](int z) {
        const float w = float(z) / float(kSize);
        qfloat16 *sliceOut = out + qsizetype(z) * kSize * kSize * 4;
        for (int y = 0; y < kSize; ++y) {
            const float v = float(y) / float(kSize);
            for (int x = 0; x < kSize; ++x) {
                const float u = float(x) / float(kSize);

                // R: Schneider's Perlin-Worley. Perlin FBM is "folded" via
                // abs(p * 2 - 1) so both Perlin extremes map to high values,
                // then remapped over an inverted Worley FBM. The result has
                // smooth Perlin-style variation modulated by Worley cells —
                // the canonical "cloud base shape" used by AAA renderers.
                const float perlin = perlinFbm3T(u, v, w, 4, 4);
                const float pfbm = std::abs(perlin * 2.0f - 1.0f);
                const float wfbm = invWorleyFbm(u, v, w, gridR, 0x100u);
                float r = remap(pfbm, 0.0f, 1.0f, wfbm, 1.0f);

                // G/B/A: pure Worley FBM at three increasing frequencies for
                // cauliflower detail at three scales of carving.
                float g = invWorleyFbm(u, v, w, gridG, 0x200u);
                float b = invWorleyFbm(u, v, w, gridB, 0x300u);
                float a = invWorleyFbm(u, v, w, gridA, 0x400u);

                r = std::clamp(r, 0.0f, 1.0f);
                g = std::clamp(g, 0.0f, 1.0f);
                b = std::clamp(b, 0.0f, 1.0f);
                a = std::clamp(a, 0.0f, 1.0f);

                sliceOut[0] = qfloat16(r);
                sliceOut[1] = qfloat16(g);
                sliceOut[2] = qfloat16(b);
                sliceOut[3] = qfloat16(a);
                sliceOut += 4;
            }
        }
    });

    setTextureData(data);
}
