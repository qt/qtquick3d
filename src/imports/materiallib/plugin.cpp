/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlengine.h>

#include <QtQuick3D/private/qquick3dcustommaterial_p.h>

QT_BEGIN_NAMESPACE

class QtQuick3DMaterialPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtQuick3DMaterialPlugin(QObject *parent = 0) : QQmlExtensionPlugin(parent) { }
    virtual void registerTypes(const char *uri)
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("QtQuick3D.Materials"));

        qmlRegisterType<QQuick3DCustomMaterial>(uri, 1, 14, "CustomMaterial");

        // Auto-increment the import to stay in sync with ALL future QtQuick minor versions from 5.12 onward
        qmlRegisterModule(uri, 1, QT_VERSION_MINOR);
    }
};

QT_END_NAMESPACE

#include "plugin.moc"
