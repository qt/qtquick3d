#include "message.h"

/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtNetwork/qlocalsocket.h>

QT_BEGIN_NAMESPACE


Message::Type Message::type() const { return Message::Type::Invalid; }

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
