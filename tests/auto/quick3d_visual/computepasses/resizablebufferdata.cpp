// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "resizablebufferdata.h"

#include <QVector4D>
#include <cstring>

ResizableBufferData::ResizableBufferData(QObject *parent)
    : QQuick3DShaderStorageBufferData(parent)
{
}

void ResizableBufferData::setCount(int count)
{
    if (m_count == count)
        return;
    m_count = count;
    const QVector4D value(0.25f, 0.5f, 0.75f, 1.0f);
    QByteArray bytes(count * qsizetype(sizeof(QVector4D)), Qt::Uninitialized);
    for (int i = 0; i < count; ++i)
        std::memcpy(bytes.data() + i * sizeof(QVector4D), &value, sizeof(QVector4D));
    setBufferData(bytes);
    emit countChanged();
}
