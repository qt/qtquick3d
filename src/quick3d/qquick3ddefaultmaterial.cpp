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

#include "qquick3ddefaultmaterial_p.h"
#include "qquick3dobject_p_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterial_p.h>

QT_BEGIN_NAMESPACE


/*!
    \qmltype DefaultMaterial
    \inherits Material
    \inqmlmodule QtQuick3D
    \brief Defines a Material generated depending on which properites are set.

    Before a Model can be rendered in a scene, it must have at least one
    material to define how the mesh is shaded. The DefaultMaterial is the
    easiest way to define such a material. Even if you define a
    DefaultMaterial with no properties set, a valid mesh will be rendered,
    because the mesh defines some sensible defaults.

    As you change the properties of the DefaultMaterial, behind the scenes
    new shaders are generated, and the property values are bound. The
    complexity of a shader depends on a combination of the properties that
    are set on it, and the context of the scene itself.
*/

/*!
    \qmlproperty enumeration DefaultMaterial::lighting

    This property defines which lighting method is used when generating this
    material.

    The default value is \c DefaultMaterial.FragmentLighting

    When using \c DefaultMaterial.FragmentLighting, diffuse and specular lighting is
    calculated for each rendered pixel. Certain effects (such as a Fresnel or bump map) require
    \c DefaultMaterial.FragmentLighting to work.

    When using \c DefaultMaterial.NoLighting no lighting is calculated. This
    mode is (predictably) very fast, and is quite effective when image maps are
    used that you do not need to be shaded by lighting.

    \value DefaultMaterial.NoLighting No lighting is calculated.
    \value DefaultMaterial.FragmentLighting Per-fragment lighting is calculated.
*/

/*!
    \qmlproperty enumeration DefaultMaterial::blendMode

    This property determines how the colors of the model rendered blends with
    those behind it.

    \value DefaultMaterial.SourceOver Default blend mode. Opaque objects occlude
        objects behind them.
    \value DefaultMaterial.Screen Colors are blended using an inverted multiply,
        producing a lighter result. This blend mode is order-independent; if you are
        using semi-opaque objects and experiencing 'popping' as faces or models sort
        differently, using Screen blending is one way to produce results without
        popping.
    \value DefaultMaterial.Multiply Colors are blended using a multiply,
        producing a darker result. This blend mode is also order-independent.
    \value DefaultMaterial.Overlay A mix of Multiply and Screen modes, producing
        a result with higher contrast.
    \value DefaultMaterial.ColorBurn Colors are blended by inverted division where
        the result also is inverted, producing a darker result. Darker than Multiply.
    \value DefaultMaterial.ColorDodge Colors are blended by inverted division,
        producing a lighter result. Lighter than Screen.

*/

/*!
    \qmlproperty color DefaultMaterial::diffuseColor

    This property determines the base color for the material. Set to black to
    create a purely-specular material (e.g. metals or mirrors).
 */

/*!
    \qmlproperty Texture DefaultMaterial::diffuseMap

    This property defines a Texture to apply to the material. Using Texture
    with transparency will also apply the alpha channel as an opacity map.
 */

/*!
    \qmlproperty real DefaultMaterial::emissiveFactor

    This property determines the amount of self-illumination from the material.
    In a scene with black ambient lighting a material with an emissive factor of 0
    will appear black wherever the light does not shine on it; turning the emissive
    factor to 1 will cause the material to appear as its diffuse color instead.

    \note When you want a material to not be affected by lighting, instead of
    using 100% emissiveFactor consider setting the lightingMode to
    /c DefaultMaterial.NoLighting for a performance benefit.
*/

/*!
    \qmlproperty Texture DefaultMaterial::emissiveMap

    This property sets a Texture to be used to set the emissive factor for
    different parts of the material. Using a grayscale image will not affect the
    color of the result, while using a color image will produce glowing regions
    with the color affected by the emissive map.
*/

/*!
    \qmlproperty color DefaultMaterial::emissiveColor

    This property determines the color of self-illumination for this material.
*/

