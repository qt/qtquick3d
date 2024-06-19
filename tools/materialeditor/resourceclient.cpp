// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "resourceclient.h"

// Messages:
// Uniforms []
// Filenames (vert, frag)

QT_BEGIN_NAMESPACE

ResourceClient::ResourceClient(const QString &serverName)
    : m_serverName(serverName)
{

}

void ResourceClient::init()
{
    const int timeout = 10000;
    QObject::connect(&m_socket, &QLocalSocket::readyRead, this, [this]() {
        const auto message = Message::getMessage(m_socket);
        if (message->type() != Message::Type::Invalid)
            Q_EMIT messageReceived(message);
    });
    QObject::connect(&m_socket, &QLocalSocket::errorOccurred, this, [this]() {
        qDebug("client: Error occurred when connecting to: \'%s\'\n - %s", qPrintable(m_serverName), qPrintable(m_socket.errorString()));
    });

    m_socket.connectToServer(m_serverName); // ReadWrite
    if (m_socket.waitForConnected(timeout)) {
        qDebug("client: Connected to: %s", qPrintable(m_serverName));
        Q_EMIT connected();
    }
}

void ResourceClient::sendMessage(const Message::MessagePtr &message)
{
    if (message != nullptr)
        Message::postMessage(m_socket, *message);
}

QT_END_NAMESPACE
