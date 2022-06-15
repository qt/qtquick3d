// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DEFFECT_H
#define QQUICK3DEFFECT_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick3D/qtquick3dglobal.h>
#include <QtQuick3D/private/qquick3dobject_p.h>
#include <QtQuick3D/private/qquick3dtexture_p.h>

#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendercommands_p.h>

#include <QtCore/qvector.h>

#include <QtQuick3D/private/qquick3dshaderutils_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3D_EXPORT QQuick3DEffect : public QQuick3DObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QQuick3DShaderUtilsRenderPass> passes READ passes)

    QML_NAMED_ELEMENT(Effect)
public:
    explicit QQuick3DEffect(QQuick3DObject *parent = nullptr);

    QQmlListProperty<QQuick3DShaderUtilsRenderPass> passes();

    // Passes
    static void qmlAppendPass(QQmlListProperty<QQuick3DShaderUtilsRenderPass> *list,
                              QQuick3DShaderUtilsRenderPass *pass);
    static QQuick3DShaderUtilsRenderPass *qmlPassAt(QQmlListProperty<QQuick3DShaderUtilsRenderPass> *list,
                                                    qsizetype index);
    static qsizetype qmlPassCount(QQmlListProperty<QQuick3DShaderUtilsRenderPass> *list);
    static void qmlPassClear(QQmlListProperty<QQuick3DShaderUtilsRenderPass> *list);

    void setDynamicTextureMap(QQuick3DTexture *textureMap, const QByteArray &name);

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void itemChange(QQuick3DObject::ItemChange , const QQuick3DObject::ItemChangeData &) override;

private Q_SLOTS:
    void onPropertyDirty();
    void onTextureDirty();
private:
    enum Dirty {
        TextureDirty = 0x1,
        PropertyDirty = 0x2
    };

    void markDirty(QQuick3DEffect::Dirty type);

    quint32 m_dirtyAttributes = 0xffffffff;

    void updateSceneManager(QQuick3DSceneManager *sceneManager);

    friend class QQuick3DSceneRenderer;
    QVector<QQuick3DShaderUtilsRenderPass *> m_passes;
    QVector<QQuick3DTexture *> m_dynamicTextureMaps;
    QHash<QByteArray, QMetaObject::Connection> m_connections;
};

QT_END_NAMESPACE

#endif // QQUICK3DEFFECT_H
