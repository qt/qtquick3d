// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "ieslightprofileindex.h"

IESLightProfileIndex::IESLightProfileIndex(QObject *parent)
    : QObject(parent)
{
}

QObject *IESLightProfileIndex::light() const { return m_light; }
int IESLightProfileIndex::index() const { return m_index; }
float IESLightProfileIndex::intensity() const { return m_intensity; }

void IESLightProfileIndex::setLight(QObject *light)
{
    if (m_light == light)
        return;
    m_light = light;
    emit lightChanged();
}

void IESLightProfileIndex::setIndex(int index)
{
    if (m_index == index)
        return;
    m_index = index;
    emit indexChanged();
}

void IESLightProfileIndex::setIntensity(float intensity)
{
    if (qFuzzyCompare(m_intensity, intensity))
        return;
    m_intensity = intensity;
    emit intensityChanged();
}
