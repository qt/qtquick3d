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
#include "qquick3dobject_p_p.h"
#include "qquick3dtexture_p.h"

QT_BEGIN_NAMESPACE

static void updateProperyListener(QQuick3DObject *newO, QQuick3DObject *oldO, QQuick3DSceneManager *manager, QHash<QObject*, QMetaObject::Connection> &connections, std::function<void(QQuick3DObject *o)> callFn) {
    // disconnect previous destruction listern
    if (oldO) {
        if (manager)
            QQuick3DObjectPrivate::get(oldO)->derefSceneRenderer();

        auto connection = connections.find(oldO);
        if (connection != connections.end()) {
            QObject::disconnect(connection.value());
            connections.erase(connection);
        }
    }

    // listen for new map's destruction
    if (newO) {
        if (manager)
            QQuick3DObjectPrivate::get(newO)->refSceneRenderer(manager);
        auto connection = QObject::connect(newO, &QObject::destroyed, [callFn](){
            callFn(nullptr);
        });
        connections.insert(newO, connection);
    }
}

/*!
    \qmltype SceneEnvironment
    \inherits Object3D
    \instantiates QQuick3DSceneEnvironment
    \inqmlmodule QtQuick3D
    \brief Lets you configure how a scene is rendered.

    SceneEnvironment defines the environment in which the scene is rendered,
    which defines how the scene gets rendered globaly.


*/

QQuick3DSceneEnvironment::QQuick3DSceneEnvironment(QQuick3DObject *parent)
    : QQuick3DObject(parent)
{

}

QQuick3DSceneEnvironment::~QQuick3DSceneEnvironment()
{
    for (auto connection : m_connections)
        disconnect(connection);
}

/*!
    \qmlproperty enumeration QtQuick3D::SceneEnvironment::progressiveAAMode

    This property enables and sets the level of progressive antialiasing
    applied to the scene.

    When all content of the scene has stopped moving, the camera is jiggled
    very slightly between frames, and the result of each new frame is blended
    with the previous frames. The more frames you accumulate, the better
    looking the result.

    Pros: Provides wonderful detail on static images with no performance cost.

    Cons: Does not take effect if any visual changes are occurring;
    8x PAA takes one eighth of a second—to finish rendering (at 60fps),
    which may be noticeable.

    Possible Values:
    \list
    \li SceneEnvironment.NoAA
    \li SceneEnvironment.X2
    \li SceneEnvironment.X4
    \li SceneEnvironment.X8
    \endlist

    The default value is \c SceneEnvironment.NoAA
 */
QQuick3DSceneEnvironment::QQuick3DEnvironmentAAModeValues QQuick3DSceneEnvironment::progressiveAAMode() const
{
    return m_progressiveAAMode;
}

/*!
    \qmlproperty enumeration QtQuick3D::SceneEnvironment::multisampleAAMode

    This property enables and sets the level of multisample antialiasing
    applied to the scene.

    The edges of geometry are super-sampled, resulting in smoother silhouettes.
    This technique has no effect on the materials inside geometry, however.

    Pros: Good results on geometry silhouettes, where aliasing is often most
    noticeable; works with fast animation without issue.

    Cons: Can be expensive to use; does not help with texture or reflection
    issues.

    Possible Values:
    \list
    \li SceneEnvironment.NoAA
    \li SceneEnvironment.X2
    \li SceneEnvironment.X4
    \endlist

    The default value is \c SceneEnvironment.NoAA
*/

QQuick3DSceneEnvironment::QQuick3DEnvironmentAAModeValues QQuick3DSceneEnvironment::multisampleAAMode() const
{
    return m_multisampleAAMode;
}

/*!
    \qmlproperty enumeration QtQuick3D::SceneEnvironment::backgroundMode

    This property controls if and how the background of the scene should be
    cleared.

    \table
    \header \li Background Mode \li Result
    \row \li \c SceneEnvironment.Transparent \li The scene is cleared to be
    transparent.  This is useful to render 3D content on top of another item.
    \row \li \c SceneEnvironment.Color \li The scene is cleared with the color
    specified by the QtQuick3D::SceneEnvironment::clearColor property.
    \row \li \c SceneEnvironment.Skybox \li The scene will not be cleared, but
    instead a Skybox or Skydome will be rendered.  The Skybox is defined using
    the HDRI map defined in the QtQuick3D::SceneEnvironment::lightProbe
    property.
    \endtable

    The default value is \c SceneEnvironment.Color
*/

