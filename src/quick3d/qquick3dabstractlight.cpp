// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dabstractlight_p.h"
#include "qquick3dobject_p.h"
#include "qquick3dnode_p_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Light
    \inherits Node
    \inqmlmodule QtQuick3D
    \brief An uncreatable abstract base type for all lights.

    Light itself is an uncreatable base for all of its subtypes. The subtypes provide multiple
    options to determine the style of the light.

    For usage examples, see \l{Qt Quick 3D - Lights Example}.

    \sa DirectionalLight, PointLight
*/

/*!
    \qmlproperty color Light::color
    This property defines the color applied to models illuminated by this light.
    The default value is white, rgb(255, 255, 255).
 */

/*!
    \qmlproperty color Light::ambientColor
    The property defines the ambient color applied to materials before being lit by this light.
    The default value is black, rgb(0, 0, 0).
 */

/*!
    \qmlproperty real Light::brightness
    This property defines an overall multiplier for this light’s effects.
    The default value is 1.
*/

/*!
    \qmlproperty Node Light::scope

    The property allows the selection of a Node in the scene. Only that node
    and its children are affected by this light. By default the value is null,
    which indicates no scope selected.

    \note Scoped lights cannot cast real-time shadows, meaning a Light with a
    scope set should not set \l castsShadow to true. They can however generate
    baked shadows when \l bakeMode is set to Light.BakeModeAll.
*/

/*!
    \qmlproperty bool Light::castsShadow

    When this property is enabled, the light will cast (real-time) shadows. The
    default value is false.

    \note When \l bakeMode is set to Light.BakeModeAll, this property has no
    effect. A fully baked light always has baked shadows, but it will never
    participate in real-time shadow mapping.
*/

/*!
    \qmlproperty real Light::shadowBias
    This property is used to tweak the shadowing effect when objects
    are casting shadows on themselves. The value range is [-1.0, 1.0]. Generally, a value
    inside [-0.1, 0.1] is sufficient.
    The default value is 0.
*/

/*!
    \qmlproperty real Light::shadowFactor
    This property determines how dark the cast shadows should be. The value range is [0, 100], where
    0 means no shadows and 100 means the light is fully shadowed.
    The default value is 5.
*/

/*!
    \qmlproperty enumeration Light::shadowMapQuality
    The property sets the quality of the shadow map created for shadow rendering. Lower quality uses
    less resources, but produces lower quality shadows while higher quality uses more resources, but
    produces better quality shadows.

    Supported quality values are:
    \value Light.ShadowMapQualityLow Render shadowmap using 256x256 texture.
    \value Light.ShadowMapQualityMedium Render shadowmap using 512x512 texture.
    \value Light.ShadowMapQualityHigh Render shadowmap using 1024x1024 texture.
    \value Light.ShadowMapQualityVeryHigh Render shadowmap using 2048x2048 texture.

    The default value is \c Light.ShadowMapQualityLow
*/

/*!
    \qmlproperty real Light::shadowMapFar
    The property determines the maximum distance for the shadow map. Smaller
    values improve the precision and effects of the map.
    The default value is 5000. Unit is points in local coordinate space.
*/

/*!
    \qmlproperty real Light::shadowFilter
    This property sets how much blur is applied to the shadows.
    The default value is 5.
*/

/*!
    \qmlproperty enumeration Light::bakeMode
    The property controls if the light is active in baked lighting, such as
    when generating lightmaps.

    \value Light.BakeModeDisabled The light is not used in baked lighting.

    \value Light.BakeModeIndirect Indirect lighting contribution (for global
    illumination) is baked for this light. Direct lighting (diffuse, specular,
    real-time shadow mapping) is calculated normally for the light at run time.
    At run time, when not in baking mode, the renderer will attempt to sample
    the lightmap to get the indirect lighting data and combine that with the
    results of the real-time calculations.

    \value Light.BakeModeAll Both direct (diffuse, shadow) and indirect
    lighting is baked for this light. The light will not have a specular
    contribution and will not generate realtime shadow maps, but it will always
    have baked shadows. At run time, when not in baking mode, the renderer will
    attempt to sample the lightmap in place of the standard, real-time
    calculations for diffuse lighting and shadow mapping.

    The default value is \c Light.BakeModeDisabled

    \note Just as with \l Model::usedInBakedLighting, designers and developers
    must always evaluate on a per-light basis if the light is suitable to take
    part in baked lighting.

    \warning Lights with dynamically changing properties, for example, animated
    position, rotation, or other properties, are not suitable for participating
    in baked lighting.

    This property is relevant both when baking and when using lightmaps. A
    consistent state between the baking run and the subsequent runs that use
    the generated data is essential. Changing to a different value will not
    change the previously generated and persistently stored data in the
    lightmaps, the engine's rendering behavior will however follow the
    property's current value.

    For more information on how to bake lightmaps, see the \l {Lightmaps and
    Global Illumination}.

    \sa Model::usedInBakedLighting, Model::bakedLightmap, Lightmapper, {Lightmaps and Global Illumination}
*/