/*!
    \qmlproperty Texture DefaultMaterial::specularReflectionMap

    This property sets a Texture used for specular highlights on the material.
    By default the Texture is applied using environmental mapping (not UV
    mapping): as you rotate the model the map will appear as though it is
    reflecting from the environment. Specular Reflection maps are an easy way to
    add a high-quality look with relatively low cost.

    \note Using a Light Probe in your SceneEnvironment for image-based lighting
    will automatically use that image as the specular reflection.

    \note Crisp images cause your material to look very glossy; the more you
    blur your image the softer your material will appear.
*/

/*!
    \qmlproperty Texture DefaultMaterial::specularMap

    The property defines a RGB Texture to modulate the amount and the color of
    specularity across the surface of the material. These values are multiplied
    by the specularAmount.
*/

/*!
    \qmlproperty enumeration DefaultMaterial::specularModel

    This property determines which functions are used to calculate specular
    highlights for lights in the scene.

    \value DefaultMaterial::Default Specular lighting uses default lighting model.
    \value DefaultMaterial::KGGX Specular lighting uses GGX lighting model.
    \value DefaultMaterial::KWard Specular lighting uses Ward lighting model.
*/

/*!
    \qmlproperty real DefaultMaterial::specularTint

    This property defines a color used to adjust the specular reflections.
    Use white for no effect
*/

/*!
    \qmlproperty real DefaultMaterial::indexOfRefraction

    This property controls what angles of reflections are affected by the
    fresnelPower.
*/

/*!
    \qmlproperty real DefaultMaterial::fresnelPower

    This property decreases head-on reflections (looking directly at the
    surface) while maintaining reflections seen at grazing angles.

*/

/*!
    \qmlproperty real DefaultMaterial::specularAmount

    This property controls the strength of specularity (highlights and
    reflections).

    \note This property does not affect the \l specularReflectionMap, but does
    affect the amount of reflections from a scenes SceneEnvironment::lightProbe.

    \note Unless your mesh is high resolution, you may need to use
    \c DefaultMaterial.FragmentLighting to get good specular highlights from scene
    lights.
*/

/*!
    \qmlproperty real DefaultMaterial::specularRoughness

    This property controls the size of the specular highlight generated from
    lights, and the clarity of reflections in general. Larger values increase
    the roughness, softening specular highlights and blurring reflections.
*/

/*!
    \qmlproperty Texture DefaultMaterial::roughnessMap

    This property defines a Texture to control the specular roughness of the
    material.
*/

/*!
    \qmlproperty real DefaultMaterial::opacity

    This property drops the opacity of just this matrial, separate from the
    model.
*/

/*!
    \qmlproperty Texture DefaultMaterial::opacityMap

    This property defines a Texture used to control the opacity differently for
    different parts of the material.

    \note This must be an image format with transparency for the opacity to be
     applied.
*/

/*!
    \qmlproperty Texture DefaultMaterial::bumpMap

    This property defines a a grayscale Texture to simulate fine geometry
    displacement across the surface of the material. Brighter pixels indicate
    raised regions. The amount of the effect is controlled by the
    \l bumpAmount property.

    \note bump maps will not affect the silhouette of a model. Use a
    displacementMap if this is required.

*/

/*!
    \qmlproperty real DefaultMaterial::bumpAmount

    This property controls the amount of simulated displacement for the
    \l bumpMap or \l normalMap.

*/

/*!
    \qmlproperty Texture DefaultMaterial::normalMap

    This property defines an RGB image used to simulate fine geometry
    displacement across the surface of the material. The RGB channels indicate
    XYZ normal deviations. The amount of the effect is controlled by the
    \l bumpAmount property.

    \note Normal maps will not affect the silhouette of a model. Use a
    displacementMap if this is required.
*/

/*!
    \qmlproperty Texture DefaultMaterial::translucencyMap

    This property defines a grayscale Texture controlling how much light can
    pass through the material from behind.
*/

/*!
    \qmlproperty real DefaultMaterial::translucentFalloff

    The property defines the amount of falloff for the translucency based on the
    angle of the normals of the object to the light source.
*/

/*!
    \qmlproperty real DefaultMaterial::diffuseLightWrap

    The property determines the amount of light wrap for the translucency map.
    A value of 0 will not wrap the light at all while a value of 1 will wrap
    the light all around the object.
*/

/*!
    \qmlproperty bool DefaultMaterial::vertexColorsEnabled

    When this property is enabled the material will Use vertex colors from the
    mesh. These will be multiplied by any other colors specified for the
    material.
*/

