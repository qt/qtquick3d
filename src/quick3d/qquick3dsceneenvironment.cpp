// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dsceneenvironment_p.h"
#include "qquick3dobject_p.h"
#include "qquick3dtexture_p.h"
#include "qquick3dcubemaptexture_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype SceneEnvironment
    \inherits Object3D
    \inqmlmodule QtQuick3D
    \brief Lets you configure how a scene is rendered.

    SceneEnvironment defines a set of global properties for how a scene should be rendered.
*/

QQuick3DSceneEnvironment::QQuick3DSceneEnvironment(QQuick3DObject *parent)
    : QQuick3DObject(*(new QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::SceneEnvironment)), parent)
{

}

QQuick3DSceneEnvironment::~QQuick3DSceneEnvironment()
{
    for (const auto& connection : m_connections)
        disconnect(connection);
}

/*!
    \qmlproperty enumeration QtQuick3D::SceneEnvironment::antialiasingMode
    \since 5.15

    This property controls the antialiasing mode that is applied when rendering
    the scene.

    Possible values are:
    \value SceneEnvironment.NoAA No antialiasing is applied.
    \value SceneEnvironment.SSAA Supersample antialiasing is applied.
    \value SceneEnvironment.MSAA Multisample antialiasing is applied.
    \value SceneEnvironment.ProgressiveAA Progressive antialiasing is applied.

    The default value is \c SceneEnvironment.NoAA.

    \b Supersampling

    The scene is rendered in a higher resolution, and then scaled down to
    actual resolution.

    \b Pros: High quality. Antialiases all scene content and not just geometry
    silhouettes.

    \b Cons: Usually more expensive than MSAA. Increases video memory usage.
    Supported with View3D items with all renderMode except Inline as the
    technique implies rendering to a texture first.

    \b Multisampling

    The edges of geometry are super-sampled, resulting in smoother silhouettes.
    This technique has no effect on the materials inside geometry, however.

    \b Pros: Works with any View3D item regardless of the renderMode. Good
    results on geometry silhouettes, where aliasing is often most noticeable;
    works with fast animation without issues. Performance depends purely on the
    system's (GPU) capabilities.

    \b Cons: Does not help with texture or reflection issues. Increases video
    memory usage. Can be expensive to use on less powerful graphics hardware.
    Can be controlled on a per-window basis or for individual View3D items
    depending on the renderMode. When using Underlay/Overlay with an effect
    applied or Offscreen, MSAA can be controlled for each View3D item. On the
    other hand, using Underlay/Overlay without any effect or Inline will make
    MSAA contolled per-window.

    \note For View3D items with a \l{QtQuick3D::View3D::renderMode}{renderMode}
    other than Underlay/Overlay with effects or Offscreen, multisampling can only
    be enabled via the \l{QSurfaceFormat::setSamples()}{QSurfaceFormat} of the
    QQuickWindow or QQuickView. This will then affect all content,
    both 2D and 3D, in that window.

    \b {Progressive antialiasing}

    This property enables and sets the level of progressive antialiasing
    applied to the scene.

    When all content of the scene has stopped moving, the camera is jiggled
    very slightly between frames, and the result of each new frame is blended
    with the previous frames. The more frames you accumulate, the better
    looking the result.

    \b Pros: Provides great results when all content in the scene is standing still.

    \b Cons: Does not take effect if any visual changes are occurring.
    Expensive due to having to accumulate and blend. Increases video memory
    usage.
*/
QQuick3DSceneEnvironment::QQuick3DEnvironmentAAModeValues QQuick3DSceneEnvironment::antialiasingMode() const
{
    return m_antialiasingMode;
}

