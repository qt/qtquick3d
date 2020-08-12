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

#include "qquick3dprincipledmaterial_p.h"
#include "qquick3dobject_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterial_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype PrincipledMaterial
    \inherits Material
    \inqmlmodule QtQuick3D
    \brief Lets you define a material for 3D items.

    Before a Model can be rendered in a scene, it must have at least one
    material to define how the mesh is shaded. The PrincipledMaterial aims to be
    easy to use and with as few parameters as possible. In addition to having few
    parameters, all input values are strictly normalized between 0 and 1.
    Even if you define a PrincipledMaterial with no properties set, a valid mesh will be rendered,
    because the mesh defines some sensible defaults.

    As you change the properties of the PrincipledMaterial, behind the scenes
    new shaders are generated, and the property values are bound. The
    complexity of a shader depends on a combination of the properties that
    are set on it, and the context of the scene itself.
*/

/*!
    \qmlproperty enumeration PrincipledMaterial::lighting

    This property defines which lighting method is used when generating this
    material.

    The default value is \c PrincipledMaterial.FragmentLighting

    When using \c PrincipledMaterial.FragmentLighting, diffuse and specular lighting is
    calculated for each rendered pixel. Certain effects (such as a Fresnel or normal map) require
    \c PrincipledMaterial.FragmentLighting to work.

    When using \c PrincipledMaterial.NoLighting no lighting is calculated. This
    mode is (predictably) very fast, and is quite effective when image maps are
    used that you do not need to be shaded by lighting.

    \value PrincipledMaterial.NoLighting
    \value PrincipledMaterial.FragmentLighting
*/

/*!
    \qmlproperty enumeration PrincipledMaterial::blendMode

    This property determines how the colors of the model rendered blends with
    those behind it.

    \value PrincipledMaterial.SourceOver
        Default blend mode. Opaque objects occlude objects behind them.
    \value PrincipledMaterial.Screen
        Colors are blended using an inverted multiply, producing a lighter result. This blend mode
        is order-independent; if you are using semi-opaque objects and experiencing 'popping'
        as faces or models sort differently, using Screen blending is one way to produce results
        without popping.
    \value PrincipledMaterial.Multiply
        Colors are blended using a multiply, producing a darker result. This blend mode is also
        order-independent.
    \value PrincipledMaterial.Overlay
        A mix of Multiply and Screen modes, producing a result with higher contrast.
    \value PrincipledMaterial.ColorBurn
        Colors are blended by inverted division where the result also is inverted, producing
        a darker result. Darker than Multiply.
    \value PrincipledMaterial.ColorDodge
        Colors are blended by inverted division, producing a lighter result. Lighter than Screen.
*/

/*!
    \qmlproperty color PrincipledMaterial::baseColor

    This property sets the base color for the material. Depending on the type
    of material specified (metal or dielectric) the diffuse and specular channels will be
    set appropriately. For example, a dielectric material will have a diffuse color equal to
    the base color, while it's specular color, depending on the specular amount, will have a
    bright specular color. For metals the diffuse and specular channels will be mixed from
    the base color and have a dark diffuse channel and a specular channel close to the base color.
*/

/*!
    \qmlproperty Texture PrincipledMaterial::baseColorMap

    This property defines the texture used to set the base color of the material.

    \sa baseColor
*/

/*!
    \qmlproperty real PrincipledMaterial::metalness

    The metalness property defines the \e metalness of the the material. The value
    is normalized, where 0.0 means the material is a \e dielectric (non-metallic) material and
    a value of 1.0 means the material is a metal.

    \note In principle, materials are either dielectrics with a metalness of 0, or metals with a
    metalness of 1. Metalness values between 0 and 1 are still allowed and will give a material that
    is a blend between the different models.
*/

/*!
    \qmlproperty Texture PrincipledMaterial::metalnessMap

    This property sets a Texture to be used to set the metalness amount for the
    different parts of the material.
*/

