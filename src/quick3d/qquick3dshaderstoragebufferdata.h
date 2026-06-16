// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QQUICK3DSHADERSTORAGEBUFFERDATA_H
#define QQUICK3DSHADERSTORAGEBUFFERDATA_H

#include <QtCore/qobject.h>
#include <QtQml/qqmlregistration.h>
#include <QtQuick3D/qtquick3dglobal.h>

QT_BEGIN_NAMESPACE

class QQuick3DShaderStorageBufferDataPrivate;
class Q_QUICK3D_EXPORT QQuick3DShaderStorageBufferData : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuick3DShaderStorageBufferData)
    QML_NAMED_ELEMENT(ShaderStorageBufferData)
    QML_ADDED_IN_VERSION(6, 13)
    QML_UNCREATABLE("ShaderStorageBufferData is abstract")
public:
    explicit QQuick3DShaderStorageBufferData(QObject *parent = nullptr);
    ~QQuick3DShaderStorageBufferData() override;

    QByteArray bufferData() const;
    void setBufferData(const QByteArray &bufferData);

Q_SIGNALS:
    void bufferDataDirty();

protected:
    explicit QQuick3DShaderStorageBufferData(QQuick3DShaderStorageBufferDataPrivate &dd, QObject *parent = nullptr);
};

QT_END_NAMESPACE

#endif