/*!
    \qmlproperty enumeration QtQuick3D::SceneEnvironment::antialiasingQuality
    \since 5.15

    This property sets the level of antialiasing applied to the scene.
    Behavior depends on used antialiasingMode. With antialiasingMode
    property set to \c NoAA this property doesn't have an effect.

    Possible values are:
    \value SceneEnvironment.Medium
    SSAA: Antialiasing uses 1.2x supersampling resolution.\br
    MSAA: Antialiasing uses 2 samples per pixel.\br
    ProgressiveAA: Antialiasing uses 2 frames for final image.
    \value SceneEnvironment.High
    SSAA: Antialiasing uses 1.5x supersampling resolution.\br
    MSAA: Antialiasing uses 4 samples per pixel.\br
    ProgressiveAA: Antialiasing uses 4 frames for final image.
    \value SceneEnvironment.VeryHigh
    SSAA: Antialiasing uses 2.0x supersampling resolution.\br
    MSAA: Antialiasing uses 8 samples per pixel.\br
    ProgressiveAA: Antialiasing uses 8 frames for final image.

    The default value is \c SceneEnvironment.High
*/

QQuick3DSceneEnvironment::QQuick3DEnvironmentAAQualityValues QQuick3DSceneEnvironment::antialiasingQuality() const
{
    return m_antialiasingQuality;
}

/*!
    \qmlproperty enumeration QtQuick3D::SceneEnvironment::backgroundMode

    This property controls if and how the background of the scene should be
    cleared.

    \note Clearing does not always happen: depending on the
    \l{QtQuick3D::View3D::renderMode}{renderMode} property the View3D may not
    perform any clearing on its own, in which case
    SceneEnvironment.Transparent and SceneEnvironment.Color have no effect.
    Only the Offscreen mode (rendering into a texture) supports all clearing
    modes. With the Underlay mode, use \l{QQuickWindow::setColor()} or
    \l[QtQuick]{Window::color}{Window.color} to control the clear color for
    the Qt Quick scene. SkyBox is handled differently, as it implies drawing
    actual geometry, so that works identically across all render modes.

    \value SceneEnvironment.Transparent
        The scene is cleared to be transparent. This is useful to render 3D content on top of another item.
        This mode has no effect when the View3D is using a renderMode of Underlay or Overlay without any
        post processing enabled.
    \value SceneEnvironment.Color
        The scene is cleared with the color specified by the clearColor property.
        This mode has no effect when the View3D is using a renderMode of Underlay or Overlay without any
        post processing enabled.
    \value SceneEnvironment.SkyBox
        The scene will not be cleared, but instead a SkyBox or Skydome will be rendered. The SkyBox
        is defined using the HDRI map defined in the lightProbe property.
    \value SceneEnvironment.SkyBoxCubeMap
        The scene will not be cleared, but instead a SkyBox or Skydome will be rendered. The SkyBox
        is defined using the cubemap defined in the skyBoxCubeMap property.

    The default value is \c SceneEnvironment.Transparent

    \sa QQuickWindow::setColor(), Window::color
*/

QQuick3DSceneEnvironment::QQuick3DEnvironmentBackgroundTypes QQuick3DSceneEnvironment::backgroundMode() const
{
    return m_backgroundMode;
}

/*!
    \qmlproperty color QtQuick3D::SceneEnvironment::clearColor

    This property defines which color will be used to clear the viewport when
    using \c SceneEnvironment.Color for the backgroundMode property.

    The default value is \c Qt::black

    \sa backgroundMode
*/

QColor QQuick3DSceneEnvironment::clearColor() const
{
    return m_clearColor;
}

/*!
    \qmlproperty float QtQuick3D::SceneEnvironment::aoStrength

    This property defines the amount of ambient occulusion applied. Ambient
    occulusion is a form of approximated global illumination which causes
    non-directional self-shadowing where objects are close together.
    A value of 100 causes full darkness shadows; lower values cause the
    shadowing to appear lighter. A value of 0 disables ambient occlusion
    entirely, improving performance at a cost to the visual realism of 3D
    objects rendered in the scene.

    All values other than 0 have the same impact to the performance.

    The default value is 0.0. The maximum value is 100.0.
*/
float QQuick3DSceneEnvironment::aoStrength() const
{
    return m_aoStrength;
}

