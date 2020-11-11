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

#include "qquick3dmaterial_p.h"
#include "qquick3dobject_p.h"
#include "qquick3dscenemanager_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterial_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Material
    \inherits Object3D
    \inqmlmodule QtQuick3D
    \brief Abstract base type providing functionality common to materials.
*/

/*!
    \qmlproperty Texture Material::lightmapIndirect

    This property defines a baked lightmap Texture containing indirect lighting
    information for this material.

    \note This feature is still in development so there is currently no way to
    bake lights. The texture currently still uses the UV1 coordinates which is
    going to change later to UV2.
*/

/*!
    \qmlproperty Texture Material::lightmapRadiosity

    This property defines a baked lightmap Texture containing direct lighting
    information for this material.

    \note This feature is still in development so there is currently no way to
    bake lights. The texture currently still uses the UV1 coordinates which is
    going to change later to UV2.
*/

/*!
    \qmlproperty Texture Material::lightmapShadow

    This property defines a baked lightmap Texture containing shadowing
    information for this material.

    \note This feature is still in development so there is currently no way to
    bake lights. The texture currently still uses the UV1 coordinates which is
    going to change later to UV2.
*/

/*!
    \qmlproperty Texture Material::lightProbe

    This property defines a Texture for overriding or setting an image based
    lighting Texture for use with this material only.

    \note Setting a light probe on the material will override the
    \l {SceneEnvironment::lightProbe} {scene's light probe} for models using this material.

    \sa SceneEnvironment::lightProbe
*/

/*!
    \qmlproperty enumeration Material::cullMode

    This property defines whether primitive culling is enabled, and, when
    enabled, which primitives are discarded.

    The default value is Material.BackFaceCulling.

    A triangle is considered front-facing if it has a counter-clockwise
    winding, meaning its vertices in framebuffer coordinates are in
    counter-clockwise order.

    \value Material.BackFaceCulling Back-facing triangles are discarded.
    \value Material.FrontFaceCulling Front-facing triangles are discarded.
    \value Material.NoCulling No triangles are discarded.
*/


QQuick3DMaterial::QQuick3DMaterial(QQuick3DObjectPrivate &dd, QQuick3DObject *parent)
    : QQuick3DObject(dd, parent) {}

QQuick3DMaterial::~QQuick3DMaterial()
{
    for (const auto &connection : qAsConst(m_connections))
        disconnect(connection);
}

QQuick3DTexture *QQuick3DMaterial::lightmapIndirect() const
{
    return m_lightmapIndirect;
}

QQuick3DTexture *QQuick3DMaterial::lightmapRadiosity() const
{
    return m_lightmapRadiosity;
}

QQuick3DTexture *QQuick3DMaterial::lightmapShadow() const
{
    return m_lightmapShadow;
}

QQuick3DTexture *QQuick3DMaterial::lightProbe() const
{
    return m_iblProbe;
}

QQuick3DMaterial::CullMode QQuick3DMaterial::cullMode() const
{
    return m_cullMode;
}

void QQuick3DMaterial::setLightmapIndirect(QQuick3DTexture *lightmapIndirect)
{
    if (m_lightmapIndirect == lightmapIndirect)
        return;

    QQuick3DObjectPrivate::updatePropertyListener(lightmapIndirect, m_lightmapIndirect, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("lightmapIndirect"), m_connections, [this](QQuick3DObject *n) {
        setLightmapIndirect(qobject_cast<QQuick3DTexture *>(n));
    });

    m_lightmapIndirect = lightmapIndirect;
    emit lightmapIndirectChanged(m_lightmapIndirect);
    update();
}

void QQuick3DMaterial::setLightmapRadiosity(QQuick3DTexture *lightmapRadiosity)
{
    if (m_lightmapRadiosity == lightmapRadiosity)
        return;

    QQuick3DObjectPrivate::updatePropertyListener(lightmapRadiosity, m_lightmapRadiosity, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("lightmapRadiosity"), m_connections, [this](QQuick3DObject *n) {
        setLightmapRadiosity(qobject_cast<QQuick3DTexture *>(n));
    });

    m_lightmapRadiosity = lightmapRadiosity;
    emit lightmapRadiosityChanged(m_lightmapRadiosity);
    update();
}

void QQuick3DMaterial::setLightmapShadow(QQuick3DTexture *lightmapShadow)
{
    if (m_lightmapShadow == lightmapShadow)
        return;

    QQuick3DObjectPrivate::updatePropertyListener(lightmapShadow, m_lightmapShadow, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("lightmapShadow"), m_connections, [this](QQuick3DObject *n) {
        setLightmapShadow(qobject_cast<QQuick3DTexture *>(n));
    });

    m_lightmapShadow = lightmapShadow;
    emit lightmapShadowChanged(m_lightmapShadow);
    update();
}