/*!
    \qmlproperty enumeration PrincipledMaterial::metalnessChannel

    This property defines the texture channel used to read the metalness value from metalnessMap.
    The default value is \c Material.B.

    \value Material.R Read value from texture R channel.
    \value Material.G Read value from texture G channel.
    \value Material.B Read value from texture B channel.
    \value Material.A Read value from texture A channel.
*/

/*!
    \qmlproperty Texture PrincipledMaterial::emissiveMap

    This property sets a Texture to be used to set the emissive factor for
    different parts of the material. Using a grayscale image will not affect the
    color of the result, while using a color image will produce glowing regions
    with the color affected by the emissive map.
*/

/*!
    \qmlproperty color PrincipledMaterial::emissiveColor

    This property determines the color of self-illumination for this material.
    If an emissive map is set, this property is used as a factor for the RGB channels
    of the texture.

    \note In a scene with black ambient lighting a material with a emissive factor of 0 will
    appear black wherever the light does not shine on it; turning the emissive
    factor to 1 will cause the material to appear as its diffuse color instead.

    \note When you want a material to not be affected by lighting, instead of
    using 100% emissiveFactor consider setting the lightingMode to
    \c PrincipledMaterial.NoLighting for a performance benefit.
*/

/*!
    \qmlproperty Texture PrincipledMaterial::specularReflectionMap

    This property sets a Texture used for specular highlights on the material.
    By default the Texture is applied using environmental mapping (not UV
    mapping): as you rotate the model the map will appear as though it is
    reflecting from the environment. Specular Reflection maps are an easy way to
    add a high-quality look with relatively low cost.

    \note Using a Light Probe in your \l SceneEnvironment for image-based lighting
    will automatically use that image as the specular reflection.

    \note Crisp images cause your material to look very glossy; the more you
    blur your image the softer your material will appear.
*/

/*!
    \qmlproperty Texture PrincipledMaterial::specularMap

    The property defines a RGB Texture to modulate the amount and the color of
    specularity across the surface of the material. These values are multiplied
    by the specularAmount.

    \note The specular map will be ignored unless the material is dielectric.
*/

/*!
    \qmlproperty real PrincipledMaterial::specularTint

    This property defines how much of the base color contributes to the specular reflections.

    \note This property does only apply to dielectric materials.
*/

/*!
    \qmlproperty real PrincipledMaterial::indexOfRefraction

    This property controls how fast light travels through the material.
*/

/*!
    \qmlproperty real PrincipledMaterial::specularAmount

    This property controls the strength of specularity (highlights and
    reflections).

    \note For non-dielectrics (metals) this property has no effect.

    \note This property does not affect the specularReflectionMap, but does affect the amount of
    reflections from a scenes SceneEnvironment::lightProbe.

    \note Unless your mesh is high resolution, you may need to use
    \c PrincipledMaterial.FragmentLighting to get good specular highlights from scene
    lights.
*/

/*!
    \qmlproperty real PrincipledMaterial::roughness

    This property controls the size of the specular highlight generated from
    lights, and the clarity of reflections in general. Larger values increase
    the roughness, softening specular highlights and blurring reflections.
*/

/*!
    \qmlproperty Texture PrincipledMaterial::roughnessMap

    This property defines a Texture to control the specular roughness of the
    material.
*/

/*!
    \qmlproperty enumeration PrincipledMaterial::roughnessChannel

    This property defines the texture channel used to read the roughness value from roughnessMap.
    The default value is \c Material.G.

    \value Material.R Read value from texture R channel.
    \value Material.G Read value from texture G channel.
    \value Material.B Read value from texture B channel.
    \value Material.A Read value from texture A channel.
*/

/*!
    \qmlproperty real PrincipledMaterial::opacity

    This property drops the opacity of just this material, separate from the
    model.
*/

/*!
    \qmlproperty Texture PrincipledMaterial::opacityMap

    This property defines a Texture used to control the opacity differently for
    different parts of the material.
*/

/*!
    \qmlproperty enumeration PrincipledMaterial::opacityChannel

    This property defines the texture channel used to read the opacity value from opacityMap.
    The default value is \c Material.A.

    \value Material.R Read value from texture R channel.
    \value Material.G Read value from texture G channel.
    \value Material.B Read value from texture B channel.
    \value Material.A Read value from texture A channel.
*/

