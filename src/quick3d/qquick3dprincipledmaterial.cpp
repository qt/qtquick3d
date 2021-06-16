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
#include <QtQuick3DUtils/private/qssgutils_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype PrincipledMaterial
    \inherits Material
    \inqmlmodule QtQuick3D
    \brief Lets you define a material for 3D items using the metal/roughness workflow.

    Before a Model can be rendered in a scene, it must have at least one material attached
    to it that describes how the mesh should be shaded. The PrincipledMaterial is a PBR
    metal/roughness material that aims at being an easy to use material with a minimal
    set of parameters.
    In addition to having few parameters, all input values are strictly normalized
    between 0 and 1 and have sensible defaults, meaning even without changing any values,
    the material can be used to shader a model. For an introduction on how the
    different properties of the principled material affects how a model is shaded,
    see the \l{Qt Quick 3D - Principled Material Example}{Principled Material example}.

    \section1 Metal/Roughness workflow

    The Principled material is what's known as a metal/roughness material, in essence
    that means the main characteristics of the material is controlled through
    the \l {PrincipledMaterial::metalnessMap} {metallness}, \l {PrincipledMaterial::roughnessMap} {roughness},
    and the \l {PrincipledMaterial::baseColorMap} {base color} property.

    \section2 Metalness

    Real world materials are put into two main categories, metals and dielectrics (non-metals).
    In the Principled material, the category a material belongs to is decided by the
    \c metalness value. Setting the \c metalness value to 0, means the material is a dialectric,
    while everything above 0 is a considered to be a metal. In reality metals will have
    a \c metalness value of 1, but values between 0 and 1 are possible, and usually used
    for metals with reduced reflectance. For example, to render corrosion, or similar,
    on a material, the \c metalness of the material should be reduced to give the output
    properties more similar to a dielectric material.
    Since the \c metalness value affects the reflectance of the material it might be tempting to
    use the metalness to adjust glossiness, but consider what type of material you want
    to describe first. Increasing the \c metalness value to give a dielectric material
    a more polished look, will introduce properties that are not accurate for a dielectric
    material, so consider if it would be more appropriate to adjust, for example,
    the \c roughness value instead.

    \section2 Roughness

    The \c roughness of a material describes the condition of an object's surface.
    A low \c roughness value means the object has a smooth surface and therefore be more
    reflective then a material with a higher \c roughness value.

    \section2 Base color

    The \l {PrincipledMaterial::baseColorMap} {base color} of a metal/roughness material
    contains both the diffuse and the specular data, how much the base color is interpreted
    as one or the other is primarily dictated by the \c metalness value. For example,
    a material with a metalness value of 1, will have most of its base color interpreted
    as specular color, while the diffuse color would be a black tint. The opposite would
    happen for a material with a metalness value of 0. This is of course a bit simplified,
    but gives a rough idea how the \l {PrincipledMaterial::baseColor} {base color} and
    \c metalness value interacts. For those more familiar with a Specular/Glossiness workflow,
    there's a clear difference here which is worth noting, namely that the color data of the
    two materials are not directly compatible, since in a Specular/Glossiness
    \l {DefaultMaterial} {material}, the diffuse and specular color comes from separate inputs.
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
    used that you do not need to be shaded by lighting. All other shading
    properties except baseColor values, alpha values, and vertex colors will be
    ignored.

    \value PrincipledMaterial.NoLighting
    \value PrincipledMaterial.FragmentLighting
*/