QQuick3DSceneEnvironment::QQuick3DEnvironmentBackgroundTypes QQuick3DSceneEnvironment::backgroundMode() const
{
    return m_backgroundMode;
}

/*!
    \qmlproperty color QtQuick3D::SceneEnvironment::clearColor

    This property defines which color will be used to clear the viewport when
    using \c SceneEnvironment.Color for the
    QtQuick3D::SceneEnvironment::backgroundMode property.

    The default value is \c Qt::black
*/

QColor QQuick3DSceneEnvironment::clearColor() const
{
    return m_clearColor;
}

/*!
    \qmlproperty float QtQuick3D::SceneEnvironment::aoStrength

    This property defines the amount of ambient occulusion applied. ambient
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
    your ambient occlusion, try adjusting the QtQuick3D::Camera::clipFar
    property of your QtQuick3D::Camera to be closer to your content.

    \sa QtQuick3D::Camera::clipFar

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
    be no shadowing, increase the SceneEnvironment::aoBias value slightly to
    clip away close results.
*/

float QQuick3DSceneEnvironment::aoBias() const
{
    return m_aoBias;
}

/*!
    \qmlproperty float QtQuick3D::SceneEnvironment::shadowStrength

    This property controls the strength of directional occlusion. Directional
    occlusion is a form of approximated directional shadowing. A value of 100
    causes full darkness shadows; lower values cause the shadowing to appear
    lighter. A value of 0 disables directional occlusion entirely, improving
    performance at a cost to the visual realism of 3D objects rendered in the
    scene. All values other than 0 have the same impact to the performance.

    \note Directional occlusion will only render with DefaultMaterial materials
    that have the DefaultMaterial::lighting property set to
    \c DefaultMaterial::Pixel
*/
float QQuick3DSceneEnvironment::shadowStrength() const
{
    return m_shadowStrength;
}

/*!
    \qmlproperty float QtQuick3D::SceneEnvironment::shadowDistance

    This property defines roughly how far the faked shadows spread away from
    objects.
*/
float QQuick3DSceneEnvironment::shadowDistance() const
{
    return m_shadowDistance;
}

/*!
    \qmlproperty float QtQuick3D::SceneEnvironment::shadowSoftness
    This property crossfades amount between sharp shadows and smooth gradations.
*/
float QQuick3DSceneEnvironment::shadowSoftness() const
{
    return m_shadowSoftness;
}