QQuick3DAbstractLight::QQuick3DAbstractLight(QQuick3DNodePrivate &dd, QQuick3DNode *parent)
    : QQuick3DNode(dd, parent)
    , m_color(Qt::white)
    , m_ambientColor(Qt::black) {}

QQuick3DAbstractLight::~QQuick3DAbstractLight() {}

QColor QQuick3DAbstractLight::color() const
{
    return m_color;
}

QColor QQuick3DAbstractLight::ambientColor() const
{
    return m_ambientColor;
}

float QQuick3DAbstractLight::brightness() const
{
    return m_brightness;
}

QQuick3DNode *QQuick3DAbstractLight::scope() const
{
    return m_scope;
}

bool QQuick3DAbstractLight::castsShadow() const
{
    return m_castsShadow;
}

float QQuick3DAbstractLight::shadowBias() const
{
    return m_shadowBias;
}

float QQuick3DAbstractLight::shadowFactor() const
{
    return m_shadowFactor;
}

QQuick3DAbstractLight::QSSGShadowMapQuality QQuick3DAbstractLight::shadowMapQuality() const
{
    return m_shadowMapQuality;
}

float QQuick3DAbstractLight::shadowMapFar() const
{
    return m_shadowMapFar;
}

float QQuick3DAbstractLight::shadowFilter() const
{
    return m_shadowFilter;
}

QQuick3DAbstractLight::QSSGBakeMode QQuick3DAbstractLight::bakeMode() const
{
    return m_bakeMode;
}

void QQuick3DAbstractLight::markAllDirty()
{
    m_dirtyFlags = DirtyFlags(DirtyFlag::ShadowDirty)
            | DirtyFlags(DirtyFlag::ColorDirty)
            | DirtyFlags(DirtyFlag::BrightnessDirty)
            | DirtyFlags(DirtyFlag::FadeDirty)
            | DirtyFlags(DirtyFlag::AreaDirty)
            | DirtyFlags(DirtyFlag::BakeModeDirty);
    QQuick3DNode::markAllDirty();
}

void QQuick3DAbstractLight::setColor(const QColor &color)
{
    if (m_color == color)
        return;

    m_color = color;
    m_dirtyFlags.setFlag(DirtyFlag::ColorDirty);
    emit colorChanged();
    update();
}

void QQuick3DAbstractLight::setAmbientColor(const QColor &ambientColor)
{
    if (m_ambientColor == ambientColor)
        return;

    m_ambientColor = ambientColor;
    m_dirtyFlags.setFlag(DirtyFlag::ColorDirty);
    emit ambientColorChanged();
    update();
}

void QQuick3DAbstractLight::setBrightness(float brightness)
{
    if (qFuzzyCompare(m_brightness, brightness))
        return;

    m_brightness = brightness;
    m_dirtyFlags.setFlag(DirtyFlag::BrightnessDirty);
    emit brightnessChanged();
    update();
}

void QQuick3DAbstractLight::setScope(QQuick3DNode *scope)
{
    if (m_scope == scope)
        return;

    m_scope = scope;
    emit scopeChanged();
    update();
}

void QQuick3DAbstractLight::setCastsShadow(bool castsShadow)
{
    if (m_castsShadow == castsShadow)
        return;

    m_castsShadow = castsShadow;
    m_dirtyFlags.setFlag(DirtyFlag::ShadowDirty);
    emit castsShadowChanged();
    update();
}

void QQuick3DAbstractLight::setShadowBias(float shadowBias)
{
    shadowBias = qBound(-1.0f, shadowBias, 1.0f);
    if (qFuzzyCompare(m_shadowBias, shadowBias))
        return;

    m_shadowBias = shadowBias;
    m_dirtyFlags.setFlag(DirtyFlag::ShadowDirty);
    emit shadowBiasChanged();
    update();
}

void QQuick3DAbstractLight::setShadowFactor(float shadowFactor)
{
    shadowFactor = qBound(0.0f, shadowFactor, 100.0f);
    if (qFuzzyCompare(m_shadowFactor, shadowFactor))
        return;

    m_shadowFactor = shadowFactor;
    m_dirtyFlags.setFlag(DirtyFlag::ShadowDirty);
    emit shadowFactorChanged();
    update();
}