/*!
    \qmlproperty enumeration PrincipledMaterial::blendMode

    This property determines how the colors of the model rendered blends with
    those behind it.

    \value PrincipledMaterial.SourceOver Default blend mode. Opaque objects
    occlude objects behind them. This default mode does not guarantee alpha
    blending in the rendering pipeline on its own for models that use this
    material, but rather makes the decision dependent on a number of factors:
    if the object's and material's total opacity is \c{1.0}, there is no
    opacity map in the material, and \l alphaMode is not set to a value that
    enforces alpha blending, then the model is treated as opaque, meaning it is
    rendered with depth testing and depth write enabled, together with other
    opaque objects, with blending disabled. Otherwise the model is treated as
    semi-transparent, and is rendered after the opaque objects, together with
    other semi-transparent objects in a back-to-front order based on their
    center's distance from the camera, with alpha blending enabled.

    \value PrincipledMaterial.Screen Colors are blended using an inverted
    multiply, producing a lighter result. This blend mode is order-independent;
    if you are using semi-opaque objects and experiencing 'popping' as faces or
    models sort differently, using Screen blending is one way to produce
    results without popping.

    \value PrincipledMaterial.Multiply Colors are blended using a multiply,
    producing a darker result. This blend mode is also order-independent.

    \sa alphaMode, {Qt Quick 3D Architecture}
*/

/*!
    \qmlproperty color PrincipledMaterial::baseColor

    This property sets the base color for the material. Depending on the type
    of material specified (metal or dielectric) the diffuse and specular channels will be
    set appropriately. For example, a dielectric material will have a diffuse color equal to
    the base color, while it's specular color, depending on the specular amount, will have a
    bright specular color. For metals the diffuse and specular channels will be mixed from
    the base color and have a dark diffuse channel and a specular channel close to the base color.

    \sa baseColorMap, alphaMode
*/

/*!
    \qmlproperty Texture PrincipledMaterial::baseColorMap

    This property defines the texture used to set the base color of the material.

    \sa baseColor, alphaMode
*/

/*!
    \qmlproperty real PrincipledMaterial::metalness

    The metalness property defines the \e metalness of the the material. The value
    is normalized, where 0.0 means the material is a \e dielectric (non-metallic) material and
    a value of 1.0 means the material is a metal.

    \note In principle, materials are either dielectrics with a metalness of 0, or metals with a
    metalness of 1. Metalness values between 0 and 1 are still allowed and will give a material that
    is a blend between the different models.

    The range is [0.0, 1.0]. The default value is 0.
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

    This property sets a RGB Texture to be used to specify the intensity of the
    emissive color.
*/

/*!
    \qmlproperty vector3d PrincipledMaterial::emissiveFactor

    This property determines the color of self-illumination for this material.
    If an emissive map is set, the x, y, and z components are used as factors
    (multipliers) for the R, G and B channels of the texture, respectively.
    The default value is (0, 0, 0) and it means no emissive contribution at all.

    \note Setting the lightingMode to DefaultMaterial.NoLighting means emissive
    Factor does not have an effect on the scene.
*/