/*!
    \qmlproperty Texture PrincipledMaterial::normalMap

    This property defines an RGB image used to simulate fine geometry
    displacement across the surface of the material. The RGB channels indicate
    XYZ normal deviations.

    \note Normal maps will not affect the silhouette of a model.
*/

/*!
    \qmlproperty real PrincipledMaterial::normalStrength

    This property controls the amount of simulated displacement for the normalMap.
*/

/*!
    \qmlproperty real PrincipledMaterial::occlusionAmount

    This property contains the factor used to modify the values from the \l occlusionMap texture.
    The value should be between 0.0 to 1.0. The default is 1.0
*/

/*!
    \qmlproperty Texture PrincipledMaterial::occlusionMap

    This property defines a texture used to determine how much indirect light the different areas of the
    material should receive. Values are expected to be linear from 0.0 to 1.0, where 0.0 means no indirect lighting
    and 1.0 means the effect of the indirect lighting is left unchanged.

    \sa occlusionAmount
*/

/*!
    \qmlproperty enumeration PrincipledMaterial::occlusionChannel

    This property defines the texture channel used to read the occlusion value from occlusionMap.
    The default value is \c Material.R.

    \value Material.R Read value from texture R channel.
    \value Material.G Read value from texture G channel.
    \value Material.B Read value from texture B channel.
    \value Material.A Read value from texture A channel.
*/

/*!
    \qmlproperty enumeration PrincipledMaterial::alphaMode

    This property sets the mode for how the alpha channel of material color is used.

    \value PrincipledMaterial.Opaque The alpha channel is ignored and the output is rendered
            fully opaque. This is the default.
    \value PrincipledMaterial.Mask The output is either fully transparent of fully opaque depending
            on the alpha value and the specified \l alphaCutoff value.
    \value PrincipledMaterial.Blend The output is blended with the background.

    \note These modes only consider the alpha channel of the material's
    \l {baseColor} {color} or \l {baseColorMap}{color map}.
    The general \l opacity of the material does therefore not affect
    how the \c alphaMode or \c alphaCutoff is interpreted.
*/

/*!
    \qmlproperty real PrincipledMaterial::alphaCutoff

    The alphaCutoff property can be used to specify the cutoff value when using the
    \l {alphaMode} {Mask alphaMode}. Alpha values below the threshold will be rendered
    fully transparent, everything else will be fully opaque. The default value is 0.5

    \sa alphaMode
*/

inline static float ensureNormalized(float val) { return qBound(0.0f, val, 1.0f); }

QQuick3DPrincipledMaterial::QQuick3DPrincipledMaterial(QQuick3DObject *parent)
    : QQuick3DMaterial(*(new QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::PrincipledMaterial)), parent)
{}

QQuick3DPrincipledMaterial::~QQuick3DPrincipledMaterial()
{
    for (auto cit = m_connections.cbegin(), end = m_connections.cend(); cit != end; ++cit)
        disconnect(*cit);
}

QQuick3DPrincipledMaterial::Lighting QQuick3DPrincipledMaterial::lighting() const
{
    return m_lighting;
}

QQuick3DPrincipledMaterial::BlendMode QQuick3DPrincipledMaterial::blendMode() const
{
    return m_blendMode;
}

QColor QQuick3DPrincipledMaterial::baseColor() const
{
    return m_baseColor;
}

QQuick3DTexture *QQuick3DPrincipledMaterial::baseColorMap() const
{
    return m_baseColorMap;
}

QQuick3DTexture *QQuick3DPrincipledMaterial::emissiveMap() const
{
    return m_emissiveMap;
}

QColor QQuick3DPrincipledMaterial::emissiveColor() const
{
    return m_emissiveColor;
}

QQuick3DTexture *QQuick3DPrincipledMaterial::specularReflectionMap() const
{
    return m_specularReflectionMap;
}

QQuick3DTexture *QQuick3DPrincipledMaterial::specularMap() const
{
    return m_specularMap;
}