/*!
    \qmlproperty float QtQuick3D::SceneEnvironment::aoDistance

    This property defines roughly how far ambient occlusion shadows spread away
    from objects. Greater distances cause increasing impact to performance.

    The default value is 5.0.
*/
float QQuick3DSceneEnvironment::aoDistance() const
{
    return m_aoDistance;
}

/*!
    \qmlproperty float QtQuick3D::SceneEnvironment::aoSoftness

    This property how smooth the edges of the ambient occlusion shading are.

    The value must be between 0.0 and 50.0. The default value is 50.0.
*/
float QQuick3DSceneEnvironment::aoSoftness() const
{
    return m_aoSoftness;
}

/*!
    \qmlproperty bool QtQuick3D::SceneEnvironment::aoDither

    When this property is enabled it scatters the edges of the ambient
    occlusion shadow bands to improve smoothness (at the risk of sometimes
    producing obvious patterned artifacts).

    \note Very large distances between the clipping planes of your camera may
    cause problems with ambient occlusion. If you are seeing odd banding in
    your ambient occlusion, try adjusting the \l {PerspectiveCamera::}{clipFar}
    property of your camera to be closer to your content.

    The default value is \c false.

    \sa {QtQuick3D::PerspectiveCamera::clipFar}{PerspectiveCamera.clipFar},
        {QtQuick3D::OrthographicCamera::clipFar}{OrthographicCamera.clipFar}
*/
bool QQuick3DSceneEnvironment::aoDither() const
{
    return m_aoDither;
}

/*!
    \qmlproperty int QtQuick3D::SceneEnvironment::aoSampleRate

    This property defines ambient occlusion quality (more shades of gray) at
    the expense of performance.

    The value must be 2, 3, or 4. The default value is 2.
*/
int QQuick3DSceneEnvironment::aoSampleRate() const
{
    return m_aoSampleRate;
}

/*!
    \qmlproperty float QtQuick3D::SceneEnvironment::aoBias

    This property defines a cutoff distance preventing objects from exhibiting
    ambient occlusion at close distances. Higher values increase the distance
    required between objects before ambient occlusion is seen.

    \note If you see ambient occlusion shadowing on objects where there should
    be no shadowing, increase the value slightly to clip away close results.

    The default value is 0.0.
*/
float QQuick3DSceneEnvironment::aoBias() const
{
    return m_aoBias;
}

/*!
    \qmlproperty QtQuick3D::Texture QtQuick3D::SceneEnvironment::lightProbe

    This property defines an image used to light the scene, either instead of,
    or in addition to standard lights.

    The image is preferably a high-dynamic range image or a \l{Pre-generating
    IBL cubemap}{pre-generated cubemap}. Pre-baking provides significant
    performance improvements at run time, because no time is spent on filtering
    and mipmap generation. If the source is a .hdr or other image, the GPU-based
    pre-processing happens at run time after loading the image file, and that
    can be potentially time consuming, in particular on embedded and mobile
    hardware. Therefore, it is strongly recommended that applications
    pre-process .hdr images at latest at build time, as described
    \l{Pre-generating IBL cubemap}{here}.

    \note Using a Texture with \l{Texture::sourceItem}{sourceItem} is not
    supported in combination with this property. Pre-filtering of all mip
    levels for dynamic Qt Quick content is typically not reasonable in practice
    due to performance implications.
*/
QQuick3DTexture *QQuick3DSceneEnvironment::lightProbe() const
{
    return m_lightProbe;
}

/*!
    \qmlproperty float QtQuick3D::SceneEnvironment::probeExposure

    This property modifies the amount of light emitted by the light probe. Part
    of the tonemapping is exposure mapping, and this property adjusts how
    the light values in the light probes get tonemaped.

    By default exposure is set to is 1.0

    \note This property does not have an effect when \l tonemapMode is set to
    \c SceneEnvironment.TonemapModeNone.
*/
float QQuick3DSceneEnvironment::probeExposure() const
{
    return m_probeExposure;
}