/*!
    \qmlproperty float QtQuick3D::SceneEnvironment::shadowBias

    This property is a cutoff distance preventing objects from self-shadowing.
    Higher values increase the distance required between objects before
    directional occlusion is seen.
*/
float QQuick3DSceneEnvironment::shadowBias() const
{
    return m_shadowBias;
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
    \qmlproperty bool QtQuick3D::SceneEnvironment::fastIBL

    When this property is enabled more shortcuts are taken to approximate
    the light contributes of the light probe at the expense of quality.
*/
bool QQuick3DSceneEnvironment::fastIBL() const
{
    return m_fastIBL;
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
    \qmlproperty QtQuick3D::Texture QtQuick3D::SceneEnvironment::lightProbe2

    This property defines a secound image (preferably a high-dynamic range
    image) to use to light the scene, either instead of or in addition to
    standard lights.

    \sa QtQuick3D::SceneEnvironment::lightProbe
*/
QQuick3DTexture *QQuick3DSceneEnvironment::lightProbe2() const
{
    return m_lightProbe2;
}

/*!
    \qmlproperty float QtQuick3D::SceneEnvironment::probe2Fade

    This property defines the blend amount between the first and second light
    probes.
*/
float QQuick3DSceneEnvironment::probe2Fade() const
{
    return m_probe2Fade;
}

/*!
    \qmlproperty float QtQuick3D::SceneEnvironment::probe2Window

    This property restricts how much of the second light probe is used.
*/
float QQuick3DSceneEnvironment::probe2Window() const
{
    return m_probe2Window;
}

/*!
    \qmlproperty float QtQuick3D::SceneEnvironment::probe2Position

    This property sets the offset of the restriction set by the
    QtQuick3D::SceneEnvironment::probe2Window property.
*/
float QQuick3DSceneEnvironment::probe2Postion() const
{
    return m_probe2Postion;
}

/*!
    \qmlproperty bool QtQuick3D::SceneEnvironment::temporalAAEnabled

    When this property is enabled temporal antialiasing will be used.

    The camera is jiggled very slightly between frames, and the result of each
    new frame is blended with the previous frame.

    Pros: Due to the jiggling camera it finds real details that were otherwise
    lost; low impact on performance.

    Cons: Fast-moving objects cause one-frame ghosting.
*/
bool QQuick3DSceneEnvironment::temporalAAEnabled() const
{
    return m_temporalAAEnabled;
}
/*!
    \qmlproperty List<QtQuick3D::Effect> QtQuick3D::SceneEnvironment::effects

    This property contains a list of post-processing effects that will be
    applied to the entire viewport. The result of each effect is fed to the
    next so the order is significant.

    \note This property currently has no effect, because post processing
    effects are still not implimented.
*/
QQmlListProperty<QQuick3DEffect> QQuick3DSceneEnvironment::effectsList()
{
    return QQmlListProperty<QQuick3DEffect>(this,
                                          nullptr,
                                          QQuick3DSceneEnvironment::qmlAppendEffect,
                                          QQuick3DSceneEnvironment::qmlEffectsCount,
                                          QQuick3DSceneEnvironment::qmlEffectAt,
                                          QQuick3DSceneEnvironment::qmlClearEffects);
}

/*!
    \qmlproperty bool QtQuick3D::SceneEnvironment::isDepthTestDisabled

    When this property is enabled, the depth test will be skipped. This is an
    optimization that can cause rendering errors if used.
*/
bool QQuick3DSceneEnvironment::isDepthTestDisabled() const
{
    return m_isDepthTestDisabled;
}
/*!
    \qmlproperty bool QtQuick3D::SceneEnvironment::isDepthPrePassDisabled

    When this property is enabled the renderer will perform the depth buffer
    writing as part of the color pass instead of doing a seperate pass that
    only writes to the depth buffer. On GPU's that uses a tiled rendering
    architecture, this should always be set to true.
*/
bool QQuick3DSceneEnvironment::isDepthPrePassDisabled() const
{
    return m_isDepthPrePassDisabled;
}

QQuick3DObject::Type QQuick3DSceneEnvironment::type() const
{
    return QQuick3DObject::SceneEnvironment;
}

void QQuick3DSceneEnvironment::setProgressiveAAMode(QQuick3DSceneEnvironment::QQuick3DEnvironmentAAModeValues progressiveAAMode)
{
    if (m_progressiveAAMode == progressiveAAMode)
        return;

    m_progressiveAAMode = progressiveAAMode;
    emit progressiveAAModeChanged(m_progressiveAAMode);
    update();
}

void QQuick3DSceneEnvironment::setMultisampleAAMode(QQuick3DSceneEnvironment::QQuick3DEnvironmentAAModeValues multisampleAAMode)
{
    if (m_multisampleAAMode == multisampleAAMode)
        return;

    m_multisampleAAMode = multisampleAAMode;
    emit multisampleAAModeChanged(m_multisampleAAMode);
    update();
}

void QQuick3DSceneEnvironment::setBackgroundMode(QQuick3DSceneEnvironment::QQuick3DEnvironmentBackgroundTypes backgroundMode)
{
    if (m_backgroundMode == backgroundMode)
        return;

    m_backgroundMode = backgroundMode;
    emit backgroundModeChanged(m_backgroundMode);
    update();
}

void QQuick3DSceneEnvironment::setClearColor(QColor clearColor)
{
    if (m_clearColor == clearColor)
        return;

    m_clearColor = clearColor;
    emit clearColorChanged(m_clearColor);
    update();
}

void QQuick3DSceneEnvironment::setAoStrength(float aoStrength)
{
    if (qFuzzyCompare(m_aoStrength, aoStrength))
        return;

    m_aoStrength = aoStrength;
    emit aoStrengthChanged(m_aoStrength);
    update();
}

void QQuick3DSceneEnvironment::setAoDistance(float aoDistance)
{
    if (qFuzzyCompare(m_aoDistance, aoDistance))
        return;

    m_aoDistance = aoDistance;
    emit aoDistanceChanged(m_aoDistance);
    update();
}

void QQuick3DSceneEnvironment::setAoSoftness(float aoSoftness)
{
    if (qFuzzyCompare(m_aoSoftness, aoSoftness))
        return;

    m_aoSoftness = aoSoftness;
    emit aoSoftnessChanged(m_aoSoftness);
    update();
}

void QQuick3DSceneEnvironment::setAoDither(bool aoDither)
{
    if (m_aoDither == aoDither)
        return;

    m_aoDither = aoDither;
    emit aoDitherChanged(m_aoDither);
    update();
}

void QQuick3DSceneEnvironment::setAoSampleRate(int aoSampleRate)
{
    if (m_aoSampleRate == aoSampleRate)
        return;

    m_aoSampleRate = aoSampleRate;
    emit aoSampleRateChanged(m_aoSampleRate);
    update();
}

void QQuick3DSceneEnvironment::setAoBias(float aoBias)
{
    if (qFuzzyCompare(m_aoBias, aoBias))
        return;

    m_aoBias = aoBias;
    emit aoBiasChanged(m_aoBias);
    update();
}

void QQuick3DSceneEnvironment::setShadowStrength(float shadowStrength)
{
    if (qFuzzyCompare(m_shadowStrength, shadowStrength))
        return;

    m_shadowStrength = shadowStrength;
    emit shadowStrengthChanged(m_shadowStrength);
    update();
}

void QQuick3DSceneEnvironment::setShadowDistance(float shadowDistance)
{
    if (qFuzzyCompare(m_shadowDistance, shadowDistance))
        return;

    m_shadowDistance = shadowDistance;
    emit shadowDistanceChanged(m_shadowDistance);
    update();
}

void QQuick3DSceneEnvironment::setShadowSoftness(float shadowSoftness)
{
    if (qFuzzyCompare(m_shadowSoftness, shadowSoftness))
        return;

    m_shadowSoftness = shadowSoftness;
    emit shadowSoftnessChanged(m_shadowSoftness);
    update();
}

void QQuick3DSceneEnvironment::setShadowBias(float shadowBias)
{
    if (qFuzzyCompare(m_shadowBias, shadowBias))
        return;

    m_shadowBias = shadowBias;
    emit shadowBiasChanged(m_shadowBias);
    update();
}

void QQuick3DSceneEnvironment::setLightProbe(QQuick3DTexture *lightProbe)
{
    if (m_lightProbe == lightProbe)
        return;

    updateProperyListener(lightProbe, m_lightProbe, sceneRenderer(), m_connections, [this](QQuick3DObject *n) {
        setLightProbe(qobject_cast<QQuick3DTexture *>(n));
    });

    m_lightProbe = lightProbe;
    emit lightProbeChanged(m_lightProbe);
    update();
}

void QQuick3DSceneEnvironment::setProbeBrightness(float probeBrightness)
{
    if (qFuzzyCompare(m_probeBrightness, probeBrightness))
        return;

    m_probeBrightness = probeBrightness;
    emit probeBrightnessChanged(m_probeBrightness);
    update();
}

void QQuick3DSceneEnvironment::setFastIBL(bool fastIBL)
{
    if (m_fastIBL == fastIBL)
        return;

    m_fastIBL = fastIBL;
    emit fastIBLChanged(m_fastIBL);
    update();
}

void QQuick3DSceneEnvironment::setProbeHorizon(float probeHorizon)
{
    if (qFuzzyCompare(m_probeHorizon, probeHorizon))
        return;

    m_probeHorizon = probeHorizon;
    emit probeHorizonChanged(m_probeHorizon);
    update();
}

void QQuick3DSceneEnvironment::setProbeFieldOfView(float probeFieldOfView)
{
    if (qFuzzyCompare(m_probeFieldOfView, probeFieldOfView))
        return;

    m_probeFieldOfView = probeFieldOfView;
    emit probeFieldOfViewChanged(m_probeFieldOfView);
    update();
}

void QQuick3DSceneEnvironment::setLightProbe2(QQuick3DTexture *lightProbe2)
{
    if (m_lightProbe2 == lightProbe2)
        return;

    updateProperyListener(lightProbe2, m_lightProbe2, sceneRenderer(), m_connections, [this](QQuick3DObject *n) {
        setLightProbe2(qobject_cast<QQuick3DTexture *>(n));
    });

    m_lightProbe2 = lightProbe2;
    emit lightProbe2Changed(m_lightProbe2);
    update();
}

void QQuick3DSceneEnvironment::setProbe2Fade(float probe2Fade)
{
    if (qFuzzyCompare(m_probe2Fade, probe2Fade))
        return;

    m_probe2Fade = probe2Fade;
    emit probe2FadeChanged(m_probe2Fade);
    update();
}

void QQuick3DSceneEnvironment::setProbe2Window(float probe2Window)
{
    if (qFuzzyCompare(m_probe2Window, probe2Window))
        return;

    m_probe2Window = probe2Window;
    emit probe2WindowChanged(m_probe2Window);
    update();
}

void QQuick3DSceneEnvironment::setProbe2Postion(float probe2Postion)
{
    if (qFuzzyCompare(m_probe2Postion, probe2Postion))
        return;

    m_probe2Postion = probe2Postion;
    emit probe2PostionChanged(m_probe2Postion);
    update();
}

void QQuick3DSceneEnvironment::setIsDepthTestDisabled(bool isDepthTestDisabled)
{
    if (m_isDepthTestDisabled == isDepthTestDisabled)
        return;

    m_isDepthTestDisabled = isDepthTestDisabled;
    emit isDepthTestDisabledChanged(m_isDepthTestDisabled);
    update();
}

void QQuick3DSceneEnvironment::setIsDepthPrePassDisabled(bool isDepthPrePassDisabled)
{
    if (m_isDepthPrePassDisabled == isDepthPrePassDisabled)
        return;

    m_isDepthPrePassDisabled = isDepthPrePassDisabled;
    emit isDepthPrePassDisabledChanged(m_isDepthPrePassDisabled);
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
        updateSceneManager(value.sceneRenderer);
}

void QQuick3DSceneEnvironment::updateSceneManager(QQuick3DSceneManager *manager)
{
    if (manager) {
        if (m_lightProbe)
            QQuick3DObjectPrivate::get(m_lightProbe)->refSceneRenderer(manager);
        if (m_lightProbe2)
            QQuick3DObjectPrivate::get(m_lightProbe2)->refSceneRenderer(manager);
    } else {
        if (m_lightProbe)
            QQuick3DObjectPrivate::get(m_lightProbe)->derefSceneRenderer();
        if (m_lightProbe2)
            QQuick3DObjectPrivate::get(m_lightProbe2)->derefSceneRenderer();
    }
}

void QQuick3DSceneEnvironment::setTemporalAAEnabled(bool temporalAAEnabled)
{
    if (m_temporalAAEnabled == temporalAAEnabled)
        return;

    m_temporalAAEnabled = temporalAAEnabled;
    emit temporalAAEnabledChanged(m_temporalAAEnabled);
    update();
}

void QQuick3DSceneEnvironment::qmlAppendEffect(QQmlListProperty<QQuick3DEffect> *list, QQuick3DEffect *effect)
{
    if (effect == nullptr)
        return;
    QQuick3DSceneEnvironment *self = static_cast<QQuick3DSceneEnvironment *>(list->object);
    self->m_effects.push_back(effect);
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
}

QT_END_NAMESPACE
