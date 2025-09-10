// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef RESOURCECLIENT_H
#define RESOURCECLIENT_H

#include "message.h"
#include <QtNetwork/qlocalsocket.h>

QT_BEGIN_NAMESPACE

class ResourceClient : public QObject
{
    Q_OBJECT
public:
    ResourceClient(const QString &serverName);

    void init();

Q_SIGNALS:
    void messageReceived(Message::MessagePtr message);
    void connected();

public Q_SLOTS:
    void sendMessage(const Message::MessagePtr &message);

private:
    QString m_serverName;
    QLocalSocket m_socket;
};

QT_END_NAMESPACE

#endif // RESOURCECLIENT_H