QQuick3DDefaultMaterial::QQuick3DDefaultMaterial()
    : m_diffuseColor(Qt::white)
    , m_emissiveColor(Qt::white)
    , m_specularTint(Qt::white)
{}

QQuick3DDefaultMaterial::~QQuick3DDefaultMaterial()
{
    for(auto connection : m_connections.values())
        disconnect(connection);
}

static void updatePropertyListener(QQuick3DObject *newO, QQuick3DObject *oldO, QQuick3DSceneManager *window, QQuick3DDefaultMaterial::ConnectionMap &connections, std::function<void(QQuick3DObject *o)> callFn) {
    // disconnect previous destruction listern
    if (oldO) {
        if (window)
            QQuick3DObjectPrivate::get(oldO)->derefSceneManager();

        auto connection = connections.find(oldO);
        if (connection != connections.end()) {
            QObject::disconnect(connection.value());
            connections.erase(connection);
        }
    }

    // listen for new map's destruction
    if (newO) {
        if (window)
            QQuick3DObjectPrivate::get(newO)->refSceneManager(window);
        auto connection = QObject::connect(newO, &QObject::destroyed, [callFn](){
            callFn(nullptr);
        });
        connections.insert(newO, connection);
    }
}

QQuick3DObject::Type QQuick3DDefaultMaterial::type() const
{
    return QQuick3DObject::DefaultMaterial;
}

QQuick3DDefaultMaterial::Lighting QQuick3DDefaultMaterial::lighting() const
{
    return m_lighting;
}

QQuick3DDefaultMaterial::BlendMode QQuick3DDefaultMaterial::blendMode() const
{
    return m_blendMode;
}

QColor QQuick3DDefaultMaterial::diffuseColor() const
{
    return m_diffuseColor;
}

QQuick3DTexture *QQuick3DDefaultMaterial::diffuseMap() const
{
    return m_diffuseMap;
}

float QQuick3DDefaultMaterial::emissiveFactor() const
{
    return m_emissiveFactor;
}

QQuick3DTexture *QQuick3DDefaultMaterial::emissiveMap() const
{
    return m_emissiveMap;
}

QColor QQuick3DDefaultMaterial::emissiveColor() const
{
    return m_emissiveColor;
}

QQuick3DTexture *QQuick3DDefaultMaterial::specularReflectionMap() const
{
    return m_specularReflectionMap;
}

QQuick3DTexture *QQuick3DDefaultMaterial::specularMap() const
{
    return m_specularMap;
}

QQuick3DDefaultMaterial::SpecularModel QQuick3DDefaultMaterial::specularModel() const
{
    return m_specularModel;
}

QColor QQuick3DDefaultMaterial::specularTint() const
{
    return m_specularTint;
}

float QQuick3DDefaultMaterial::indexOfRefraction() const
{
    return m_indexOfRefraction;
}

float QQuick3DDefaultMaterial::fresnelPower() const
{
    return m_fresnelPower;
}

float QQuick3DDefaultMaterial::specularAmount() const
{
    return m_specularAmount;
}

float QQuick3DDefaultMaterial::specularRoughness() const
{
    return m_specularRoughness;
}

QQuick3DTexture *QQuick3DDefaultMaterial::roughnessMap() const
{
    return m_roughnessMap;
}

float QQuick3DDefaultMaterial::opacity() const
{
    return m_opacity;
}

QQuick3DTexture *QQuick3DDefaultMaterial::opacityMap() const
{
    return m_opacityMap;
}

QQuick3DTexture *QQuick3DDefaultMaterial::bumpMap() const
{
    return m_bumpMap;
}

float QQuick3DDefaultMaterial::bumpAmount() const
{
    return m_bumpAmount;
}

QQuick3DTexture *QQuick3DDefaultMaterial::normalMap() const
{
    return m_normalMap;
}

QQuick3DTexture *QQuick3DDefaultMaterial::translucencyMap() const
{
    return m_translucencyMap;
}

float QQuick3DDefaultMaterial::translucentFalloff() const
{
    return m_translucentFalloff;
}

float QQuick3DDefaultMaterial::diffuseLightWrap() const
{
    return m_diffuseLightWrap;
}

bool QQuick3DDefaultMaterial::vertexColorsEnabled() const
{
    return m_vertexColorsEnabled;
}

