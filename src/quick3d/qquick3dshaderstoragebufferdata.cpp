// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtQuick3D/private/qquick3dshaderstoragebufferdata_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ShaderStorageBufferData
    \inqmlmodule QtQuick3D
    \nativetype QQuick3DShaderStorageBufferData
    \brief Base type for CPU-authored shader storage buffer content.
    \since 6.13

    ShaderStorageBufferData is the base type for providing application-generated
    (CPU-side) content for a \l ShaderStorageBuffer, analogous to how
    \l TextureData provides application-generated content for a \l Texture.
    It cannot be instantiated directly from QML; instead, subclass
    QQuick3DShaderStorageBufferData in C++, set the data with \l{QQuick3DShaderStorageBufferData::}{setBufferData()}
    and register the subclass to QML. Once registered, it can be assigned
    to a \l{ShaderStorageBuffer::bufferData}{ShaderStorageBuffer's bufferData} property.

    The \c QQuick3DShaderStorageBufferData::bufferDataDirty() signal is emitted whenever the data
    changes, so the ShaderStorageBuffer using it can pick up the new content
    (and, if its size changed, propagate that too).

    \sa ShaderStorageBuffer, TextureData
*/

/*!
    \class QQuick3DShaderStorageBufferData
    \inmodule QtQuick3D
    \since 6.13
    \brief Base class for defining custom shader storage buffer content.

    The QQuick3DShaderStorageBufferData class is used to provide application-generated
    (CPU-side) content for a \l ShaderStorageBuffer in a Qt Quick 3D scene,
    analogous to how \l QQuick3DTextureData provides application-generated
    content for a \l {QtQuick3D::Texture}{Texture}.

    Instances are created by inheriting from this class and registering the
    subclass to the QML type system. The \l{ShaderStorageBuffer::bufferData}{bufferData}
    property of a \l ShaderStorageBuffer can then be set to an instance of the registered
    type. Assigning a QQuick3DShaderStorageBufferData also makes the buffer's
    \l{ShaderStorageBuffer::size}{size} follow the size of the buffer data.

    Example implementation:

    \code
    class CustomBufferData : public QQuick3DShaderStorageBufferData
    {
        ... object and QML registration macros ...
    public:
        void setValues(const QList<float> &values)
        {
            QByteArray bytes(values.size() * int(sizeof(float)), Qt::Uninitialized);
            memcpy(bytes.data(), values.constData(), bytes.size());
            setBufferData(bytes);
        }
    };
    \endcode

    The layout of the buffer bytes has to match how the buffer is declared
    in the shader's \c STORAGE_BUFFER block; the shader's block is what
    determines the interpretation, this class only supplies the raw bytes.

    \sa QQuick3DTextureData, ShaderStorageBuffer, ShaderStorageBufferData
*/

QQuick3DShaderStorageBufferData::QQuick3DShaderStorageBufferData(QObject *parent)
    : QQuick3DShaderStorageBufferData(*new QQuick3DShaderStorageBufferDataPrivate, parent)
{
}

/*!
    \internal
*/
QQuick3DShaderStorageBufferData::QQuick3DShaderStorageBufferData(QQuick3DShaderStorageBufferDataPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
}

QQuick3DShaderStorageBufferData::~QQuick3DShaderStorageBufferData() = default;

/*!
    Returns the current buffer data.
*/
QByteArray QQuick3DShaderStorageBufferData::bufferData() const
{
    Q_D(const QQuick3DShaderStorageBufferData);
    return d->bufferData;
}

/*!
    Sets the buffer data to \a bufferData. The layout of the buffer bytes has to match
    how the buffer is declared in the shader's \c STORAGE_BUFFER block;
    the shader's block is what determines the interpretation, this
    class only supplies the raw bytes.
*/
void QQuick3DShaderStorageBufferData::setBufferData(const QByteArray &bufferData)
{
    Q_D(QQuick3DShaderStorageBufferData);
    d->bufferData = bufferData;
    emit bufferDataDirty();
}

QT_END_NAMESPACE
