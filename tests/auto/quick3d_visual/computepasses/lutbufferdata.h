// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef LUTBUFFERDATA_H
#define LUTBUFFERDATA_H

#include <QtQuick3D/qquick3dshaderstoragebufferdata.h>
#include <QtQmlIntegration>

// A fixed 4-entry RGBA colour table, uploaded to the GPU once via
// QQuick3DShaderStorageBuffer::bufferData. Exercises the CPU-authored (as opposed to
// GPU-computed) ShaderStorageBuffer content path.
class LutBufferData : public QQuick3DShaderStorageBufferData
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit LutBufferData(QObject *parent = nullptr);
};

#endif // LUTBUFFERDATA_H
