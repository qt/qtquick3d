// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef RESIZABLEBUFFERDATA_H
#define RESIZABLEBUFFERDATA_H

#include <QtQuick3D/qquick3dshaderstoragebufferdata.h>
#include <QtQmlIntegration>

// Uploads count vec4 elements, so the buffer can be resized from QML.
class ResizableBufferData : public QQuick3DShaderStorageBufferData
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)

public:
    explicit ResizableBufferData(QObject *parent = nullptr);

    int count() const { return m_count; }
    void setCount(int count);

Q_SIGNALS:
    void countChanged();

private:
    int m_count = 0;
};

#endif // RESIZABLEBUFFERDATA_H
