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
#include <QtQuick3D/private/qquick3dcustommaterial_p.h>
#include <QtQuick3D/private/qquick3ddefaultmaterial_p.h>
#include <QtQuick3D/private/qquick3deffect_p.h>
#include <QtQuick3D/private/qquick3dtexture_p.h>
#include <QtQuick3D/private/qquick3dlight_p.h>
#include <QtQuick3D/private/qquick3dmaterial_p.h>
#include <QtQuick3D/private/qquick3dmodel_p.h>
#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dobject_p.h>
#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3D/private/qquick3dsceneenvironment_p.h>
#include <QtQuick3D/private/qquick3dpickresult_p.h>

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

        qmlRegisterType<QQuick3DCamera>(uri, 1, 0, "Camera");
        qmlRegisterType<QQuick3DCustomMaterial>(uri, 1, 0, "CustomMaterial");
        qmlRegisterType<QQuick3DCustomMaterialShader>(uri, 1, 0, "CustomMaterialShader");
        qmlRegisterType<QQuick3DCustomMaterialShaderInfo>(uri, 1, 0, "CustomMaterialShaderInfo");
        qmlRegisterType<QQuick3DCustomMaterialTexture>(uri, 1, 0, "CustomMaterialTexture");
        qmlRegisterType<QQuick3DCustomMaterialRenderPass>(uri, 1, 0, "CustomMaterialPass");
        qmlRegisterType<QQuick3DCustomMaterialRenderCommand>(uri, 1, 0, "CustomMaterialCommand");
        qmlRegisterType<QQuick3DCustomMaterialBufferInput>(uri, 1, 0, "CustomMaterialBufferInput");
        qmlRegisterType<QQuick3DCustomMaterialBufferBlit>(uri, 1, 0, "CustomMaterialBufferBlit");
        qmlRegisterType<QQuick3DCustomMaterialBlending>(uri, 1, 0, "CustomMaterialBlending");
        qmlRegisterType<QQuick3DCustomMaterialBuffer>(uri, 1, 0, "CustomMaterialBuffer");
        qmlRegisterType<QQuick3DCustomMaterialRenderState>(uri, 1, 0, "CustomMaterialRenderState");
        qmlRegisterType<QQuick3DDefaultMaterial>(uri, 1, 0, "DefaultMaterial");
        qmlRegisterType<QQuick3DEffect>(uri, 1, 0, "Effect");
        qmlRegisterType<QQuick3DTexture>(uri, 1, 0, "Texture");
        qmlRegisterType<QQuick3DLight>(uri, 1, 0, "Light");
        qmlRegisterUncreatableType<QQuick3DMaterial>(uri, 1, 0, "Material", QLatin1String("Material is Abstract"));
        qmlRegisterType<QQuick3DModel>(uri, 1, 0, "Model");
        qmlRegisterType<QQuick3DNode>(uri, 1, 0, "Node");
        qmlRegisterUncreatableType<QQuick3DObject>(uri, 1, 0, "Object3D", QLatin1String("Object3D is Abtract"));
        qmlRegisterType<QQuick3DViewport>(uri, 1, 0, "View3D");
        qmlRegisterType<QQuick3DSceneEnvironment>(uri, 1, 0, "SceneEnvironment");
        qRegisterMetaType<QQuick3DPickResult>();

        qmlRegisterModule(uri, 1, QT_VERSION_MINOR);
    }
};

QT_END_NAMESPACE

#include "plugin.moc"
