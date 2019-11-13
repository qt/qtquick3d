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

#include <QtQuick3D/private/qquick3dcamera_p.h>
#include <QtQuick3D/private/qquick3dperspectivecamera_p.h>
#include <QtQuick3D/private/qquick3dorthographiccamera_p.h>
#include <QtQuick3D/private/qquick3dfrustumcamera_p.h>
#include <QtQuick3D/private/qquick3dcustomcamera_p.h>

#include <QtQuick3D/private/qquick3ddefaultmaterial_p.h>
#include <QtQuick3D/private/qquick3dtexture_p.h>
#include <QtQuick3D/private/qquick3ddirectionallight_p.h>
#include <QtQuick3D/private/qquick3dpointlight_p.h>
#include <QtQuick3D/private/qquick3darealight_p.h>
#include <QtQuick3D/private/qquick3dmaterial_p.h>
#include <QtQuick3D/private/qquick3dmodel_p.h>
#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dobject_p.h>
#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3D/private/qquick3dsceneenvironment_p.h>
#include <QtQuick3D/private/qquick3dpickresult_p.h>
#include <QtQuick3D/private/qquick3drepeater_p.h>
#include <QtQuick3D/private/qquick3dloader_p.h>
#include <QtQuick3D/private/qquick3dprincipledmaterial_p.h>
#include <QtQuick3D/private/qquick3drenderstats_p.h>
#include <QtQuick3D/private/qquick3dgeometry_p.h>

#include <private/qqmlglobal_p.h>

static void initResources()
{
#ifdef QT_STATIC
    Q_INIT_RESOURCE(qmake_QtQuick3D);
#endif
}

QT_BEGIN_NAMESPACE

static QQmlPrivate::AutoParentResult qquick3dobject_autoParent(QObject *obj, QObject *parent)
{
    // When setting a parent (especially during dynamic object creation) in QML,
    // also try to set up the analogous item/window relationship.
    if (QQuick3DObject *parentItem = qmlobject_cast<QQuick3DObject *>(parent)) {
        QQuick3DObject *item = qmlobject_cast<QQuick3DObject *>(obj);
        if (item) {
            // An Item has another Item
            item->setParentItem(parentItem);
            return QQmlPrivate::Parented;
        }
        return QQmlPrivate::IncompatibleObject;
    } else if (qmlobject_cast<QQuick3DObject *>(obj)) {
        return QQmlPrivate::IncompatibleParent;
    }
    return QQmlPrivate::IncompatibleObject;
}

class QQuick3DPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QQuick3DPlugin(QObject *parent = nullptr) : QQmlExtensionPlugin(parent) { initResources(); }
    void registerTypes(const char *uri) override
    {
        QQmlPrivate::RegisterAutoParent autoparent = { 0, &qquick3dobject_autoParent };
        QQmlPrivate::qmlregister(QQmlPrivate::AutoParentRegistration, &autoparent);

        qmlRegisterUncreatableType<QQuick3DCamera>(uri, 1, 0, "Camera", QLatin1String("Camera is Abstract"));
        qmlRegisterType<QQuick3DPerspectiveCamera>(uri, 1, 0, "PerspectiveCamera");
        qmlRegisterType<QQuick3DOrthographicCamera>(uri, 1, 0, "OrthographicCamera");
        qmlRegisterType<QQuick3DFrustumCamera>(uri, 1, 0, "FrustumCamera");
        qmlRegisterType<QQuick3DCustomCamera>(uri, 1, 0, "CustomCamera");
        qmlRegisterType<QQuick3DDefaultMaterial>(uri, 1, 0, "DefaultMaterial");
        qmlRegisterType<QQuick3DPrincipledMaterial>(uri, 1, 0, "PrincipledMaterial");
        qmlRegisterType<QQuick3DTexture>(uri, 1, 0, "Texture");
        qmlRegisterUncreatableType<QQuick3DAbstractLight>(uri, 1, 0, "Light", QLatin1String("Light is Abstract"));
        qmlRegisterType<QQuick3DDirectionalLight>(uri, 1, 0, "DirectionalLight");
        qmlRegisterType<QQuick3DPointLight>(uri, 1, 0, "PointLight");
        qmlRegisterType<QQuick3DAreaLight>(uri, 1, 0, "AreaLight");
        qmlRegisterUncreatableType<QQuick3DMaterial>(uri, 1, 0, "Material", QLatin1String("Material is Abstract"));
        qmlRegisterType<QQuick3DModel>(uri, 1, 0, "Model");
        qmlRegisterType<QQuick3DNode>(uri, 1, 0, "Node");
        qmlRegisterUncreatableType<QQuick3DObject>(uri, 1, 0, "Object3D", QLatin1String("Object3D is Abstract"));
        qmlRegisterType<QQuick3DViewport>(uri, 1, 0, "View3D");
        qmlRegisterType<QQuick3DSceneEnvironment>(uri, 1, 0, "SceneEnvironment");
        qmlRegisterType<QQuick3DRepeater>(uri, 1, 0, "Repeater3D");
        qmlRegisterType<QQuick3DLoader>(uri, 1, 0, "Loader3D");
        qmlRegisterUncreatableType<QQuick3DGeometry>(uri, 1, 0, "Geometry", QLatin1String("Geometry is Abstract"));
        qRegisterMetaType<QQuick3DPickResult>();
        qRegisterMetaType<QQuick3DRenderStats *>();

        qmlRegisterModule(uri, 1, QT_VERSION_MINOR);
    }
};

QT_END_NAMESPACE

#include "plugin.moc"