void QQuick3DDefaultMaterial::setLighting(QQuick3DDefaultMaterial::Lighting lighting)
{
    if (m_lighting == lighting)
        return;

    m_lighting = lighting;
    emit lightingChanged(m_lighting);
    markDirty(LightingModeDirty);
}

void QQuick3DDefaultMaterial::setBlendMode(QQuick3DDefaultMaterial::BlendMode blendMode)
{
    if (m_blendMode == blendMode)
        return;

    m_blendMode = blendMode;
    emit blendModeChanged(m_blendMode);
    markDirty(BlendModeDirty);
}

void QQuick3DDefaultMaterial::setDiffuseColor(QColor diffuseColor)
{
    if (m_diffuseColor == diffuseColor)
        return;

    m_diffuseColor = diffuseColor;
    emit diffuseColorChanged(m_diffuseColor);
    markDirty(DiffuseDirty);
}

void QQuick3DDefaultMaterial::setDiffuseMap(QQuick3DTexture *diffuseMap)
{
    if (m_diffuseMap == diffuseMap)
        return;

    updatePropertyListener(diffuseMap, m_diffuseMap, sceneManager(), m_connections, [this](QQuick3DObject *n) {
        setDiffuseMap(qobject_cast<QQuick3DTexture *>(n));
    });

    m_diffuseMap = diffuseMap;
    emit diffuseMapChanged(m_diffuseMap);
    markDirty(DiffuseDirty);
}

void QQuick3DDefaultMaterial::setEmissiveFactor(float emissiveFactor)
{
    emissiveFactor = qBound(0.0f, emissiveFactor, 1.0f);
    if (qFuzzyCompare(m_emissiveFactor, emissiveFactor))
        return;

    m_emissiveFactor = emissiveFactor;
    emit emissiveFactorChanged(m_emissiveFactor);
    markDirty(EmissiveDirty);
}

void QQuick3DDefaultMaterial::setEmissiveMap(QQuick3DTexture *emissiveMap)
{
    if (m_emissiveMap == emissiveMap)
        return;


    updatePropertyListener(emissiveMap, m_emissiveMap, sceneManager(), m_connections, [this](QQuick3DObject *n) {
        setEmissiveMap(qobject_cast<QQuick3DTexture *>(n));
    });

    m_emissiveMap = emissiveMap;
    emit emissiveMapChanged(m_emissiveMap);
    markDirty(EmissiveDirty);
}

void QQuick3DDefaultMaterial::setEmissiveColor(QColor emissiveColor)
{
    if (m_emissiveColor == emissiveColor)
        return;

    m_emissiveColor = emissiveColor;
    emit emissiveColorChanged(m_emissiveColor);
    markDirty(EmissiveDirty);
}

void QQuick3DDefaultMaterial::setSpecularReflectionMap(QQuick3DTexture *specularReflectionMap)
{
    if (m_specularReflectionMap == specularReflectionMap)
        return;

    updatePropertyListener(specularReflectionMap, m_specularReflectionMap, sceneManager(), m_connections, [this](QQuick3DObject *n) {
        setSpecularReflectionMap(qobject_cast<QQuick3DTexture *>(n));
    });

    m_specularReflectionMap = specularReflectionMap;
    emit specularReflectionMapChanged(m_specularReflectionMap);
    markDirty(SpecularDirty);
}

void QQuick3DDefaultMaterial::setSpecularMap(QQuick3DTexture *specularMap)
{
    if (m_specularMap == specularMap)
        return;

    updatePropertyListener(specularMap, m_specularMap, sceneManager(), m_connections, [this](QQuick3DObject *n) {
        setSpecularMap(qobject_cast<QQuick3DTexture *>(n));
    });

    m_specularMap = specularMap;
    emit specularMapChanged(m_specularMap);
    markDirty(SpecularDirty);
}

void QQuick3DDefaultMaterial::setSpecularModel(QQuick3DDefaultMaterial::SpecularModel specularModel)
{
    if (m_specularModel == specularModel)
        return;

    m_specularModel = specularModel;
    emit specularModelChanged(m_specularModel);
    markDirty(SpecularDirty);
}

