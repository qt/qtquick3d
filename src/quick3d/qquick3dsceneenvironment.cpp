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

static void updatePropertyListener(QQuick3DObject *newO, QQuick3DObject *oldO, QQuick3DSceneManager *manager, QHash<QObject*, QMetaObject::Connection> &connections, std::function<void(QQuick3DObject *o)> callFn) {
    // disconnect previous destruction listern
    if (oldO) {
        if (manager)
            QQuick3DObjectPrivate::get(oldO)->derefSceneManager();

        auto connection = connections.find(oldO);
        if (connection != connections.end()) {
            QObject::disconnect(connection.value());
            connections.erase(connection);
        }
    }

    // listen for new map's destruction
    if (newO) {
        if (manager)
            QQuick3DObjectPrivate::get(newO)->refSceneManager(manager);
        auto connection = QObject::connect(newO, &QObject::destroyed, [callFn](){
            callFn(nullptr);
        });
        connections.insert(newO, connection);
    }
}

/*!
    \qmltype SceneEnvironment
    \inherits Object3D
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

    \b Pros: Provides wonderful detail on static images with no performance cost.

    \b Cons: Does not take effect if any visual changes are occurring;
    8x PAA takes one eighth of a second—to finish rendering (at 60fps),
    which may be noticeable.

    Possible values are:
    \value SceneEnvironment.NoAA No progressive antialiasing is applied.
    \value SceneEnvironment.X2 Progressive antialiasing uses 2 frames for final image.
    \value SceneEnvironment.X4 Progressive antialiasing uses 4 frames for final image.
    \value SceneEnvironment.X8 Progressive antialiasing uses 8 frames for final image.

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

    \b Pros: Good results on geometry silhouettes, where aliasing is often most
    noticeable; works with fast animation without issue.

    \b Cons: Can be expensive to use; does not help with texture or reflection
    issues.

    Possible values are:
    \value SceneEnvironment.NoAA No multisample antialiasing is applied.
    \value SceneEnvironment.X2 Antialiasing uses 2 samples per pixel.
    \value SceneEnvironment.X4 Antialiasing uses 4 samples per pixel.

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

    \b Pros: Due to the jiggling camera it finds real details that were otherwise
    lost; low impact on performance.

    \b Cons: Fast-moving objects cause one-frame ghosting.
*/
bool QQuick3DSceneEnvironment::temporalAAEnabled() const
{
    return m_temporalAAEnabled;
}

/*!
    \qmlproperty bool QtQuick3D::SceneEnvironment::depthTestEnabled

    When this property is set to \c {false}, the depth test will be skipped.
    This is an optimization that can cause rendering errors if disabled.
*/
bool QQuick3DSceneEnvironment::depthTestEnabled() const
{
    return m_depthTestEnabled;
}
/*!
    \qmlproperty bool QtQuick3D::SceneEnvironment::depthPrePassEnabled

    When this property is set to \c {false}, the renderer will perform the depth buffer
    writing as part of the color pass instead of doing a seperate pass that
    only writes to the depth buffer. On GPU's that uses a tiled rendering
    architecture, this should always be set to false.
*/
bool QQuick3DSceneEnvironment::depthPrePassEnabled() const
{
    return m_depthPrePassEnabled;
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
    emit progressiveAAModeChanged();
    update();
}

void QQuick3DSceneEnvironment::setMultisampleAAMode(QQuick3DSceneEnvironment::QQuick3DEnvironmentAAModeValues multisampleAAMode)
{
    if (m_multisampleAAMode == multisampleAAMode)
        return;

    m_multisampleAAMode = multisampleAAMode;
    emit multisampleAAModeChanged();
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

    updatePropertyListener(lightProbe, m_lightProbe, sceneManager(), m_connections, [this](QQuick3DObject *n) {
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

void QQuick3DSceneEnvironment::updateSceneManager(QQuick3DSceneManager *manager)
{
    if (manager) {
        if (m_lightProbe)
            QQuick3DObjectPrivate::get(m_lightProbe)->refSceneManager(manager);
    } else {
        if (m_lightProbe)
            QQuick3DObjectPrivate::get(m_lightProbe)->derefSceneManager();
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

QT_END_NAMESPACE