/*!
    \qmlproperty Texture PrincipledMaterial::specularReflectionMap

    This property sets a Texture used for specular highlights on the material.

    This is typically used to perform environment mapping: as the model is
    rotated, the map will appear as though it is reflecting from the
    environment. For this to work as expected, the Texture's
    \l{Texture::mappingMode}{mappingMode} needs to be set to
    Texture.Environment. Specular reflection maps are an easy way to add a
    high-quality look with a relatively low cost.

    \note Associating a \l{SceneEnvironment::lightProbe}{light probe} with the
    \l SceneEnvironment, and thus relying on image-based lighting, can achieve
    similar environmental reflection effects. Light probes are however a
    conceptually different, and when it comes to performance, potentially more
    expensive solution. Each approaches have their own specific uses, and the
    one to use needs to be decided on a case by case basis. When it comes to
    the Texture set to the property, specularReflectionMap has an advantage,
    because it presents no limitations and supports all types of textures,
    including ones that source their data from a Qt Quick sub-scene via
    \l{Texture::sourceItem}{sourceItem}.

    \note Crisp images cause your material to look very glossy; the more you
    blur your image the softer your material will appear.

    \sa Texture::mappingMode
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
    \qmlproperty real PrincipledMaterial::specularAmount

    This property controls the strength of specularity (highlights and
    reflections).

    The range is [0.0, 1.0]. The default value is \c 0.5.

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
    The range is [0.0, 1.0]. The default value is 0.
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

    This property specifies how the alpha color value from \l baseColor and the
    alpha channel of a \l{baseColorMap}{base color map} are used.

    \note The alpha cutoff test only considers the base color alpha. \l opacity
    and \l [QtQuick3D] {Node::opacity} are not taken into account there.

    \note When sampling a base color map, the effective alpha value is the
    sampled alpha multiplied by the \l baseColor alpha.

    \value PrincipledMaterial.Default No test is applied, the effective alpha
    value is passed on as-is. Note that a \l baseColor or \l baseColorMap alpha
    less than \c 1.0 does not automatically imply alpha blending, the object
    with the material may still be treated as opaque, if no other relevant
    properties (such as, an opacity less than 1, the presence of an opacity
    map, or a non-default \l blendMode value) trigger treating the object as
    semi-transparent. To ensure alpha blending happens regardless of any other
    object or material property, set \c Blend instead.

    \value PrincipledMaterial.Blend No cutoff test is applied, but guarantees
    that alpha blending happens. The object with this material will therefore
    never be treated as opaque by the renderer.

    \value PrincipledMaterial.Opaque No cutoff test is applied and the rendered
    object is assumed to be fully opaque, meaning the alpha values in the
    vertex color, base color, and base color map are ignored and a value of 1.0
    is substituted instead. This mode does not guarantee alpha blending does
    not happen. If relevant properties (such as, an opacity less than 1, an
    opacity map, or a non-default \l blendMode) say so, then the object will
    still be treated as semi-transparent by the rendering pipeline, just like
    with the \c Default alphaMode.

    \value PrincipledMaterial.Mask A test based on \l alphaCutoff is applied.
    If the effective alpha value falls below \l alphaCutoff, the fragment is
    changed to fully transparent and is discarded (with all implications of
    discarding: the depth buffer is not written for that fragment). Otherwise
    the alpha is changed to 1.0, so that the fragment will become fully opaque.
    When it comes to alpha blending, the behavior of this mode is identical to
    \c Opaque, regardless of the cutoff test's result. This means that the
    \l{https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#alpha-coverage}{glTF
    2 spec's alpha coverage} Implementation Notes are fulfilled. Objects with
    alpha cutoff tests can also cast shadows since they behave like opaque
    objects by default, unless the relevant properties (such as, an opacity
    less than 1, an opacity map, or a non-default \l blendMode) imply otherwise
    (in which case casting shadows will not be possible).

    \sa alphaCutoff, blendMode
*/

/*!
    \qmlproperty real PrincipledMaterial::alphaCutoff

    The alphaCutoff property can be used to specify the cutoff value when using
    the \l{alphaMode}{Mask alphaMode}. Fragments where the alpha value falls
    below the threshold will be rendered fully transparent (\c{0.0} for all
    color channels). When the alpha value is equal or greater than the cutoff
    value, the color will not be affected in any way.

    The default value is 0.5.

    \sa alphaMode
*/

/*!
    \qmlproperty real PrincipledMaterial::pointSize

    This property determines the size of the points rendered, when the geometry
    is using a primitive type of points. The default value is 1.0. This
    property is not relevant when rendering other types of geometry, such as,
    triangle meshes.

    \warning Point sizes other than 1 may not be supported at run time,
    depending on the underyling graphics API. For example, setting a size other
    than 1 has no effect with Direct 3D.
*/

/*!
    \qmlproperty real PrincipledMaterial::lineWidth

    This property determines the width of the lines rendered, when the geometry
    is using a primitive type of lines or line strips. The default value is
    1.0. This property is not relevant when rendering other types of geometry,
    such as, triangle meshes.

    \warning Line widths other than 1 may not be suported at run time,
    depending on the underlying graphics API. When that is the case, the
    request to change the width is ignored. For example, none of the following
    can be expected to support wide lines: Direct3D, Metal, OpenGL with core
    profile contexts.
*/