float QQuick3DPrincipledMaterial::specularTint() const
{
    return m_specularTint;
}

float QQuick3DPrincipledMaterial::indexOfRefraction() const
{
    return m_indexOfRefraction;
}

float QQuick3DPrincipledMaterial::specularAmount() const
{
    return m_specularAmount;
}

float QQuick3DPrincipledMaterial::roughness() const
{
    return m_roughness;
}

QQuick3DTexture *QQuick3DPrincipledMaterial::roughnessMap() const
{
    return m_roughnessMap;
}

float QQuick3DPrincipledMaterial::opacity() const
{
    return m_opacity;
}

QQuick3DTexture *QQuick3DPrincipledMaterial::opacityMap() const
{
    return m_opacityMap;
}

QQuick3DTexture *QQuick3DPrincipledMaterial::normalMap() const
{
    return m_normalMap;
}

float QQuick3DPrincipledMaterial::metalness() const
{
    return m_metalnessAmount;
}

QQuick3DTexture *QQuick3DPrincipledMaterial::metalnessMap() const
{
    return m_metalnessMap;
}

float QQuick3DPrincipledMaterial::normalStrength() const
{
    return m_normalStrength;
}

QQuick3DTexture *QQuick3DPrincipledMaterial::occlusionMap() const
{
    return m_occlusionMap;
}

float QQuick3DPrincipledMaterial::occlusionAmount() const
{
    return m_occlusionAmount;
}

QQuick3DPrincipledMaterial::AlphaMode QQuick3DPrincipledMaterial::alphaMode() const
{
    return m_alphaMode;
}

float QQuick3DPrincipledMaterial::alphaCutoff() const
{
    return m_alphaCutoff;
}

QQuick3DMaterial::TextureChannelMapping QQuick3DPrincipledMaterial::roughnessChannel() const
{
    return m_roughnessChannel;
}

QQuick3DMaterial::TextureChannelMapping QQuick3DPrincipledMaterial::opacityChannel() const
{
    return m_opacityChannel;
}

QQuick3DMaterial::TextureChannelMapping QQuick3DPrincipledMaterial::metalnessChannel() const
{
    return m_metalnessChannel;
}

QQuick3DMaterial::TextureChannelMapping QQuick3DPrincipledMaterial::occlusionChannel() const
{
    return m_occlusionChannel;
}

void QQuick3DPrincipledMaterial::markAllDirty()
{
    m_dirtyAttributes = 0xffffffff;
    QQuick3DMaterial::markAllDirty();
}

void QQuick3DPrincipledMaterial::setLighting(QQuick3DPrincipledMaterial::Lighting lighting)
{
    if (m_lighting == lighting)
        return;

    m_lighting = lighting;
    emit lightingChanged(m_lighting);
    markDirty(LightingModeDirty);
}

void QQuick3DPrincipledMaterial::setBlendMode(QQuick3DPrincipledMaterial::BlendMode blendMode)
{
    if (m_blendMode == blendMode)
        return;

    m_blendMode = blendMode;
    emit blendModeChanged(m_blendMode);
    markDirty(BlendModeDirty);
}

void QQuick3DPrincipledMaterial::setBaseColor(QColor diffuseColor)
{
    if (m_baseColor == diffuseColor)
        return;

    m_baseColor = diffuseColor;
    emit baseColorChanged(m_baseColor);
    markDirty(BaseColorDirty);
}

void QQuick3DPrincipledMaterial::setBaseColorMap(QQuick3DTexture *baseColorMap)
{
    if (m_baseColorMap == baseColorMap)
        return;

    updatePropertyListener(baseColorMap, m_baseColorMap, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("baseColorMap"), m_connections, [this](QQuick3DObject *n) {
        setBaseColorMap(qobject_cast<QQuick3DTexture *>(n));
    });

    m_baseColorMap = baseColorMap;
    emit baseColorMapChanged(m_baseColorMap);
    markDirty(BaseColorDirty);
}