void QQuick3DMaterial::setLightProbe(QQuick3DTexture *iblProbe)
{
    if (m_iblProbe == iblProbe)
        return;

    QQuick3DObjectPrivate::updatePropertyListener(iblProbe, m_iblProbe, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("lightProbe"), m_connections, [this](QQuick3DObject *n) {
        setLightProbe(qobject_cast<QQuick3DTexture *>(n));
    });

    m_iblProbe = iblProbe;
    emit lightProbeChanged(m_iblProbe);
    update();
}

void QQuick3DMaterial::setCullMode(QQuick3DMaterial::CullMode cullMode)
{
    if (m_cullMode == cullMode)
        return;

    m_cullMode = cullMode;
    emit cullModeChanged(m_cullMode);
    update();
}

QSSGRenderGraphObject *QQuick3DMaterial::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node)
        return nullptr;

    // Set the common properties
    if (node->type == QSSGRenderGraphObject::Type::DefaultMaterial || node->type == QSSGRenderGraphObject::Type::PrincipledMaterial) {
        auto defaultMaterial = static_cast<QSSGRenderDefaultMaterial *>(node);
        if (!m_lightmapIndirect)
            defaultMaterial->lightmaps.m_lightmapIndirect = nullptr;
        else
            defaultMaterial->lightmaps.m_lightmapIndirect = m_lightmapIndirect->getRenderImage();

        if (!m_lightmapRadiosity)
            defaultMaterial->lightmaps.m_lightmapRadiosity = nullptr;
        else
            defaultMaterial->lightmaps.m_lightmapRadiosity = m_lightmapRadiosity->getRenderImage();

        if (!m_lightmapShadow)
            defaultMaterial->lightmaps.m_lightmapShadow = nullptr;
        else
            defaultMaterial->lightmaps.m_lightmapShadow = m_lightmapShadow->getRenderImage();

        if (!m_iblProbe)
            defaultMaterial->iblProbe = nullptr;
        else
            defaultMaterial->iblProbe = m_iblProbe->getRenderImage();

        defaultMaterial->cullMode = QSSGCullFaceMode(m_cullMode);
        node = defaultMaterial;

    } else if (node->type == QSSGRenderGraphObject::Type::CustomMaterial) {
        auto customMaterial = static_cast<QSSGRenderCustomMaterial *>(node);
        if (!m_lightmapIndirect)
            customMaterial->m_lightmaps.m_lightmapIndirect = nullptr;
        else
            customMaterial->m_lightmaps.m_lightmapIndirect = m_lightmapIndirect->getRenderImage();

        if (!m_lightmapRadiosity)
            customMaterial->m_lightmaps.m_lightmapRadiosity = nullptr;
        else
            customMaterial->m_lightmaps.m_lightmapRadiosity = m_lightmapRadiosity->getRenderImage();

        if (!m_lightmapShadow)
            customMaterial->m_lightmaps.m_lightmapShadow = nullptr;
        else
            customMaterial->m_lightmaps.m_lightmapShadow = m_lightmapShadow->getRenderImage();

        if (!m_iblProbe)
            customMaterial->m_iblProbe = nullptr;
        else
            customMaterial->m_iblProbe = m_iblProbe->getRenderImage();

        customMaterial->m_cullMode = QSSGCullFaceMode(m_cullMode);
        node = customMaterial;
    }

    return node;
}

void QQuick3DMaterial::itemChange(QQuick3DObject::ItemChange change, const QQuick3DObject::ItemChangeData &value)
{
    if (change == QQuick3DObject::ItemSceneChange)
        updateSceneManager(value.sceneManager);
}

void QQuick3DMaterial::updateSceneManager(QQuick3DSceneManager *sceneManager)
{
    if (sceneManager) {
        QQuick3DObjectPrivate::refSceneManager(m_lightmapIndirect, *sceneManager);
        QQuick3DObjectPrivate::refSceneManager(m_lightmapRadiosity, *sceneManager);
        QQuick3DObjectPrivate::refSceneManager(m_lightmapShadow, *sceneManager);
        QQuick3DObjectPrivate::refSceneManager(m_iblProbe, *sceneManager);
    } else {
       QQuick3DObjectPrivate::derefSceneManager(m_lightmapIndirect);
       QQuick3DObjectPrivate::derefSceneManager(m_lightmapRadiosity);
       QQuick3DObjectPrivate::derefSceneManager(m_lightmapShadow);
       QQuick3DObjectPrivate::derefSceneManager(m_iblProbe);
    }
}

QT_END_NAMESPACE
