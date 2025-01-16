// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "resourceserver.h"

#include <QtCore/qcommandlineoption.h>
#include <QtCore/qcommandlineparser.h>
#include <QtCore/qdebug.h>

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

    QObject::connect(&m_server, &QLocalServer::newConnection, this, [this]() {
        qDebug() << "srv: Incoming connection!";
        if (m_connection != nullptr && m_connection->isOpen()) {
            qDebug("Client already connected! Connection refused!");
            m_server.close();
        } else {
            if (m_connection)
                m_connection->close();
            m_connection = m_server.nextPendingConnection();
            QObject::connect(m_connection, &QLocalSocket::readyRead, this, [this]() {
                const auto message = Message::getMessage(*m_connection);
                if (message && message->type() != Message::Type::Invalid)
                    Q_EMIT messageReceived(message);
            });
            QObject::connect(m_connection, &QLocalSocket::errorOccurred, &m_server, [this]() {
                qDebug("srv: Error occurred\n - %s", qPrintable(m_connection->errorString()));
                m_server.close();
                m_server.listen(m_serverName);
            });
            QObject::connect(m_connection, &QLocalSocket::connected, this, []() {
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
