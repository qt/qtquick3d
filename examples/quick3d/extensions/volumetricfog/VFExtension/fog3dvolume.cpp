// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "fog3dvolume.h"

Fog3DVolume::Fog3DVolume(QQuick3DNode *parent)
    : QQuick3DNode(parent)
{}

Fog3DVolume::FogType Fog3DVolume::type() const { return m_type; }
QVector3D Fog3DVolume::extents() const { return m_extents; }
QColor Fog3DVolume::color() const { return m_color; }
float Fog3DVolume::density() const { return m_density; }
bool Fog3DVolume::heightEnabled() const { return m_heightEnabled; }
float Fog3DVolume::leastIntenseY() const { return m_leastIntenseY; }
float Fog3DVolume::mostIntenseY() const { return m_mostIntenseY; }
float Fog3DVolume::heightCurve() const { return m_heightCurve; }
QVector3D Fog3DVolume::noiseOffset() const { return m_noiseOffset; }
float Fog3DVolume::noiseScale() const { return m_noiseScale; }

void Fog3DVolume::setType(FogType v)
{
    if (m_type == v) return;
    m_type = v;
    emit typeChanged();
}

void Fog3DVolume::setExtents(const QVector3D &v)
{
    if (m_extents == v) return;
    m_extents = v;
    emit extentsChanged();
}

void Fog3DVolume::setColor(const QColor &v)
{
    if (m_color == v) return;
    m_color = v;
    emit colorChanged();
}

void Fog3DVolume::setDensity(float v)
{
    if (qFuzzyCompare(m_density, v)) return;
    m_density = v;
    emit densityChanged();
}

void Fog3DVolume::setHeightEnabled(bool v)
{
    if (m_heightEnabled == v) return;
    m_heightEnabled = v;
    emit heightEnabledChanged();
}

void Fog3DVolume::setLeastIntenseY(float v)
{
    if (qFuzzyCompare(m_leastIntenseY, v)) return;
    m_leastIntenseY = v;
    emit leastIntenseYChanged();
}

void Fog3DVolume::setMostIntenseY(float v)
{
    if (qFuzzyCompare(m_mostIntenseY, v)) return;
    m_mostIntenseY = v;
    emit mostIntenseYChanged();
}

void Fog3DVolume::setHeightCurve(float v)
{
    if (qFuzzyCompare(m_heightCurve, v)) return;
    m_heightCurve = v;
    emit heightCurveChanged();
}

void Fog3DVolume::setNoiseOffset(const QVector3D &v)
{
    if (m_noiseOffset == v) return;
    m_noiseOffset = v;
    emit noiseOffsetChanged();
}

void Fog3DVolume::setNoiseScale(float v)
{
    if (qFuzzyCompare(m_noiseScale, v)) return;
    m_noiseScale = v;
    emit noiseScaleChanged();
}