void QQuick3DPrincipledMaterial::setEmissiveMap(QQuick3DTexture *emissiveMap)
{
    if (m_emissiveMap == emissiveMap)
        return;

    updatePropertyListener(emissiveMap, m_emissiveMap, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("emissiveMap"), m_connections, [this](QQuick3DObject *n) {
        setEmissiveMap(qobject_cast<QQuick3DTexture *>(n));
    });

    m_emissiveMap = emissiveMap;
    emit emissiveMapChanged(m_emissiveMap);
    markDirty(EmissiveDirty);
}

void QQuick3DPrincipledMaterial::setEmissiveColor(QColor emissiveColor)
{
    if (m_emissiveColor == emissiveColor)
        return;

    m_emissiveColor = emissiveColor;
    emit emissiveColorChanged(m_emissiveColor);
    markDirty(EmissiveDirty);
}

void QQuick3DPrincipledMaterial::setSpecularReflectionMap(QQuick3DTexture *specularReflectionMap)
{
    if (m_specularReflectionMap == specularReflectionMap)
        return;

    updatePropertyListener(specularReflectionMap, m_specularReflectionMap, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("specularReflectionMap"), m_connections, [this](QQuick3DObject *n) {
        setSpecularReflectionMap(qobject_cast<QQuick3DTexture *>(n));
    });

    m_specularReflectionMap = specularReflectionMap;
    emit specularReflectionMapChanged(m_specularReflectionMap);
    markDirty(SpecularDirty);
}

void QQuick3DPrincipledMaterial::setSpecularMap(QQuick3DTexture *specularMap)
{
    if (m_specularMap == specularMap)
        return;

    updatePropertyListener(specularMap, m_specularMap, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("specularMap"), m_connections, [this](QQuick3DObject *n) {
        setSpecularMap(qobject_cast<QQuick3DTexture *>(n));
    });

    m_specularMap = specularMap;
    emit specularMapChanged(m_specularMap);
    markDirty(SpecularDirty);
}

void QQuick3DPrincipledMaterial::setSpecularTint(float specularTint)
{
    specularTint = ensureNormalized(specularTint);
    if (qFuzzyCompare(m_specularTint, specularTint))
        return;

    m_specularTint = specularTint;
    emit specularTintChanged(m_specularTint);
    markDirty(SpecularDirty);
}

void QQuick3DPrincipledMaterial::setIndexOfRefraction(float indexOfRefraction)
{
    if (qFuzzyCompare(m_indexOfRefraction, indexOfRefraction))
        return;

    m_indexOfRefraction = indexOfRefraction;
    emit indexOfRefractionChanged(m_indexOfRefraction);
    markDirty(IorDirty);
}

void QQuick3DPrincipledMaterial::setSpecularAmount(float specularAmount)
{
    specularAmount = ensureNormalized(specularAmount);
    if (qFuzzyCompare(m_specularAmount, specularAmount))
        return;

    m_specularAmount = specularAmount;
    emit specularAmountChanged(m_specularAmount);
    markDirty(SpecularDirty);
}

void QQuick3DPrincipledMaterial::setRoughness(float roughness)
{
    roughness = ensureNormalized(roughness);
    if (qFuzzyCompare(m_roughness, roughness))
        return;

    m_roughness = roughness;
    emit roughnessChanged(m_roughness);
    markDirty(RoughnessDirty);
}

void QQuick3DPrincipledMaterial::setRoughnessMap(QQuick3DTexture *roughnessMap)
{
    if (m_roughnessMap == roughnessMap)
        return;

    updatePropertyListener(roughnessMap, m_roughnessMap, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("roughnessMap"), m_connections, [this](QQuick3DObject *n) {
        setRoughnessMap(qobject_cast<QQuick3DTexture *>(n));
    });

    m_roughnessMap = roughnessMap;
    emit roughnessMapChanged(m_roughnessMap);
    markDirty(RoughnessDirty);
}

void QQuick3DPrincipledMaterial::setOpacity(float opacity)
{
    opacity = ensureNormalized(opacity);
    if (qFuzzyCompare(m_opacity, opacity))
        return;

    m_opacity = opacity;
    emit opacityChanged(m_opacity);
    markDirty(OpacityDirty);
}