/*!
    \qmlproperty Texture PrincipledMaterial::heightMap

    This property defines a texture used to determine the height the texture
    will be displaced when rendered through the use of Parallax Mapping. Values
    are expected to be linear from 0.0 to 1.0, where 0.0 means no displacement
    and 1.0 means means maximum displacement.

*/

/*!
    \qmlproperty enumeration PrincipledMaterial::heightChannel

    This property defines the texture channel used to read the height value
    from heightMap. The default value is \c Material.R.

    \value Material.R Read value from texture R channel.
    \value Material.G Read value from texture G channel.
    \value Material.B Read value from texture B channel.
    \value Material.A Read value from texture A channel.

*/

/*!
    \qmlproperty real PrincipledMaterial::heightAmount

    This property contains the factor used to modify the values from the
    \l heightMap texture. The value should be between 0.0 to 1.0. The default
    value is 0.0 which means that height displacement will be disabled, even
    if a height map set.
*/

/*!
    \qmlproperty int PrincipledMaterial::minHeightMapSamples

    This property defines the minimum number of samples used for performing
    Parallex Occlusion Mapping using the \l heightMap. The minHeightMapSamples
    value is the number of samples of the heightMap are used when looking directly
    at a surface (when the camera view is perpendicular to the fragment).
    The default value is 8.

    The actual number of samples used for each fragment will be between
    \l minHeightMapSamples and \l maxHeightMapSamples depending on the angle of
    the camera relative to the surface being rendered.

    \note This value should only be adjusted to fine tune materials using a
    \l heightMap in the case undesired artifacts are present.
*/

/*!
    \qmlproperty int PrincipledMaterial::maxHeightMapSamples

    This property defines the maximum number of samples used for performing
    Parallex Occlusion Mapping using the \l heightMap. The maxHeightMapSamples
    value is the number of samples of the heightMap are used when looking
    parallel to a surface.
    The default value is 32.

    The actual number of samples used for each fragment will be between
    \l minHeightMapSamples and \l maxHeightMapSamples depending on the angle of
    the camera relative to the surface being rendered.

    \note This value should only be adjusted to fine tune materials using a
    \l heightMap in the case undesired artifacts are present.
*/

inline static float ensureNormalized(float val) { return qBound(0.0f, val, 1.0f); }

QQuick3DPrincipledMaterial::QQuick3DPrincipledMaterial(QQuick3DObject *parent)
    : QQuick3DMaterial(*(new QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::PrincipledMaterial)), parent)
{}

