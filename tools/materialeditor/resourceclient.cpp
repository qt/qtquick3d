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
    QObject::connect(&m_socket, &QLocalSocket::readyRead, [this]() {
        const auto message = Message::getMessage(m_socket);
        if (message->type() != Message::Type::Invalid)
            Q_EMIT messageReceived(message);
    });
    QObject::connect(&m_socket, &QLocalSocket::errorOccurred, [this]() {
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