void QQuick3DPrincipledMaterial::setOpacityMap(QQuick3DTexture *opacityMap)
{
    if (m_opacityMap == opacityMap)
        return;

    updatePropertyListener(opacityMap, m_opacityMap, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("opacityMap"), m_connections, [this](QQuick3DObject *n) {
        setOpacityMap(qobject_cast<QQuick3DTexture *>(n));
    });

    m_opacityMap = opacityMap;
    emit opacityMapChanged(m_opacityMap);
    markDirty(OpacityDirty);
}

void QQuick3DPrincipledMaterial::setNormalMap(QQuick3DTexture *normalMap)
{
    if (m_normalMap == normalMap)
        return;

    updatePropertyListener(normalMap, m_normalMap, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("normalMap"), m_connections, [this](QQuick3DObject *n) {
        setNormalMap(qobject_cast<QQuick3DTexture *>(n));
    });

    m_normalMap = normalMap;
    emit normalMapChanged(m_normalMap);
    markDirty(NormalDirty);
}

void QQuick3DPrincipledMaterial::setMetalness(float metalnessAmount)
{
    metalnessAmount = ensureNormalized(metalnessAmount);
    if (qFuzzyCompare(m_metalnessAmount, metalnessAmount))
        return;

    m_metalnessAmount = metalnessAmount;
    emit metalnessChanged(m_metalnessAmount);
    markDirty(MetalnessDirty);
}

void QQuick3DPrincipledMaterial::setMetalnessMap(QQuick3DTexture *metallicMap)
{
    if (m_metalnessMap == metallicMap)
        return;

    updatePropertyListener(metallicMap, m_metalnessMap, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("metalnessMap"), m_connections, [this](QQuick3DObject *n) {
        setMetalnessMap(qobject_cast<QQuick3DTexture *>(n));
    });

    m_metalnessMap = metallicMap;
    emit metalnessMapChanged(m_metalnessMap);
    markDirty(MetalnessDirty);
}

void QQuick3DPrincipledMaterial::setNormalStrength(float factor)
{
    factor = ensureNormalized(factor);
    if (qFuzzyCompare(m_normalStrength, factor))
        return;

    m_normalStrength = factor;
    emit normalStrengthChanged(m_normalStrength);
    markDirty(NormalDirty);
}

void QQuick3DPrincipledMaterial::setOcclusionMap(QQuick3DTexture *occlusionMap)
{
    if (m_occlusionMap == occlusionMap)
        return;

    updatePropertyListener(occlusionMap, m_occlusionMap, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("occlusionMap"), m_connections, [this](QQuick3DObject *n) {
        setOcclusionMap(qobject_cast<QQuick3DTexture *>(n));
    });

    m_occlusionMap = occlusionMap;
    emit occlusionMapChanged(m_occlusionMap);
    markDirty(OcclusionDirty);
}

void QQuick3DPrincipledMaterial::setOcclusionAmount(float occlusionAmount)
{
    if (qFuzzyCompare(m_occlusionAmount, occlusionAmount))
        return;

    m_occlusionAmount = occlusionAmount;
    emit occlusionAmountChanged(m_occlusionAmount);
    markDirty(OcclusionDirty);
}

void QQuick3DPrincipledMaterial::setAlphaMode(QQuick3DPrincipledMaterial::AlphaMode alphaMode)
{
    if (m_alphaMode == alphaMode)
        return;

    m_alphaMode = alphaMode;
    emit alphaModeChanged(m_alphaMode);
    markDirty(AlphaModeDirty);
}

void QQuick3DPrincipledMaterial::setAlphaCutoff(float alphaCutoff)
{
    if (qFuzzyCompare(m_alphaCutoff, alphaCutoff))
        return;

    m_alphaCutoff = alphaCutoff;
    emit alphaCutoffChanged(m_alphaCutoff);
    markDirty(AlphaModeDirty);
}