QQuick3DPrincipledMaterial::~QQuick3DPrincipledMaterial()
{
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

QVector3D QQuick3DPrincipledMaterial::emissiveFactor() const
{
    return m_emissiveFactor;
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

float QQuick3DPrincipledMaterial::pointSize() const
{
    return m_pointSize;
}

float QQuick3DPrincipledMaterial::lineWidth() const
{
    return m_lineWidth;
}

QQuick3DTexture *QQuick3DPrincipledMaterial::heightMap() const
{
    return m_heightMap;
}

QQuick3DMaterial::TextureChannelMapping QQuick3DPrincipledMaterial::heightChannel() const
{
    return m_heightChannel;
}

float QQuick3DPrincipledMaterial::heightAmount() const
{
    return m_heightAmount;
}

int QQuick3DPrincipledMaterial::minHeightMapSamples() const
{
    return m_minHeightMapSamples;
}

int QQuick3DPrincipledMaterial::maxHeightMapSamples() const
{
    return m_maxHeightMapSamples;
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

    QQuick3DObjectPrivate::updatePropertyListener(baseColorMap, m_baseColorMap, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("baseColorMap"), m_connections, [this](QQuick3DObject *n) {
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

    QQuick3DObjectPrivate::updatePropertyListener(emissiveMap, m_emissiveMap, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("emissiveMap"), m_connections, [this](QQuick3DObject *n) {
        setEmissiveMap(qobject_cast<QQuick3DTexture *>(n));
    });

    m_emissiveMap = emissiveMap;
    emit emissiveMapChanged(m_emissiveMap);
    markDirty(EmissiveDirty);
}

void QQuick3DPrincipledMaterial::setEmissiveFactor(QVector3D emissiveFactor)
{
    if (m_emissiveFactor == emissiveFactor)
        return;

    m_emissiveFactor = emissiveFactor;
    emit emissiveFactorChanged(m_emissiveFactor);
    markDirty(EmissiveDirty);
}

void QQuick3DPrincipledMaterial::setSpecularReflectionMap(QQuick3DTexture *specularReflectionMap)
{
    if (m_specularReflectionMap == specularReflectionMap)
        return;

    QQuick3DObjectPrivate::updatePropertyListener(specularReflectionMap, m_specularReflectionMap, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("specularReflectionMap"), m_connections, [this](QQuick3DObject *n) {
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

    QQuick3DObjectPrivate::updatePropertyListener(specularMap, m_specularMap, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("specularMap"), m_connections, [this](QQuick3DObject *n) {
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

    QQuick3DObjectPrivate::updatePropertyListener(roughnessMap, m_roughnessMap, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("roughnessMap"), m_connections, [this](QQuick3DObject *n) {
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

    QQuick3DObjectPrivate::updatePropertyListener(opacityMap, m_opacityMap, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("opacityMap"), m_connections, [this](QQuick3DObject *n) {
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

    QQuick3DObjectPrivate::updatePropertyListener(normalMap, m_normalMap, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("normalMap"), m_connections, [this](QQuick3DObject *n) {
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

    QQuick3DObjectPrivate::updatePropertyListener(metallicMap, m_metalnessMap, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("metalnessMap"), m_connections, [this](QQuick3DObject *n) {
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

    QQuick3DObjectPrivate::updatePropertyListener(occlusionMap, m_occlusionMap, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("occlusionMap"), m_connections, [this](QQuick3DObject *n) {
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

void QQuick3DPrincipledMaterial::setPointSize(float size)
{
    if (qFuzzyCompare(m_pointSize, size))
        return;
    m_pointSize = size;
    emit pointSizeChanged();
    markDirty(PointSizeDirty);
}

void QQuick3DPrincipledMaterial::setLineWidth(float width)
{
    if (qFuzzyCompare(m_lineWidth, width))
        return;
    m_lineWidth = width;
    emit lineWidthChanged();
    markDirty(LineWidthDirty);
}

void QQuick3DPrincipledMaterial::setHeightMap(QQuick3DTexture *heightMap)
{
    if (m_heightMap == heightMap)
        return;

    QQuick3DObjectPrivate::updatePropertyListener(heightMap, m_heightMap, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("heightMap"), m_connections, [this](QQuick3DObject *n) {
        setHeightMap(qobject_cast<QQuick3DTexture *>(n));
    });

    m_heightMap = heightMap;
    emit heightMapChanged(m_heightMap);
    markDirty(HeightDirty);
}

void QQuick3DPrincipledMaterial::setHeightChannel(QQuick3DMaterial::TextureChannelMapping channel)
{
    if (m_heightChannel == channel)
        return;

    m_heightChannel = channel;
    emit heightChannelChanged(m_heightChannel);
    markDirty(HeightDirty);
}

void QQuick3DPrincipledMaterial::setHeightAmount(float heightAmount)
{
    if (m_heightAmount == heightAmount)
        return;

    m_heightAmount = heightAmount;
    emit heightAmountChanged(m_heightAmount);
    markDirty(HeightDirty);
}

void QQuick3DPrincipledMaterial::setMinHeightMapSamples(int samples)
{
    if (m_minHeightMapSamples == samples)
        return;

    m_minHeightMapSamples = samples;
    emit minHeightMapSamplesChanged(samples);
    markDirty(HeightDirty);
}

void QQuick3DPrincipledMaterial::setMaxHeightMapSamples(int samples)
{
    if (m_maxHeightMapSamples == samples)
        return;

    m_maxHeightMapSamples = samples;
    emit maxHeightMapSamplesChanged(samples);
    markDirty(HeightDirty);
}

QSSGRenderGraphObject *QQuick3DPrincipledMaterial::updateSpatialNode(QSSGRenderGraphObject *node)
{
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

    material->specularModel = QSSGRenderDefaultMaterial::MaterialSpecularModel::KGGX;

    if (m_dirtyAttributes & LightingModeDirty)
        material->lighting = QSSGRenderDefaultMaterial::MaterialLighting(m_lighting);

    if (m_dirtyAttributes & BlendModeDirty)
        material->blendMode = QSSGRenderDefaultMaterial::MaterialBlendMode(m_blendMode);

    if (m_dirtyAttributes & BaseColorDirty) {
        if (!m_baseColorMap)
            material->colorMap = nullptr;
        else
            material->colorMap = m_baseColorMap->getRenderImage();

        material->color = color::sRGBToLinear(m_baseColor);
    }

    if (m_dirtyAttributes & EmissiveDirty) {
        if (!m_emissiveMap)
            material->emissiveMap = nullptr;
        else
            material->emissiveMap = m_emissiveMap->getRenderImage();

        material->emissiveColor = m_emissiveFactor;
    }

    material->fresnelPower = 5.0f;
    material->vertexColorsEnabled = false;

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

    }

    if (m_dirtyAttributes & SpecularDirty) {
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

    if (m_dirtyAttributes & PointSizeDirty)
        material->pointSize = m_pointSize;

    if (m_dirtyAttributes & LineWidthDirty)
        material->lineWidth = m_lineWidth;

    if (m_dirtyAttributes & HeightDirty) {
        if (!m_heightMap)
            material->heightMap = nullptr;
        else
            material->heightMap = m_heightMap->getRenderImage();
        material->heightAmount = m_heightAmount;
        material->minHeightSamples = m_minHeightMapSamples;
        material->maxHeightSamples = m_maxHeightMapSamples;
        material->heightChannel = channelMapping(m_heightChannel);
    }

    m_dirtyAttributes = 0;

    return node;
}

void QQuick3DPrincipledMaterial::itemChange(QQuick3DObject::ItemChange change, const QQuick3DObject::ItemChangeData &value)
{
    if (change == QQuick3DObject::ItemSceneChange)
        updateSceneManager(value.sceneManager);
}

void QQuick3DPrincipledMaterial::updateSceneManager(QQuick3DSceneManager *sceneManager)
{
    // Check all the resource value's scene manager, and update as necessary.
    if (sceneManager) {
        QQuick3DObjectPrivate::refSceneManager(m_baseColorMap, *sceneManager);
        QQuick3DObjectPrivate::refSceneManager(m_emissiveMap, *sceneManager);
        QQuick3DObjectPrivate::refSceneManager(m_specularReflectionMap, *sceneManager);
        QQuick3DObjectPrivate::refSceneManager(m_specularMap, *sceneManager);
        QQuick3DObjectPrivate::refSceneManager(m_roughnessMap, *sceneManager);
        QQuick3DObjectPrivate::refSceneManager(m_opacityMap, *sceneManager);
        QQuick3DObjectPrivate::refSceneManager(m_normalMap, *sceneManager);
        QQuick3DObjectPrivate::refSceneManager(m_metalnessMap, *sceneManager);
        QQuick3DObjectPrivate::refSceneManager(m_occlusionMap, *sceneManager);
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
