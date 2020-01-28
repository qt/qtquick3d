/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlengine.h>

#include "gridgeometry_p.h"
#include "pointerplane.h"

QT_BEGIN_NAMESPACE

class QtQuick3DHelpersPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtQuick3DHelpersPlugin(QObject *parent = 0) : QQmlExtensionPlugin(parent) { }
    virtual void registerTypes(const char *uri)
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("QtQuick3D.Helpers"));

        qmlRegisterType<PointerPlane>(uri, 1, 14, "PointerPlane");
        qmlRegisterType<GridGeometry>(uri, 1, 14, "GridGeometry");

        // Auto-increment the import to stay in sync with ALL future QtQuick minor versions from 5.12 onward
        qmlRegisterModule(uri, 1, QT_VERSION_MINOR);
    }
};

QT_END_NAMESPACE

#include "plugin.moc"