void QQuick3DPrincipledMaterial::setMetalnessChannel(TextureChannelMapping channel)
{
    if (m_metalnessChannel == channel)
        return;

    m_metalnessChannel = channel;
    emit metalnessChannelChanged(channel);
    markDirty(MetalnessDirty);
}

void QQuick3DPrincipledMaterial::setRoughnessChannel(TextureChannelMapping channel)
{
    if (m_roughnessChannel == channel)
        return;

    m_roughnessChannel = channel;
    emit roughnessChannelChanged(channel);
    markDirty(RoughnessDirty);
}

void QQuick3DPrincipledMaterial::setOpacityChannel(TextureChannelMapping channel)
{
    if (m_opacityChannel == channel)
        return;

    m_opacityChannel = channel;
    emit opacityChannelChanged(channel);
    markDirty(OpacityDirty);
}

void QQuick3DPrincipledMaterial::setOcclusionChannel(TextureChannelMapping channel)
{
    if (m_occlusionChannel == channel)
        return;

    m_occlusionChannel = channel;
    emit occlusionChannelChanged(channel);
    markDirty(OcclusionDirty);
}

QSSGRenderGraphObject *QQuick3DPrincipledMaterial::updateSpatialNode(QSSGRenderGraphObject *node)
{
    static const auto colorToVec3 = [](const QColor &c) {
        return QVector3D{float(c.redF()), float(c.greenF()), float(c.blueF())};
    };

    static const auto channelMapping = [](TextureChannelMapping mapping) {
        return QSSGRenderDefaultMaterial::TextureChannelMapping(mapping);
    };

    if (!node) {
        markAllDirty();
        node = new QSSGRenderDefaultMaterial(QSSGRenderGraphObject::Type::PrincipledMaterial);
    }

    // Set common material properties
    QQuick3DMaterial::updateSpatialNode(node);

    QSSGRenderDefaultMaterial *material = static_cast<QSSGRenderDefaultMaterial *>(node);

    if (m_dirtyAttributes & LightingModeDirty)
        material->lighting = QSSGRenderDefaultMaterial::MaterialLighting(m_lighting);

    if (m_dirtyAttributes & BlendModeDirty)
        material->blendMode = QSSGRenderDefaultMaterial::MaterialBlendMode(m_blendMode);

    if (m_dirtyAttributes & BaseColorDirty) {
        if (!m_baseColorMap)
            material->colorMap = nullptr;
        else
            material->colorMap = m_baseColorMap->getRenderImage();

        material->color = QVector4D{colorToVec3(m_baseColor), float(m_baseColor.alphaF())};
    }

    if (m_dirtyAttributes & EmissiveDirty) {
        if (!m_emissiveMap)
            material->emissiveMap = nullptr;
        else
            material->emissiveMap = m_emissiveMap->getRenderImage();

        material->emissiveColor = colorToVec3(m_emissiveColor);
    }

    material->fresnelPower = 5.0f;

    if (m_dirtyAttributes & IorDirty)
        material->ior = m_indexOfRefraction;

    if (m_dirtyAttributes & RoughnessDirty) {
        if (!m_roughnessMap)
            material->roughnessMap = nullptr;
        else
            material->roughnessMap = m_roughnessMap->getRenderImage();

        material->specularRoughness = m_roughness;
        material->roughnessChannel = channelMapping(m_roughnessChannel);
    }

    if (m_dirtyAttributes & MetalnessDirty) {
        if (!m_metalnessMap)
            material->metalnessMap = nullptr;
        else
            material->metalnessMap = m_metalnessMap->getRenderImage();

        material->metalnessAmount = m_metalnessAmount;
        material->metalnessChannel = channelMapping(m_metalnessChannel);

        // Update specular values if needed.
        if (!material->isMetalnessEnabled()) {
            m_dirtyAttributes |= SpecularDirty;
        } else {
            material->specularAmount = m_specularAmount;
            material->specularTint = colorToVec3(Qt::white);
        }
    }

    // This test here is intentional, as we want to make sure we match the backend!
    const bool isDielectric = !material->isMetalnessEnabled();
    if (isDielectric && m_dirtyAttributes & SpecularDirty) {
        if (!m_specularReflectionMap)
            material->specularReflection = nullptr;
        else
            material->specularReflection = m_specularReflectionMap->getRenderImage();

        if (!m_specularMap) {
            material->specularMap = nullptr;
        } else {
            material->specularMap = m_specularMap->getRenderImage();
        }

        material->specularAmount = m_specularAmount;
        material->specularTint = QVector3D(m_specularTint, m_specularTint, m_specularTint);
    }

    if (m_dirtyAttributes & OpacityDirty) {
        material->opacity = m_opacity;
        if (!m_opacityMap)
            material->opacityMap = nullptr;
        else
            material->opacityMap = m_opacityMap->getRenderImage();

        material->opacity = m_opacity;
        material->opacityChannel = channelMapping(m_opacityChannel);
    }

    if (m_dirtyAttributes & NormalDirty) {
        if (!m_normalMap)
            material->normalMap = nullptr;
        else
            material->normalMap = m_normalMap->getRenderImage();

        material->bumpAmount = m_normalStrength;
    }

    if (m_dirtyAttributes & OcclusionDirty) {
        if (!m_occlusionMap)
            material->occlusionMap = nullptr;
        else
            material->occlusionMap = m_occlusionMap->getRenderImage();
        material->occlusionAmount = m_occlusionAmount;
        material->occlusionChannel = channelMapping(m_occlusionChannel);
    }

    if (m_dirtyAttributes & AlphaModeDirty) {
        material->alphaMode = QSSGRenderDefaultMaterial::MaterialAlphaMode(m_alphaMode);
        material->alphaCutoff = m_alphaCutoff;
    }

    m_dirtyAttributes = 0;

    return node;
}

