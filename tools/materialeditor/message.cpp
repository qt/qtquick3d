// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "message.h"
#include <QtNetwork/qlocalsocket.h>

QT_BEGIN_NAMESPACE


Message::Type Message::type() const { return Message::Type::Invalid; }

UniformsMessage::~UniformsMessage()
{

}

bool UniformsMessage::read(QDataStream &stream)
{
    stream >> uniforms;
    return (stream.status() == QDataStream::Ok);
}

bool UniformsMessage::write(QDataStream &stream) const
{
    if (uniforms.size() > 0)
        stream << id.toByteArray() << uniforms;

    return (stream.status() == QDataStream::Ok);
}

FilenamesMessage::~FilenamesMessage()
{

}

bool FilenamesMessage::read(QDataStream &stream)
{
    stream >> vertFilename >> fragFilename;
    return (stream.status() == QDataStream::Ok);
}

bool FilenamesMessage::write(QDataStream &stream) const
{
    stream << vertFilename << fragFilename;
    return (stream.status() == QDataStream::Ok);
}

Message::~Message()
{

}

Message::MessagePtr Message::getMessage(QIODevice &device)
{
    if (device.isOpen()) {
        if (device.isReadable()) {
            QByteArray id;
            QDataStream stream(&device);
            stream >> id;
            if (UniformsMessage::id == id) {
                UniformsMessage uniforms;
                if (uniforms.read(stream))
                    return std::make_shared<UniformsMessage>(uniforms);
            } else if (id == FilenamesMessage::id) {
                FilenamesMessage filenames;
                if (filenames.read(stream))
                    return std::make_shared<FilenamesMessage>(filenames);
            } else {
                qDebug("Unknown message!");
            }
        } else {
            qDebug("Device not writeable!");
        }
    } else {
        qDebug("Device not open!");
    }

    return Message::MessagePtr();
}

bool Message::postMessage(QIODevice &device, const Message &message)
{
    bool ok = false;
    if (device.isOpen()) {
        if (device.isWritable()) {
            QDataStream stream(&device);
            stream << message;
            ok = (stream.status() == QDataStream::Ok);
        } else {
            qDebug("Device not writeable!");
        }
    } else {
        qDebug("Device not open!");
    }

    return ok;
}

QT_END_NAMESPACE