/*!
    \qmlproperty float QtQuick3D::SceneEnvironment::probeHorizon

    This property when defined with increasing values adds darkness (black)
    to the bottom half of the environment, forcing the lighting to come
    predominantly from the top of the image (and removing specific reflections
    from the lower half). This property is useful for accounting for a ground
    plane that would have the effect of obscuring the reflection of the light
    probe from the ground. This is necessary because light probe contributions
    come directily from the image without consideration for the content of the
    scene.

    The expected value range for the probeHorizon property is between 0.0
    and 1.0. Any value outside of this range will be clamped to the
    expected range.

    By default probeHorizon is set to 0.0 which means the whole light probe
    is used without adjustment.

    \note The probeHorizon property only affects materials lighting, and has
    no effect on the rendering of the sky box.
*/
float QQuick3DSceneEnvironment::probeHorizon() const
{
    return m_probeHorizon;
}

/*!
    \qmlproperty vector3d QtQuick3D::SceneEnvironment::probeOrientation

    This property when defines the orientation of the light probe. Orientation
    is defined in terms of euler angles in degrees over the x, y, and z axes.

    \note This value augments how the lightProbe Texture is sampled in combination
    with any texture rotations and offsets set on the lightProbe texture.
*/
QVector3D QQuick3DSceneEnvironment::probeOrientation() const
{
    return m_probeOrientation;
}

/*!
    \qmlproperty bool QtQuick3D::SceneEnvironment::temporalAAEnabled

    When this property is enabled temporal antialiasing will be used.

    The camera is jiggled very slightly between frames, and the result of each
    new frame is blended with the previous frame.

    \note Temporal antialiasing doesn't have an effect when antialiasingMode is MSAA.
    \note When combined with ProgressiveAA antialiasingMode, temporalAA is used
    when scene animates while ProgressiveAA is used once animations stop.

    \b Pros: Due to the jiggling camera it finds real details that were otherwise
    lost; low impact on performance.

    \b Cons: Fast-moving objects cause one-frame ghosting.
*/
bool QQuick3DSceneEnvironment::temporalAAEnabled() const
{
    return m_temporalAAEnabled;
}

/*!
    \qmlproperty float QtQuick3D::SceneEnvironment::temporalAAStrength
    \since 5.15

    This property modifies the amount of temporal movement (antialiasing).
    This has an effect only when temporalAAEnabled property is true.

    \sa temporalAAEnabled
*/
float QQuick3DSceneEnvironment::temporalAAStrength() const
{
    return m_temporalAAStrength;
}

/*!
    \qmlproperty bool QtQuick3D::SceneEnvironment::specularAAEnabled
    \since 6.4

    When this property is enabled specular aliasing will be mitigated.
*/
bool QQuick3DSceneEnvironment::specularAAEnabled() const
{
    return m_specularAAEnabled;
}