void QQuick3DDefaultMaterial::setSpecularTint(QColor specularTint)
{
    if (m_specularTint == specularTint)
        return;

    m_specularTint = specularTint;
    emit specularTintChanged(m_specularTint);
    markDirty(SpecularDirty);
}

void QQuick3DDefaultMaterial::setIndexOfRefraction(float indexOfRefraction)
{
    if (qFuzzyCompare(m_indexOfRefraction, indexOfRefraction))
        return;

    m_indexOfRefraction = indexOfRefraction;
    emit indexOfRefractionChanged(m_indexOfRefraction);
    markDirty(SpecularDirty);
}

void QQuick3DDefaultMaterial::setFresnelPower(float fresnelPower)
{
    if (qFuzzyCompare(m_fresnelPower, fresnelPower))
        return;

    m_fresnelPower = fresnelPower;
    emit fresnelPowerChanged(m_fresnelPower);
    markDirty(SpecularDirty);
}

void QQuick3DDefaultMaterial::setSpecularAmount(float specularAmount)
{
    if (qFuzzyCompare(m_specularAmount, specularAmount))
        return;

    m_specularAmount = specularAmount;
    emit specularAmountChanged(m_specularAmount);
    markDirty(SpecularDirty);
}

void QQuick3DDefaultMaterial::setSpecularRoughness(float specularRoughness)
{
    if (qFuzzyCompare(m_specularRoughness, specularRoughness))
        return;

    m_specularRoughness = specularRoughness;
    emit specularRoughnessChanged(m_specularRoughness);
    markDirty(SpecularDirty);
}

void QQuick3DDefaultMaterial::setRoughnessMap(QQuick3DTexture *roughnessMap)
{
    if (m_roughnessMap == roughnessMap)
        return;

    updatePropertyListener(roughnessMap, m_roughnessMap, sceneManager(), m_connections, [this](QQuick3DObject *n) {
        setRoughnessMap(qobject_cast<QQuick3DTexture *>(n));
    });


    m_roughnessMap = roughnessMap;
    emit roughnessMapChanged(m_roughnessMap);
    markDirty(SpecularDirty);
}

void QQuick3DDefaultMaterial::setOpacity(float opacity)
{
    if (qFuzzyCompare(m_opacity, opacity))
        return;

    if (opacity > 1.0f)
        opacity = 1.0f;

    if (opacity < 0.0f)
        opacity = 0.0f;

    m_opacity = opacity;
    emit opacityChanged(m_opacity);
    markDirty(OpacityDirty);
}

void QQuick3DDefaultMaterial::setOpacityMap(QQuick3DTexture *opacityMap)
{
    if (m_opacityMap == opacityMap)
        return;

    updatePropertyListener(opacityMap, m_opacityMap, sceneManager(), m_connections, [this](QQuick3DObject *n) {
        setOpacityMap(qobject_cast<QQuick3DTexture *>(n));
    });

    m_opacityMap = opacityMap;
    emit opacityMapChanged(m_opacityMap);
    markDirty(OpacityDirty);
}

void QQuick3DDefaultMaterial::setBumpMap(QQuick3DTexture *bumpMap)
{
    if (m_bumpMap == bumpMap)
        return;

    updatePropertyListener(bumpMap, m_bumpMap, sceneManager(), m_connections, [this](QQuick3DObject *n) {
        setBumpMap(qobject_cast<QQuick3DTexture *>(n));
    });

    m_bumpMap = bumpMap;
    emit bumpMapChanged(m_bumpMap);
    markDirty(BumpDirty);
}

void QQuick3DDefaultMaterial::setBumpAmount(float bumpAmount)
{
    if (qFuzzyCompare(m_bumpAmount, bumpAmount))
        return;

    m_bumpAmount = bumpAmount;
    emit bumpAmountChanged(m_bumpAmount);
    markDirty(BumpDirty);
}

void QQuick3DDefaultMaterial::setNormalMap(QQuick3DTexture *normalMap)
{
    if (m_normalMap == normalMap)
        return;

    updatePropertyListener(normalMap, m_normalMap, sceneManager(), m_connections, [this](QQuick3DObject *n) {
        setNormalMap(qobject_cast<QQuick3DTexture *>(n));
    });

    m_normalMap = normalMap;
    emit normalMapChanged(m_normalMap);
    markDirty(NormalDirty);
}

