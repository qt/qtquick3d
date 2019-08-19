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

#include "qquick3dlight_p.h"
#include "qquick3dobject_p_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Light
    \inqmlmodule QtQuick3D
    \brief Lets you define the lighting for a 3D item.
*/

/*!
 * \qmlproperty enumeration Light::lightType
 *
 * This property defines which type of light is being used.
 *
 * \list
 * \li Light::Directional - Lighting is applied uniformly in the direction of
 * the Z axis for this light. The rotation of directional lights affects the
 * result, but not the position or scale.
 * \li Light::Point - Lighting is applied outwards from the center of the
 * light, becoming increasingly dim away from the center. The position of
 * point lights affects the result, but not the rotation or scale.
 * \li Light::Area - Lighting emits from the +Z face of a rectangular light.
 * Use the Light::AreaWidget and Light::AreaHeight properties to control the
 * size of the area light.
 * \endlist
 *
 */

/*!
 * \qmlproperty color Light::diffuseColor
 * This property defines the diffuse color (and intensity) applied to models
 * illuminated by this light.
 *
 */

/*!
 * \qmlproperty color Light::specularColor
 *
 * This property defines the specular color (and intensity) applied to models
 * illuminated by this light.
 *
 * \note A model’s material must have a non-zero Specular Amount for any specular lighting to take effect.
 *
 */

/*!
 * \qmlproperty color Light::ambientColor
 *
 * The property defines the diffuse color (and intensity) applied to materials
 * before being lit by this light.
 *
 */

/*!
 * \qmlproperty real Light::brightness
 *
 * This property defines an overall multiplier for a Point light’s effects.
 *
 */

/*!
 * \qmlproperty real Light::linearFade
 *
 * This property increases the rate at which the lighting effect dims the
 * farther surfaces are from a Point light.
 *
 */

/*!
 * \qmlproperty real Light::exponentialFade
 *
 * This property increases the rate at which the lighting effect dims on
 * surfaces that are extra far away from a Point light.
 *
 */

/*!
 * \qmlproperty real Light::areaWidth
 *
 * This property controls the width of an Area lights rectangle.
 *
 */

/*!
 * \qmlproperty real Light::areaHeight
 *
 * This property controls the height of an Area lights rectangle
 *
 */

/*!
 * \qmlproperty bool Light::castShadow
 *
 * When this property is enabled, the light will cast shadows.
 *
 */

/*!
 * \qmlproperty real Light::shadowBias
 *
 * This property is used to tweak the shadowing effect when when objects
 * are casting shadows on themselves
 *
 */

/*!
 * \qmlproperty real Light::shadowFactor
 *
 * This propery determines how dark the cast shadows should be.
 *
 */

/*!
 * \qmlproperty int Light::shadowMapResolution
 *
 * The propery sets the size of the shadow map created for shadow rendering.
 * This is specified as 2^n.  The larger the value the larger the shadow map
 * and this can have a huge affect on resource usage.
 *
 */

/*!
 * \qmlproperty real Light::shadowMapFar
 *
 * The property determines the maximum distance for the shadow map. Smaller
 * values ma improve the precision and effects of the map.
 *
 */

/*!
 * \qmlproperty real Light::shadowMapFieldOFView
 *
 * This property determines the field of view used by the simulated cameras
 * that render to the shadow map.
 *
 */

/*!
 * \qmlproperty real Light::shadowFilter
 *
 * This property sets how much blur is applied to the shadows.
 *
 */

/*!
 * \qmlproperty Node Light::scope
 *
 * The propery allows the selection of a Node in the scene which has the affect
 * that only that Node and it's children are affected by this light.
 *
 */

QQuick3DLight::QQuick3DLight() : m_diffuseColor(Qt::white), m_specularColor(Qt::white), m_ambientColor(Qt::black) {}

QQuick3DLight::~QQuick3DLight() {}

QQuick3DObject::Type QQuick3DLight::type() const
{
    return QQuick3DObject::Light;
}

QQuick3DLight::QSSGRenderLightTypes QQuick3DLight::lightType() const
{
    return m_lightType;
}

QColor QQuick3DLight::diffuseColor() const
{
    return m_diffuseColor;
}

QColor QQuick3DLight::specularColor() const
{
    return m_specularColor;
}

QColor QQuick3DLight::ambientColor() const
{
    return m_ambientColor;
}

float QQuick3DLight::brightness() const
{
    return m_brightness;
}