/*!
    \qmlproperty bool QtQuick3D::SceneEnvironment::depthTestEnabled

    The default value is \c true. By default the renderer classifies the objects
    in the scene either as \c opaque or as \c semi-transparent. The objects
    (sub-meshes with the associated material) in the \c opaque list are rendered
    first, with depth testing and depth write enabled, providing optimal
    Z-culling for typical 3D objects that have no semi-transparent regions. The
    objects in the \c semi-transparent list are rendered with depth write
    disabled, although still with depth testing enabled (to test against the
    opaque objects), in back to front order (sorted based on their center point's
    distance from the camera). This allows correct blending ("see through") for
    3D objects that involve semi-transparent regions on their surface, either
    due to the \l{Node::opacity}{node opacity} or due to some color or texture
    map in the material.

    When this property is set to \c {false}, the Z-buffer is not written and
    tested against, the depth test is skipped, and all objects, including fully
    opaque ones, are rendered in one go, sorted back to front.

    Setting this property to \c false should be rarely needed. It can be useful
    in scenes where it is known that there is little benefit in the two-round
    approach because either there are very few opaque objects, or they are
    transformed in a way that a single back to front sorted pass performs
    better.

    \note Setting this property to \c false may cause rendering errors in
    certain scenes. In addition, some features, such as shadows, ambient
    occlusion, \c SCREEN_TEXTURE and \c DEPTH_TEXTURE in custom materials and
    effects, will not behave correctly without enabling depth buffer usage.

    \note This flag has no control over the presence of a depth or
    depth-stencil buffer. Such buffers may still be allocated even when this is
    set to \c false.
*/
bool QQuick3DSceneEnvironment::depthTestEnabled() const
{
    return m_depthTestEnabled;
}
/*!
    \qmlproperty bool QtQuick3D::SceneEnvironment::depthPrePassEnabled

    When enabled, the renderer performs a Z-prepass for opaque objects, meaning
    it renders them with a simple shader and color write disabled in order to
    get the depth buffer pre-filled before issuing draw calls for the main
    rendering passes.

    This can improve performance depending on the scene contents. It is
    typically scenes with lots of overlapping objects and expensive fragment
    shading that benefit from this. At the same time, it is worth noting that
    the renderer performs front to back sorting for opaque objects, which in
    itself helps reducing unnecessary fragment shading, and therefore the
    Z-prepass does not always bring significant improvements.

    On GPUs that use a tiled rendering architecture, which is common in mobile
    and embedded systems, it is recommended to set this to \c false.

    The default value is \c false.

    \note This property has no effect when depth testing is disabled.
*/
bool QQuick3DSceneEnvironment::depthPrePassEnabled() const
{
    return m_depthPrePassEnabled;
}

/*!
    \qmlproperty List<QtQuick3D::Effect> QtQuick3D::SceneEnvironment::effects
    \since 5.15

    This property contains a list of post-processing effects that will be
    applied to the entire viewport. The result of each effect is fed to the
    next so the order is significant.

    \note For technical reasons, adding the same \l{QtQuick3D::Effect}{Effect}
    node several times to the list is unsupported and will give unexpected results.
*/
QQmlListProperty<QQuick3DEffect> QQuick3DSceneEnvironment::effects()
{
    return QQmlListProperty<QQuick3DEffect>(this,
                                            nullptr,
                                            QQuick3DSceneEnvironment::qmlAppendEffect,
                                            QQuick3DSceneEnvironment::qmlEffectsCount,
                                            QQuick3DSceneEnvironment::qmlEffectAt,
                                            QQuick3DSceneEnvironment::qmlClearEffects);
}

/*!
    \qmlproperty enumeration QQuick3D::SceneEnvironment::tonemapMode
    \since 6.0

    This property defines how colors are tonemapped before rendering. All
    rendering in Qt Quick 3D is performed in linear color space and can in
    many cases lead to generating color values that are not displayable. The
    tonemapMode determines the technique that is used to remap colors into a
    displayable range.

    The default value is \c SceneEnvironment.TonemapModeLinear

    \value SceneEnvironment.TonemapModeNone
        All Tonemapping is bypassed. This mode is useful when performing post
        processing effects.
    \value SceneEnvironment.TonemapModeLinear
        Linear tonemapping is applied. Colors are gamma corrected and returned
        in sRGB color space.
    \value SceneEnvironment.TonemapModeAces
        Academy Color Encoding System tonemapping is applied.
    \value SceneEnvironment.TonemapModeHejlDawson
        Hejl-Dawson tonemapping is applied.
    \value SceneEnvironment.TonemapModeFilmic
        Filmic tonemapping is applied.

    \note When using post processing effects, many effects expect untonemapped
    linear color data. It is important to bypass the built-in tonemapping in
    this case by using the \c SceneEnvironment.TonemapModeNone value.

*/
QQuick3DSceneEnvironment::QQuick3DEnvironmentTonemapModes QQuick3DSceneEnvironment::tonemapMode() const
{
    return m_tonemapMode;
}

