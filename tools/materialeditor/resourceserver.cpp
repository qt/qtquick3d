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

#include "resourceserver.h"

#include <QtCore/qcommandlineoption.h>
#include <QtCore/qcommandlineparser.h>

#include <QtNetwork/qlocalsocket.h>

QT_BEGIN_NAMESPACE

// BEGIN - Test Resource IPC
//const auto serverName = QLatin1String("testServer");
//ResourceServer resServer(serverName);
//QObject::connect(&resServer, &ResourceServer::messageReceived, [](const Message::MessagePtr &msg) {
//    if (msg)
//        qDebug() << "Received message: " << int(msg->type());
//    else
//        qDebug() << "Received null message!";

//    if (msg && msg->type() == Message::Type::Uniforms) {
//        const auto &um = static_cast<const UniformsMessage &>(*msg);
//        qDebug() << "UniformMessage: " << um.uniforms;
//    }
//});
//resServer.init();

//ResourceClient resClient(serverName);
//resClient.init();

//resClient.sendMessage(std::make_shared<UniformsMessage>(UniformsMessage::UniformList{UniformsMessage::Uniform{"float", "foo"}}));
// End - Test Resoruce IPC

ResourceServer::ResourceServer(const QString &serverName)
    : m_serverName(serverName)
{

}

bool ResourceServer::init()
{
    if (m_server.isListening())
        return false;

    QObject::connect(&m_server, &QLocalServer::newConnection, [this]() {
        qDebug() << "srv: Incoming connection!";
        if (m_connection != nullptr && m_connection->isOpen()) {
            qDebug("Client already connected! Connection refused!");
            m_server.close();
        } else {
            if (m_connection)
                m_connection->close();
            m_connection = m_server.nextPendingConnection();
            QObject::connect(m_connection, &QLocalSocket::readyRead, [this]() {
                const auto message = Message::getMessage(*m_connection);
                if (message && message->type() != Message::Type::Invalid)
                    Q_EMIT messageReceived(message);
            });
            QObject::connect(m_connection, &QLocalSocket::errorOccurred, [this]() {
                qDebug("srv: Error occurred\n - %s", qPrintable(m_connection->errorString()));
                m_server.close();
                m_server.listen(m_serverName);
            });
            QObject::connect(m_connection, &QLocalSocket::connected, []() {
                qDebug("srv: Connection established!");
            });
        }
    });


    int attempts = 3;
    for (;attempts && !m_server.isListening(); --attempts) {
        if (!m_server.listen(m_serverName))
            QLocalServer::removeServer(m_serverName);
    }

    if (m_server.isListening())
        qDebug("srv: Listening for incoming connection on \'%s\'", qPrintable(m_serverName));

    return true;
}

void ResourceServer::sendMessage(const Message::MessagePtr &message)
{
    if (message) {
        if (m_connection)
            Message::postMessage(*m_connection, *message);
        else
            qDebug("srv: No connection!");
    } else {
        qDebug("srv: Inavlid message!");
    }
}

QT_END_NAMESPACE
