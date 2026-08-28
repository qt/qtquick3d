// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "lutbufferdata.h"

#include <QVector4D>
#include <cstring>

LutBufferData::LutBufferData(QObject *parent)
    : QQuick3DShaderStorageBufferData(parent)
{
    // std430 vec4 array: 16 bytes per entry, no extra padding needed.
    static const QVector4D colors[4] = {
        QVector4D(1.0f, 0.0f, 0.0f, 1.0f), // red
        QVector4D(0.0f, 1.0f, 0.0f, 1.0f), // green
        QVector4D(0.0f, 0.0f, 1.0f, 1.0f), // blue
        QVector4D(1.0f, 1.0f, 0.0f, 1.0f), // yellow
    };

    QByteArray bytes(sizeof(colors), Qt::Uninitialized);
    std::memcpy(bytes.data(), colors, sizeof(colors));
    setBufferData(bytes);
}