/*!
    \qmlproperty float QQuick3D::SceneEnvironment::skyboxBlurAmount
    \since 6.4

    This property determines how much much the skybox should be blurred when
    using \c SceneEnvironment.SkyBox for the
    \l{QtQuick3D::View3D::backgroundMode}{backgroundMode} property. The default
    value is \c 0.0 which means there is no blurring.

    Acceptable values range between 0.0 and 1.0, all other values will be clamped
    to this range.

*/

float QQuick3DSceneEnvironment::skyboxBlurAmount() const
{
    return m_skyboxBlurAmount;
}

void QQuick3DSceneEnvironment::setAntialiasingMode(QQuick3DSceneEnvironment::QQuick3DEnvironmentAAModeValues antialiasingMode)
{
    if (m_antialiasingMode == antialiasingMode)
        return;

    m_antialiasingMode = antialiasingMode;
    emit antialiasingModeChanged();
    update();
}

void QQuick3DSceneEnvironment::setAntialiasingQuality(QQuick3DSceneEnvironment::QQuick3DEnvironmentAAQualityValues antialiasingQuality)
{
    if (m_antialiasingQuality == antialiasingQuality)
        return;

    m_antialiasingQuality = antialiasingQuality;
    emit antialiasingQualityChanged();
    update();
}

void QQuick3DSceneEnvironment::setBackgroundMode(QQuick3DSceneEnvironment::QQuick3DEnvironmentBackgroundTypes backgroundMode)
{
    if (m_backgroundMode == backgroundMode)
        return;

    m_backgroundMode = backgroundMode;
    emit backgroundModeChanged();
    update();
}

void QQuick3DSceneEnvironment::setClearColor(const QColor &clearColor)
{
    if (m_clearColor == clearColor)
        return;

    m_clearColor = clearColor;
    emit clearColorChanged();
    update();
}

void QQuick3DSceneEnvironment::setAoStrength(float aoStrength)
{
    if (qFuzzyCompare(m_aoStrength, aoStrength))
        return;

    m_aoStrength = aoStrength;
    emit aoStrengthChanged();
    update();
}

void QQuick3DSceneEnvironment::setAoDistance(float aoDistance)
{
    if (qFuzzyCompare(m_aoDistance, aoDistance))
        return;

    m_aoDistance = aoDistance;
    emit aoDistanceChanged();
    update();
}

void QQuick3DSceneEnvironment::setAoSoftness(float aoSoftness)
{
    if (qFuzzyCompare(m_aoSoftness, aoSoftness))
        return;

    m_aoSoftness = aoSoftness;
    emit aoSoftnessChanged();
    update();
}

void QQuick3DSceneEnvironment::setAoDither(bool aoDither)
{
    if (m_aoDither == aoDither)
        return;

    m_aoDither = aoDither;
    emit aoDitherChanged();
    update();
}

void QQuick3DSceneEnvironment::setAoSampleRate(int aoSampleRate)
{
    if (m_aoSampleRate == aoSampleRate)
        return;

    m_aoSampleRate = aoSampleRate;
    emit aoSampleRateChanged();
    update();
}

void QQuick3DSceneEnvironment::setAoBias(float aoBias)
{
    if (qFuzzyCompare(m_aoBias, aoBias))
        return;

    m_aoBias = aoBias;
    emit aoBiasChanged();
    update();
}

void QQuick3DSceneEnvironment::setLightProbe(QQuick3DTexture *lightProbe)
{
    if (m_lightProbe == lightProbe)
        return;

    QQuick3DObjectPrivate::updatePropertyListener(lightProbe, m_lightProbe, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("lightProbe"), m_connections,
                           [this](QQuick3DObject *n) {
        setLightProbe(qobject_cast<QQuick3DTexture *>(n));
    });

    m_lightProbe = lightProbe;
    emit lightProbeChanged();
    update();
}

