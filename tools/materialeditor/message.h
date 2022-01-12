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

#ifndef MESSAGE_H
#define MESSAGE_H

#include <QtCore/qglobal.h>
#include <QtCore/qlist.h>

#include <QtCore/qbytearray.h>

QT_BEGIN_NAMESPACE

class QIODevice;

using MessageId = QByteArrayView;

struct Message
{
    enum class Type { Invalid, Uniforms, Filenames };

    using MessagePtr = std::shared_ptr<Message>;

    virtual ~Message();
    // NOTE! These functions assume the device is open and read/writeable when called!
    virtual bool read(QDataStream &device) = 0;
    virtual bool write(QDataStream &device) const = 0;
    virtual Type type() const = 0;

    static Message::MessagePtr getMessage(QIODevice &device);
    static bool postMessage(QIODevice &device, const Message &message);

    friend QDataStream &operator>>(QDataStream &stream, Message &message) { message.read(stream); return stream; }
    friend QDataStream &operator<<(QDataStream &stream, const Message &message) { message.write(stream); return stream; }
};

struct UniformsMessage : Message
{
    static constexpr MessageId id { "UM" };

    using Uniform = std::pair<QByteArray /* type */, QByteArray /* name */>;
    using UniformList = QList<Uniform>;
    UniformList uniforms;

    UniformsMessage() = default;
    UniformsMessage(UniformList list) : uniforms(list) {}
    ~UniformsMessage() override;

    bool read(QDataStream &stream) final;
    bool write(QDataStream &stream) const final;
    Type type() const final { return Type::Uniforms; }
};

struct FilenamesMessage : Message
{
    static constexpr MessageId id { "FM" };

    ~FilenamesMessage() override;
    bool read(QDataStream &stream) final;
    bool write(QDataStream &stream) const final;
    Type type() const final { return Type::Filenames; }
    QByteArray vertFilename;
    QByteArray fragFilename;
};

QT_END_NAMESPACE

#endif // MESSAGE_H