void QQuick3DDefaultMaterial::setTranslucencyMap(QQuick3DTexture *translucencyMap)
{
    if (m_translucencyMap == translucencyMap)
        return;

    updatePropertyListener(translucencyMap, m_translucencyMap, sceneManager(), m_connections, [this](QQuick3DObject *n) {
        setTranslucencyMap(qobject_cast<QQuick3DTexture *>(n));
    });

    m_translucencyMap = translucencyMap;
    emit translucencyMapChanged(m_translucencyMap);
    markDirty(TranslucencyDirty);
}

void QQuick3DDefaultMaterial::setTranslucentFalloff(float translucentFalloff)
{
    if (qFuzzyCompare(m_translucentFalloff, translucentFalloff))
        return;

    m_translucentFalloff = translucentFalloff;
    emit translucentFalloffChanged(m_translucentFalloff);
    markDirty(TranslucencyDirty);
}

void QQuick3DDefaultMaterial::setDiffuseLightWrap(float diffuseLightWrap)
{
    if (qFuzzyCompare(m_diffuseLightWrap, diffuseLightWrap))
        return;

    m_diffuseLightWrap = diffuseLightWrap;
    emit diffuseLightWrapChanged(m_diffuseLightWrap);
    markDirty(DiffuseDirty);
}

void QQuick3DDefaultMaterial::setVertexColorsEnabled(bool vertexColors)
{
    if (m_vertexColorsEnabled == vertexColors)
        return;

    m_vertexColorsEnabled = vertexColors;
    emit vertexColorsEnabledChanged(m_vertexColorsEnabled);
    markDirty(VertexColorsDirty);
}

QSSGRenderGraphObject *QQuick3DDefaultMaterial::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node)
        node = new QSSGRenderDefaultMaterial(QSSGRenderGraphObject::Type::DefaultMaterial);

    // Set common material properties
    QQuick3DMaterial::updateSpatialNode(node);

    QSSGRenderDefaultMaterial *material = static_cast<QSSGRenderDefaultMaterial *>(node);

    if (m_dirtyAttributes & LightingModeDirty) {
        material->lighting = QSSGRenderDefaultMaterial::MaterialLighting(m_lighting);
        // If the lighthing mode changes it affects the emissive property
        m_dirtyAttributes |= EmissiveDirty;
    }

    if (m_dirtyAttributes & BlendModeDirty)
        material->blendMode = QSSGRenderDefaultMaterial::MaterialBlendMode(m_blendMode);

    if (m_dirtyAttributes & DiffuseDirty) {
        material->color = QVector4D(m_diffuseColor.redF(), m_diffuseColor.greenF(), m_diffuseColor.blueF(), m_diffuseColor.alphaF());
        if (!m_diffuseMap)
            material->colorMap = nullptr;
        else
            material->colorMap = m_diffuseMap->getRenderImage();

        material->diffuseLightWrap = m_diffuseLightWrap;
    }

    if (m_dirtyAttributes & EmissiveDirty) {
        if (!m_emissiveMap)
            material->emissiveMap = nullptr;
        else
            material->emissiveMap = m_emissiveMap->getRenderImage();

        const float emissiveFactor = (m_lighting == NoLighting) ? 1.0f : m_emissiveFactor;
        material->emissiveColor = QVector3D(m_emissiveColor.redF(), m_emissiveColor.greenF(), m_emissiveColor.blueF()) * emissiveFactor;
    }

    if (m_dirtyAttributes & SpecularDirty) {
        if (!m_specularReflectionMap)
            material->specularReflection = nullptr;
        else
            material->specularReflection = m_specularReflectionMap->getRenderImage();

        if (!m_specularMap)
            material->specularMap = nullptr;
        else
            material->specularMap = m_specularMap->getRenderImage();

        material->specularModel = QSSGRenderDefaultMaterial::MaterialSpecularModel(m_specularModel);
        material->specularTint = QVector3D(m_specularTint.redF(), m_specularTint.greenF(), m_specularTint.blueF());
        material->ior = m_indexOfRefraction;
        material->fresnelPower = m_fresnelPower;
        material->specularAmount = m_specularAmount;
        material->specularRoughness = m_specularRoughness;

        if (!m_roughnessMap)
            material->roughnessMap = nullptr;
        else
            material->roughnessMap = m_roughnessMap->getRenderImage();
    }

    if (m_dirtyAttributes & OpacityDirty) {
        material->opacity = m_opacity;
        if (!m_opacityMap)
            material->opacityMap = nullptr;
        else
            material->opacityMap = m_opacityMap->getRenderImage();
    }

    if (m_dirtyAttributes & BumpDirty) {
        if (!m_bumpMap)
            material->bumpMap = nullptr;
        else
            material->bumpMap = m_bumpMap->getRenderImage();
        material->bumpAmount = m_bumpAmount;
    }

    if (m_dirtyAttributes & NormalDirty) {
        if (!m_normalMap)
            material->normalMap = nullptr;
        else
            material->normalMap = m_normalMap->getRenderImage();
    }

    if (m_dirtyAttributes & TranslucencyDirty) {
        if (!m_translucencyMap)
            material->translucencyMap = nullptr;
        else
            material->translucencyMap = m_translucencyMap->getRenderImage();
        material->translucentFalloff = m_translucentFalloff;
    }

    if (m_dirtyAttributes & VertexColorsDirty)
        material->vertexColorsEnabled = m_vertexColorsEnabled;

    m_dirtyAttributes = 0;

    return node;
}

