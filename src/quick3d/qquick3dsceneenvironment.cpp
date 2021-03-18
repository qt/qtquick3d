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

#include "qquick3dsceneenvironment_p.h"
#include "qquick3dobject_p.h"
#include "qquick3dtexture_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype SceneEnvironment
    \inherits Object3D
    \inqmlmodule QtQuick3D
    \brief Lets you configure how a scene is rendered.

    SceneEnvironment defines the environment in which the scene is rendered,
    which defines how the scene gets rendered globaly.
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
    Only supported with View3D items with renderMode set to Offscreen as the
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
    When using View3D items with a renderMode other than Offscreen, MSAA can
    only be controlled on a per-window basis, it cannot be enabled or disabled
    separately for the individual View3D items.

    \note For View3D items with a \l{QtQuick3D::View3D::renderMode}{renderMode}
    other than Offscreen, multisampling can only be enabled via the
    \l{QSurfaceFormat::setSamples()}{QSurfaceFormat} of the QQuickWindow or
    QQuickView. This will then affect all content, both 2D and 3D, in that window.

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

    \value SceneEnvironment.Transparent
        The scene is cleared to be transparent. This is useful to render 3D content on top of another item.
    \value SceneEnvironment.Color
        The scene is cleared with the color specified by the clearColor property.
    \value SceneEnvironment.Skybox
        The scene will not be cleared, but instead a Skybox or Skydome will be rendered. The Skybox
        is defined using the HDRI map defined in the lightProbe property.

    The default value is \c SceneEnvironment.Color
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

    The default value is \c 0
*/
float QQuick3DSceneEnvironment::aoStrength() const
{
    return m_aoStrength;
}

/*!
    \qmlproperty float QtQuick3D::SceneEnvironment::aoDistance

    This property defines roughly how far ambient occlusion shadows spread away
    from objects. Greater distances cause increasing impact to performance.
*/
float QQuick3DSceneEnvironment::aoDistance() const
{
    return m_aoDistance;
}

/*!
    \qmlproperty float QtQuick3D::SceneEnvironment::aoSoftness

    This property how smooth the edges of the ambient occlusion shading are.
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
*/
float QQuick3DSceneEnvironment::aoBias() const
{
    return m_aoBias;
}

/*!
    \qmlproperty QtQuick3D::Texture QtQuick3D::SceneEnvironment::lightProbe

    This property defines an image (preferably a high-dynamic range image) to
    use to light the scene, either instead of or in addition to standard
    lights. If selected, note that this image will be used as the environment
    for any custom materials, instead of the environment map often associated
    with them.
*/
QQuick3DTexture *QQuick3DSceneEnvironment::lightProbe() const
{
    return m_lightProbe;
}

/*!
    \qmlproperty float QtQuick3D::SceneEnvironment::probeBrightness

    This property modifies the amount of light emitted by the light probe.
*/
float QQuick3DSceneEnvironment::probeBrightness() const
{
    return m_probeBrightness;
}

/*!
    \qmlproperty bool QtQuick3D::SceneEnvironment::fastImageBasedLightingEnabled

    When this property is enabled more shortcuts are taken to approximate
    the light contributes of the light probe at the expense of quality.
*/
bool QQuick3DSceneEnvironment::fastImageBasedLightingEnabled() const
{
    return m_fastImageBasedLightingEnabled;
}

/*!
    \qmlproperty float QtQuick3D::SceneEnvironment::probeHorizon

    This property when defined with increasing values adds darkness (black)
    to the bottom half of the environment, forcing the lighting to come
    predominantly from the top of the image (and removing specific reflections
    from the lower half).
*/
float QQuick3DSceneEnvironment::probeHorizon() const
{
    return m_probeHorizon;
}

/*!
    \qmlproperty float QtQuick3D::SceneEnvironment::probeFieldOfView

    This property defines the image source field of view for the case of using
    a camera source as the IBL probe.
*/
float QQuick3DSceneEnvironment::probeFieldOfView() const
{
    return m_probeFieldOfView;
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
    \qmlproperty bool QtQuick3D::SceneEnvironment::depthTestEnabled

    When this property is set to \c {false}, the Z-buffer is not used, the
    depth test is skipped, and all objects, including fully opaque ones, are
    rendered in one go sorted back to front.

    This is an optimization that can cause rendering errors if disabled.

    The default value is \c true.
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

    updatePropertyListener(lightProbe, m_lightProbe, QQuick3DObjectPrivate::get(this)->sceneManager, QByteArrayLiteral("lightProbe"), m_connections,
                           [this](QQuick3DObject *n) {
        setLightProbe(qobject_cast<QQuick3DTexture *>(n));
    });

    m_lightProbe = lightProbe;
    emit lightProbeChanged();
    update();
}

void QQuick3DSceneEnvironment::setProbeBrightness(float probeBrightness)
{
    if (qFuzzyCompare(m_probeBrightness, probeBrightness))
        return;

    m_probeBrightness = probeBrightness;
    emit probeBrightnessChanged();
    update();
}

void QQuick3DSceneEnvironment::setFastImageBasedLightingEnabled(bool fastImageBasedLightingEnabled)
{
    if (m_fastImageBasedLightingEnabled == fastImageBasedLightingEnabled)
        return;

    m_fastImageBasedLightingEnabled = fastImageBasedLightingEnabled;
    emit fastImageBasedLightingEnabledChanged();
    update();
}

void QQuick3DSceneEnvironment::setProbeHorizon(float probeHorizon)
{
    if (qFuzzyCompare(m_probeHorizon, probeHorizon))
        return;

    m_probeHorizon = probeHorizon;
    emit probeHorizonChanged();
    update();
}

void QQuick3DSceneEnvironment::setProbeFieldOfView(float probeFieldOfView)
{
    if (qFuzzyCompare(m_probeFieldOfView, probeFieldOfView))
        return;

    m_probeFieldOfView = probeFieldOfView;
    emit probeFieldOfViewChanged();
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

void QQuick3DSceneEnvironment::updateSceneManager(const QSharedPointer<QQuick3DSceneManager> &manager)
{
    if (manager)
        QQuick3DObjectPrivate::refSceneManager(m_lightProbe, manager);
    else
        QQuick3DObjectPrivate::derefSceneManager(m_lightProbe);
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

QQuick3DEffect *QQuick3DSceneEnvironment::qmlEffectAt(QQmlListProperty<QQuick3DEffect> *list, int index)
{
    QQuick3DSceneEnvironment *self = static_cast<QQuick3DSceneEnvironment *>(list->object);
    return self->m_effects.at(index);
}

int QQuick3DSceneEnvironment::qmlEffectsCount(QQmlListProperty<QQuick3DEffect> *list)
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

QT_END_NAMESPACE