void QQuick3DAbstractLight::setShadowMapQuality(
        QQuick3DAbstractLight::QSSGShadowMapQuality shadowMapQuality)
{
    if (m_shadowMapQuality == shadowMapQuality)
        return;

    m_shadowMapQuality = shadowMapQuality;
    m_dirtyFlags.setFlag(DirtyFlag::ShadowDirty);
    emit shadowMapQualityChanged();
    update();
}

void QQuick3DAbstractLight::setBakeMode(QQuick3DAbstractLight::QSSGBakeMode bakeMode)
{
    if (m_bakeMode == bakeMode)
        return;

    m_bakeMode = bakeMode;
    m_dirtyFlags.setFlag(DirtyFlag::BakeModeDirty);
    emit bakeModeChanged();
    update();
}

void QQuick3DAbstractLight::setShadowMapFar(float shadowMapFar)
{
    if (qFuzzyCompare(m_shadowMapFar, shadowMapFar))
        return;

    m_shadowMapFar = shadowMapFar;
    m_dirtyFlags.setFlag(DirtyFlag::ShadowDirty);
    emit shadowMapFarChanged();
    update();
}

void QQuick3DAbstractLight::setShadowFilter(float shadowFilter)
{
    if (qFuzzyCompare(m_shadowFilter, shadowFilter))
        return;

    m_shadowFilter = shadowFilter;
    m_dirtyFlags.setFlag(DirtyFlag::ShadowDirty);
    emit shadowFilterChanged();
    update();
}

quint32 QQuick3DAbstractLight::mapToShadowResolution(QSSGShadowMapQuality quality)
{
    switch (quality) {
    case QSSGShadowMapQuality::ShadowMapQualityMedium:
        return 9;
    case QSSGShadowMapQuality::ShadowMapQualityHigh:
        return 10;
    case QSSGShadowMapQuality::ShadowMapQualityVeryHigh:
        return 11;
    default:
        break;
    }
    return 8;
}

QSSGRenderGraphObject *QQuick3DAbstractLight::updateSpatialNode(QSSGRenderGraphObject *node)
{
    Q_ASSERT_X(node, __FUNCTION__, "Node must have been created in parent class.");

    QQuick3DNode::updateSpatialNode(node);

    QSSGRenderLight *light = static_cast<QSSGRenderLight *>(node);

    if (m_dirtyFlags.toInt() != 0) // Some flag was set, so mark the light dirty!
        light->markDirty(QSSGRenderLight::DirtyFlag::LightDirty);

    if (m_dirtyFlags.testFlag(DirtyFlag::ColorDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::ColorDirty, false);
        light->m_diffuseColor = QSSGUtils::color::sRGBToLinear(m_color).toVector3D();
        light->m_specularColor = light->m_diffuseColor;
        light->m_ambientColor = QSSGUtils::color::sRGBToLinear(m_ambientColor).toVector3D();
    }

    if (m_dirtyFlags.testFlag(DirtyFlag::BrightnessDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::BrightnessDirty, false);
        light->m_brightness = m_brightness;
    }

    if (m_dirtyFlags.testFlag(DirtyFlag::ShadowDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::ShadowDirty, false);
        light->m_castShadow = m_castsShadow;
        light->m_shadowBias = m_shadowBias;
        light->m_shadowFactor = m_shadowFactor;
        light->m_shadowMapRes = mapToShadowResolution(m_shadowMapQuality);
        light->m_shadowMapFar = m_shadowMapFar;
        light->m_shadowFilter = m_shadowFilter;
    }

    if (m_dirtyFlags.testFlag(DirtyFlag::BakeModeDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::BakeModeDirty, false);
        light->m_bakingEnabled = m_bakeMode != QSSGBakeMode::BakeModeDisabled;
        light->m_fullyBaked = m_bakeMode == QSSGBakeMode::BakeModeAll;
    }

    if (m_scope) {
        // Special case:
        // If the 'scope' is 'this' and this is the first call, then the spatial node is the one we just created.
        // This is not unlikely, as it can make sense to put all child nodes that should receive light under the light node...
        if (m_scope == this)
            light->m_scope = light;
        else
            light->m_scope = static_cast<QSSGRenderNode*>(QQuick3DObjectPrivate::get(m_scope)->spatialNode);
    } else {
        light->m_scope = nullptr;
    }

    return node;
}

QT_END_NAMESPACE