void QQuick3DDefaultMaterial::itemChange(QQuick3DObject::ItemChange change, const QQuick3DObject::ItemChangeData &value)
{
    if (change == QQuick3DObject::ItemSceneChange)
        updateSceneManager(value.sceneManager);
}

void QQuick3DDefaultMaterial::updateSceneManager(QQuick3DSceneManager *sceneManager)
{
    // Check all the resource value's windows, and update as necessary
    if (sceneManager) {
        if (m_diffuseMap)
            QQuick3DObjectPrivate::get(m_diffuseMap)->refSceneManager(sceneManager);
        if (m_emissiveMap)
            QQuick3DObjectPrivate::get(m_emissiveMap)->refSceneManager(sceneManager);
        if (m_specularReflectionMap)
            QQuick3DObjectPrivate::get(m_specularReflectionMap)->refSceneManager(sceneManager);
        if (m_specularMap)
            QQuick3DObjectPrivate::get(m_specularMap)->refSceneManager(sceneManager);
        if (m_roughnessMap)
            QQuick3DObjectPrivate::get(m_roughnessMap)->refSceneManager(sceneManager);
        if (m_opacityMap)
            QQuick3DObjectPrivate::get(m_opacityMap)->refSceneManager(sceneManager);
        if (m_bumpMap)
            QQuick3DObjectPrivate::get(m_bumpMap)->refSceneManager(sceneManager);
        if (m_normalMap)
            QQuick3DObjectPrivate::get(m_normalMap)->refSceneManager(sceneManager);
        if (m_translucencyMap)
            QQuick3DObjectPrivate::get(m_translucencyMap)->refSceneManager(sceneManager);
    } else {
        if (m_diffuseMap)
            QQuick3DObjectPrivate::get(m_diffuseMap)->derefSceneManager();
        if (m_emissiveMap)
            QQuick3DObjectPrivate::get(m_emissiveMap)->derefSceneManager();
        if (m_specularReflectionMap)
            QQuick3DObjectPrivate::get(m_specularReflectionMap)->derefSceneManager();
        if (m_specularMap)
            QQuick3DObjectPrivate::get(m_specularMap)->derefSceneManager();
        if (m_roughnessMap)
            QQuick3DObjectPrivate::get(m_roughnessMap)->derefSceneManager();
        if (m_opacityMap)
            QQuick3DObjectPrivate::get(m_opacityMap)->derefSceneManager();
        if (m_bumpMap)
            QQuick3DObjectPrivate::get(m_bumpMap)->derefSceneManager();
        if (m_normalMap)
            QQuick3DObjectPrivate::get(m_normalMap)->derefSceneManager();
        if (m_translucencyMap)
            QQuick3DObjectPrivate::get(m_translucencyMap)->derefSceneManager();
    }
}

void QQuick3DDefaultMaterial::markDirty(QQuick3DDefaultMaterial::DirtyType type)
{
    if (!(m_dirtyAttributes & quint32(type))) {
        m_dirtyAttributes |= quint32(type);
        update();
    }
}

QT_END_NAMESPACE