float QQuick3DLight::linearFade() const
{
    return m_linearFade;
}

float QQuick3DLight::exponentialFade() const
{
    return m_exponentialFade;
}

float QQuick3DLight::areaWidth() const
{
    return m_areaWidth;
}

float QQuick3DLight::areaHeight() const
{
    return m_areaHeight;
}

bool QQuick3DLight::castShadow() const
{
    return m_castShadow;
}

float QQuick3DLight::shadowBias() const
{
    return m_shadowBias;
}

float QQuick3DLight::shadowFactor() const
{
    return m_shadowFactor;
}

int QQuick3DLight::shadowMapResolution() const
{
    return m_shadowMapResolution;
}

float QQuick3DLight::shadowMapFar() const
{
    return m_shadowMapFar;
}

float QQuick3DLight::shadowMapFieldOfView() const
{
    return m_shadowMapFieldOfView;
}

float QQuick3DLight::shadowFilter() const
{
    return m_shadowFilter;
}

QQuick3DNode *QQuick3DLight::scope() const
{
    return m_scope;
}

void QQuick3DLight::setLightType(QQuick3DLight::QSSGRenderLightTypes lightType)
{
    if (m_lightType == lightType)
        return;

    m_lightType = lightType;
    emit lightTypeChanged(m_lightType);
    update();
}

void QQuick3DLight::setDiffuseColor(QColor diffuseColor)
{
    if (m_diffuseColor == diffuseColor)
        return;

    m_diffuseColor = diffuseColor;
    m_dirtyFlags.setFlag(DirtyFlag::ColorDirty);
    emit diffuseColorChanged(m_diffuseColor);
    update();
}

void QQuick3DLight::setSpecularColor(QColor specularColor)
{
    if (m_specularColor == specularColor)
        return;

    m_specularColor = specularColor;
    m_dirtyFlags.setFlag(DirtyFlag::ColorDirty);
    emit specularColorChanged(m_specularColor);
    update();
}

void QQuick3DLight::setAmbientColor(QColor ambientColor)
{
    if (m_ambientColor == ambientColor)
        return;

    m_ambientColor = ambientColor;
    m_dirtyFlags.setFlag(DirtyFlag::ColorDirty);
    emit ambientColorChanged(m_ambientColor);
    update();
}

void QQuick3DLight::setBrightness(float brightness)
{
    if (qFuzzyCompare(m_brightness, brightness))
        return;

    m_brightness = brightness;
    m_dirtyFlags.setFlag(DirtyFlag::BrightnessDirty);
    emit brightnessChanged(m_brightness);
    update();
}

void QQuick3DLight::setLinearFade(float linearFade)
{
    if (qFuzzyCompare(m_linearFade, linearFade))
        return;

    m_linearFade = linearFade;
    m_dirtyFlags.setFlag(DirtyFlag::BrightnessDirty);
    emit linearFadeChanged(m_linearFade);
    update();
}

void QQuick3DLight::setExponentialFade(float exponentialFade)
{
    if (qFuzzyCompare(m_exponentialFade, exponentialFade))
        return;

    m_exponentialFade = exponentialFade;
    m_dirtyFlags.setFlag(DirtyFlag::BrightnessDirty);
    emit exponentialFadeChanged(m_exponentialFade);
    update();
}

void QQuick3DLight::setAreaWidth(float areaWidth)
{
    if (qFuzzyCompare(m_areaWidth, areaWidth))
        return;

    m_areaWidth = areaWidth;
    m_dirtyFlags.setFlag(DirtyFlag::AreaDirty);
    emit areaWidthChanged(m_areaWidth);
    update();
}

void QQuick3DLight::setAreaHeight(float areaHeight)
{
    if (qFuzzyCompare(m_areaHeight, areaHeight))
        return;

    m_areaHeight = areaHeight;
    m_dirtyFlags.setFlag(DirtyFlag::AreaDirty);
    emit areaHeightChanged(m_areaHeight);
    update();
}

void QQuick3DLight::setCastShadow(bool castShadow)
{
    if (m_castShadow == castShadow)
        return;

    m_castShadow = castShadow;
    m_dirtyFlags.setFlag(DirtyFlag::ShadowDirty);
    emit castShadowChanged(m_castShadow);
    update();
}