void QQuick3DPrincipledMaterial::itemChange(QQuick3DObject::ItemChange change, const QQuick3DObject::ItemChangeData &value)
{
    if (change == QQuick3DObject::ItemSceneChange)
        updateSceneManager(value.sceneManager);
}

void QQuick3DPrincipledMaterial::updateSceneManager(const QSharedPointer<QQuick3DSceneManager> &window)
{
    // Check all the resource value's windows, and update as necessary
    if (window) {
        QQuick3DObjectPrivate::refSceneManager(m_baseColorMap, window);
        QQuick3DObjectPrivate::refSceneManager(m_emissiveMap, window);
        QQuick3DObjectPrivate::refSceneManager(m_specularReflectionMap, window);
        QQuick3DObjectPrivate::refSceneManager(m_specularMap, window);
        QQuick3DObjectPrivate::refSceneManager(m_roughnessMap, window);
        QQuick3DObjectPrivate::refSceneManager(m_opacityMap, window);
        QQuick3DObjectPrivate::refSceneManager(m_normalMap, window);
        QQuick3DObjectPrivate::refSceneManager(m_metalnessMap, window);
        QQuick3DObjectPrivate::refSceneManager(m_occlusionMap, window);
    } else {
        QQuick3DObjectPrivate::derefSceneManager(m_baseColorMap);
        QQuick3DObjectPrivate::derefSceneManager(m_emissiveMap);
        QQuick3DObjectPrivate::derefSceneManager(m_specularReflectionMap);
        QQuick3DObjectPrivate::derefSceneManager(m_specularMap);
        QQuick3DObjectPrivate::derefSceneManager(m_roughnessMap);
        QQuick3DObjectPrivate::derefSceneManager(m_opacityMap);
        QQuick3DObjectPrivate::derefSceneManager(m_normalMap);
        QQuick3DObjectPrivate::derefSceneManager(m_metalnessMap);
        QQuick3DObjectPrivate::derefSceneManager(m_occlusionMap);
    }
}

void QQuick3DPrincipledMaterial::markDirty(QQuick3DPrincipledMaterial::DirtyType type)
{
    if (!(m_dirtyAttributes & quint32(type))) {
        m_dirtyAttributes |= quint32(type);
        update();
    }
}

QT_END_NAMESPACE