void QQuick3DSceneEnvironment::setProbeExposure(float probeExposure)
{
    if (qFuzzyCompare(m_probeExposure, probeExposure))
        return;

    m_probeExposure = probeExposure;
    emit probeExposureChanged();
    update();
}

void QQuick3DSceneEnvironment::setProbeHorizon(float probeHorizon)
{
    // clamp value to expected range
    probeHorizon = qBound(0.0f, probeHorizon, 1.0f);

    if (qFuzzyCompare(m_probeHorizon, probeHorizon))
        return;

    m_probeHorizon = probeHorizon;
    emit probeHorizonChanged();
    update();
}

void QQuick3DSceneEnvironment::setProbeOrientation(const QVector3D &orientation)
{
    if (qFuzzyCompare(m_probeOrientation, orientation))
        return;

    m_probeOrientation = orientation;
    emit probeOrientationChanged();
    update();
}

void QQuick3DSceneEnvironment::setDepthTestEnabled(bool depthTestEnabled)
{
    if (m_depthTestEnabled == depthTestEnabled)
        return;

    m_depthTestEnabled = depthTestEnabled;
    emit depthTestEnabledChanged();
    update();
}

void QQuick3DSceneEnvironment::setDepthPrePassEnabled(bool depthPrePassEnabled)
{
    if (m_depthPrePassEnabled == depthPrePassEnabled)
        return;

    m_depthPrePassEnabled = depthPrePassEnabled;
    emit depthPrePassEnabledChanged();
    update();
}

void QQuick3DSceneEnvironment::setTonemapMode(QQuick3DSceneEnvironment::QQuick3DEnvironmentTonemapModes tonemapMode)
{
    if (m_tonemapMode == tonemapMode)
        return;

    m_tonemapMode = tonemapMode;
    emit tonemapModeChanged();
    update();
}

QSSGRenderGraphObject *QQuick3DSceneEnvironment::updateSpatialNode(QSSGRenderGraphObject *node)
{
    // Don't do anything, these properties get set by the scene renderer
    return node;
}

void QQuick3DSceneEnvironment::itemChange(QQuick3DObject::ItemChange change, const QQuick3DObject::ItemChangeData &value)
{
    if (change == QQuick3DObject::ItemSceneChange)
        updateSceneManager(value.sceneManager);
}

void QQuick3DSceneEnvironment::updateSceneManager(QQuick3DSceneManager *manager)
{
    if (manager) {
        QQuick3DObjectPrivate::refSceneManager(m_lightProbe, *manager);
        QQuick3DObjectPrivate::refSceneManager(m_skyBoxCubeMap, *manager);
    } else {
        QQuick3DObjectPrivate::derefSceneManager(m_lightProbe);
        QQuick3DObjectPrivate::derefSceneManager(m_skyBoxCubeMap);
    }
}

void QQuick3DSceneEnvironment::setTemporalAAEnabled(bool temporalAAEnabled)
{
    if (m_temporalAAEnabled == temporalAAEnabled)
        return;

    m_temporalAAEnabled = temporalAAEnabled;
    emit temporalAAEnabledChanged();
    update();
}

void QQuick3DSceneEnvironment::setTemporalAAStrength(float strength)
{
    if (qFuzzyCompare(m_temporalAAStrength, strength))
        return;

    m_temporalAAStrength = strength;
    emit temporalAAStrengthChanged();
    update();
}

void QQuick3DSceneEnvironment::setSpecularAAEnabled(bool enabled)
{
    if (m_specularAAEnabled == enabled)
        return;

    m_specularAAEnabled = enabled;
    emit specularAAEnabledChanged();
    update();
}

void QQuick3DSceneEnvironment::qmlAppendEffect(QQmlListProperty<QQuick3DEffect> *list, QQuick3DEffect *effect)
{
    if (effect == nullptr)
        return;
    QQuick3DSceneEnvironment *self = static_cast<QQuick3DSceneEnvironment *>(list->object);
    self->m_effects.push_back(effect);

    if (effect->parentItem() == nullptr)
        effect->setParentItem(self);

    self->update();
}