void QQuick3DLight::setShadowBias(float shadowBias)
{
    if (qFuzzyCompare(m_shadowBias, shadowBias))
        return;

    m_shadowBias = shadowBias;
    m_dirtyFlags.setFlag(DirtyFlag::ShadowDirty);
    emit shadowBiasChanged(m_shadowBias);
    update();
}

void QQuick3DLight::setShadowFactor(float shadowFactor)
{
    if (qFuzzyCompare(m_shadowFactor, shadowFactor))
        return;

    m_shadowFactor = shadowFactor;
    m_dirtyFlags.setFlag(DirtyFlag::ShadowDirty);
    emit shadowFactorChanged(m_shadowFactor);
    update();
}

void QQuick3DLight::setShadowMapResolution(int shadowMapResolution)
{
    if (m_shadowMapResolution == shadowMapResolution)
        return;

    m_shadowMapResolution = shadowMapResolution;
    m_dirtyFlags.setFlag(DirtyFlag::ShadowDirty);
    emit shadowMapResolutionChanged(m_shadowMapResolution);
    update();
}

void QQuick3DLight::setShadowMapFar(float shadowMapFar)
{
    if (qFuzzyCompare(m_shadowMapFar, shadowMapFar))
        return;

    m_shadowMapFar = shadowMapFar;
    m_dirtyFlags.setFlag(DirtyFlag::ShadowDirty);
    emit shadowMapFarChanged(m_shadowMapFar);
    update();
}

void QQuick3DLight::setShadowMapFieldOfView(float shadowMapFieldOfView)
{
    if (qFuzzyCompare(m_shadowMapFieldOfView, shadowMapFieldOfView))
        return;

    m_shadowMapFieldOfView = shadowMapFieldOfView;
    m_dirtyFlags.setFlag(DirtyFlag::ShadowDirty);
    emit shadowMapFieldOfViewChanged(m_shadowMapFieldOfView);
    update();
}

void QQuick3DLight::setShadowFilter(float shadowFilter)
{
    if (qFuzzyCompare(m_shadowFilter, shadowFilter))
        return;

    m_shadowFilter = shadowFilter;
    m_dirtyFlags.setFlag(DirtyFlag::ShadowDirty);
    emit shadowFilterChanged(m_shadowFilter);
    update();
}

void QQuick3DLight::setScope(QQuick3DNode *scope)
{
    if (m_scope == scope)
        return;

    m_scope = scope;
    emit scopeChanged(m_scope);
    update();
}

QSSGRenderGraphObject *QQuick3DLight::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node)
        node = new QSSGRenderLight();

    QQuick3DNode::updateSpatialNode(node);

    QSSGRenderLight *light = static_cast<QSSGRenderLight *>(node);

    light->m_lightType = QSSGRenderLight::Type(m_lightType);
    if (m_dirtyFlags.testFlag(DirtyFlag::ColorDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::ColorDirty, false);
        light->m_diffuseColor = QVector3D(m_diffuseColor.redF(), m_diffuseColor.greenF(), m_diffuseColor.blueF());
        light->m_specularColor = QVector3D(m_specularColor.redF(), m_specularColor.greenF(), m_specularColor.blueF());
        light->m_ambientColor = QVector3D(m_ambientColor.redF(), m_ambientColor.greenF(), m_ambientColor.blueF());
    }

    if (m_dirtyFlags.testFlag(DirtyFlag::BrightnessDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::BrightnessDirty, false);
        light->m_brightness = m_brightness;
        light->m_linearFade = m_linearFade;
        light->m_exponentialFade = m_exponentialFade;
    }

    if (m_dirtyFlags.testFlag(DirtyFlag::AreaDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::AreaDirty, false);
        light->m_areaWidth = m_areaWidth;
        light->m_areaHeight = m_areaHeight;
    }

    if (m_dirtyFlags.testFlag(DirtyFlag::ShadowDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::ShadowDirty, false);
        light->m_castShadow = m_castShadow;
        light->m_shadowBias = m_shadowBias;
        light->m_shadowFactor = m_shadowFactor;
        light->m_shadowMapRes = m_shadowMapResolution;
        light->m_shadowMapFar = m_shadowMapFar;
        light->m_shadowMapFov = m_shadowMapFieldOfView;
        light->m_shadowFilter = m_shadowFilter;
    }

    if (m_scope)
        light->m_scope = static_cast<QSSGRenderNode*>(QQuick3DObjectPrivate::get(m_scope)->spatialNode);
    else
        light->m_scope = nullptr;

    return node;
}

QT_END_NAMESPACE
