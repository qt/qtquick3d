// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef RESOURCESERVER_H
#define RESOURCESERVER_H

#include "message.h"

#include <QtCore/qstring.h>
#include <QtCore/qpointer.h>

#include <QtNetwork/qlocalserver.h>
#include <QtNetwork/qlocalsocket.h>

QT_BEGIN_NAMESPACE

class ResourceServer : public QObject
{
    Q_OBJECT
public:
    ResourceServer(const QString &serverName);
    bool init();

Q_SIGNALS:
    void messageReceived(Message::MessagePtr message);

public Q_SLOTS:
    void sendMessage(const Message::MessagePtr &message);

private:
    QString m_serverName;
    QLocalServer m_server;
    QPointer<QLocalSocket> m_connection;
};

QT_END_NAMESPACE

#endif // RESOURCESERVER_H