QQuick3DEffect *QQuick3DSceneEnvironment::qmlEffectAt(QQmlListProperty<QQuick3DEffect> *list, qsizetype index)
{
    QQuick3DSceneEnvironment *self = static_cast<QQuick3DSceneEnvironment *>(list->object);
    return self->m_effects.at(index);
}

qsizetype QQuick3DSceneEnvironment::qmlEffectsCount(QQmlListProperty<QQuick3DEffect> *list)
{
    QQuick3DSceneEnvironment *self = static_cast<QQuick3DSceneEnvironment *>(list->object);
    return self->m_effects.count();
}

void QQuick3DSceneEnvironment::qmlClearEffects(QQmlListProperty<QQuick3DEffect> *list)
{
    QQuick3DSceneEnvironment *self = static_cast<QQuick3DSceneEnvironment *>(list->object);
    self->m_effects.clear();
    self->update();
}

void QQuick3DSceneEnvironment::setSkyboxBlurAmount(float newSkyboxBlurAmount)
{
    newSkyboxBlurAmount = qBound(0.0f, newSkyboxBlurAmount, 1.0f);

    if (qFuzzyCompare(m_skyboxBlurAmount, newSkyboxBlurAmount))
        return;

    m_skyboxBlurAmount = newSkyboxBlurAmount;
    emit skyboxBlurAmountChanged();
    update();
}

/*!
    \qmlproperty Lightmapper SceneEnvironment::lightmapper

    When this property is set to a valid Lightmapper object, the settings
    specified by the object will be taken into account when baking lightmaps.

    The default value is null, which means using default values for all the
    baking-related settings.

    For more information on how to bake lightmaps, see the \l Lightmapper
    documentation.

    When lightmaps are not relevant to an application and baked lighting is
    never generated, the property and the associated object serve no purpose in
    practice.

    \sa Model::usedInBakedLighting, Model::bakedLightmap, Light::bakeMode, Lightmapper
 */

QQuick3DLightmapper *QQuick3DSceneEnvironment::lightmapper() const
{
    return m_lightmapper;
}

void QQuick3DSceneEnvironment::setLightmapper(QQuick3DLightmapper *lightmapper)
{
    if (m_lightmapper == lightmapper)
        return;

    if (m_lightmapper)
        m_lightmapper->disconnect(m_lightmapperSignalConnection);

    m_lightmapper = lightmapper;

    m_lightmapperSignalConnection = QObject::connect(m_lightmapper, &QQuick3DLightmapper::changed, this,
                                                     [this] { update(); });

    QObject::connect(m_lightmapper, &QObject::destroyed, this,
                     [this]
    {
        m_lightmapper = nullptr;
        update();
    });

    emit lightmapperChanged();
    update();
}

/*!
    \qmlproperty QtQuick3D::CubeMapTexture QtQuick3D::SceneEnvironment::skyBoxCubeMap

    This property defines a cubemap to be used as a skybox when the background mode is \c SkyBoxCubeMap.

    \since 6.4
*/
QQuick3DCubeMapTexture *QQuick3DSceneEnvironment::skyBoxCubeMap() const
{
    return m_skyBoxCubeMap;
}

void QQuick3DSceneEnvironment::setSkyBoxCubeMap(QQuick3DCubeMapTexture *newSkyBoxCubeMap)
{
    if (m_skyBoxCubeMap == newSkyBoxCubeMap)
        return;


    QQuick3DObjectPrivate::updatePropertyListener(newSkyBoxCubeMap, m_skyBoxCubeMap, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("skyboxCubeMap"), m_connections,
                           [this](QQuick3DObject *n) {
        setSkyBoxCubeMap(qobject_cast<QQuick3DCubeMapTexture *>(n));
    });

    m_skyBoxCubeMap = newSkyBoxCubeMap;
    emit skyBoxCubeMapChanged();
}

QT_END_NAMESPACE
