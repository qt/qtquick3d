// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dfog_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Fog
    \inherits Object
    \inqmlmodule QtQuick3D
    \brief Specifies fog settings for a scene.
    \since 6.5

    When the \l{QQuick3DSceneEnvironment::fog}{fog} property of a \l SceneEnvironment is
    set to a valid Fog object, the properties are used to configure the
    rendering of fog.

    For example, the following snippet enables fog using the default settings:

    \qml
        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: theFog.color
            fog: Fog {
                id: theFog
                enabled: true
                depthEnabled: true
            }
        }
    \endqml

    \sa {Qt Quick 3D - Simple Fog Example}
 */

/*!
    \qmlproperty bool Fog::enabled

    Controls whether fog is applied to the scene. The default value is false.
 */

bool QQuick3DFog::isEnabled() const
{
    return m_enabled;
}

void QQuick3DFog::setEnabled(bool newEnabled)
{
    if (m_enabled == newEnabled)
        return;

    m_enabled = newEnabled;
    emit enabledChanged();
    emit changed();
}

/*!
    \qmlproperty color Fog::color

    The color of the fog. The default value is "#8099b3"
 */

QColor QQuick3DFog::color() const
{
    return m_color;
}

void QQuick3DFog::setColor(const QColor &newColor)
{
    if (m_color == newColor)
        return;

    m_color = newColor;
    emit colorChanged();
    emit changed();
}

/*!
    \qmlproperty float Fog::density

    Controls the fog amount, in practice this is a multiplier in range 0-1. The
    default value is 1.0. Reducing the value decreases the strength of the fog
    effect. Applicable only when depthEnabled is set to true.
 */

float QQuick3DFog::density() const
{
    return m_density;
}

void QQuick3DFog::setDensity(float newDensity)
{
    if (qFuzzyCompare(m_density, newDensity))
        return;

    m_density = newDensity;
    emit densityChanged();
    emit changed();
}

/*!
    \qmlproperty bool Fog::depthEnabled

    Controls if the fog appears in the distance. The default value is false.
 */

bool QQuick3DFog::isDepthEnabled() const
{
    return m_depthEnabled;
}

void QQuick3DFog::setDepthEnabled(bool newDepthEnabled)
{
    if (m_depthEnabled == newDepthEnabled)
        return;

    m_depthEnabled = newDepthEnabled;
    emit depthEnabledChanged();
    emit changed();
}

/*!
    \qmlproperty float Fog::depthNear

    Starting distance from the camera. The default value is 10.0. Applicable
    only when depthEnabled is set to true.
 */

float QQuick3DFog::depthNear() const
{
    return m_depthNear;
}

void QQuick3DFog::setDepthNear(float newDepthNear)
{
    if (qFuzzyCompare(m_depthNear, newDepthNear))
        return;

    m_depthNear = newDepthNear;
    emit depthNearChanged();
    emit changed();
}

/*!
    \qmlproperty float Fog::depthFar

    Ending distance from the camera. The default value is 1000.0. Applicable
    only when depthEnabled is set to true.
 */

float QQuick3DFog::depthFar() const
{
    return m_depthFar;
}

void QQuick3DFog::setDepthFar(float newDepthFar)
{
    if (qFuzzyCompare(m_depthFar, newDepthFar))
        return;

    m_depthFar = newDepthFar;
    emit depthFarChanged();
    emit changed();
}

/*!
    \qmlproperty float Fog::depthCurve

    The default value is 1.0.

    Applicable only when depthEnabled is set to true.
 */

float QQuick3DFog::depthCurve() const
{
    return m_depthCurve;
}

void QQuick3DFog::setDepthCurve(float newDepthCurve)
{
    if (qFuzzyCompare(m_depthCurve, newDepthCurve))
        return;

    m_depthCurve = newDepthCurve;
    emit depthCurveChanged();
    emit changed();
}

/*!
    \qmlproperty bool Fog::heightEnabled

    Controls if a height fog is enabled. The default value is false.
 */

bool QQuick3DFog::isHeightEnabled() const
{
    return m_heightEnabled;
}

void QQuick3DFog::setHeightEnabled(bool newHeightEnabled)
{
    if (m_heightEnabled == newHeightEnabled)
        return;

    m_heightEnabled = newHeightEnabled;
    emit heightEnabledChanged();
    emit changed();
}

/*!
    \qmlproperty float Fog::leastIntenseY

    Specifies the position (Y coordinate) where the fog is the least intense.
    The default value is 10.0. Applicable only when heightEnabled is set to
    true.

    \note By default the value is larger than mostIntenseY. As long as this is
    true, the fog is rendered top to bottom. When this value is smaller than
    mostIntenseY, the fog will render bottom to top.
 */

float QQuick3DFog::leastIntenseY() const
{
    return m_leastIntenseY;
}

void QQuick3DFog::setLeastIntenseY(float newLeastIntenseY)
{
    if (qFuzzyCompare(m_leastIntenseY, newLeastIntenseY))
        return;

    m_leastIntenseY = newLeastIntenseY;
    emit leastIntenseYChanged();
    emit changed();
}

/*!
    \qmlproperty float Fog::mostIntenseY

    Specifies the position (Y coordinate) where the fog is the most intense.
    The default value is 0. Applicable only when heightEnabled is set to true.

    \note By default the value is smaller than leastIntenseY. As long as this is
    true, the fog is rendered top to bottom. When this value is larger than
    leastIntenseY, the fog will render bottom to top.
 */

float QQuick3DFog::mostIntenseY() const
{
    return m_mostIntenseY;
}

void QQuick3DFog::setMostIntenseY(float newMostIntenseY)
{
    if (qFuzzyCompare(m_mostIntenseY, newMostIntenseY))
        return;

    m_mostIntenseY = newMostIntenseY;
    emit mostIntenseYChanged();
    emit changed();
}

/*!
    \qmlproperty float Fog::heightCurve

    Specifies the intensity of the height fog. The default value is 1.0.
    Applicable only when heightEnabled is set to true.
 */

float QQuick3DFog::heightCurve() const
{
    return m_heightCurve;
}

void QQuick3DFog::setHeightCurve(float newHeightCurve)
{
    if (qFuzzyCompare(m_heightCurve, newHeightCurve))
        return;

    m_heightCurve = newHeightCurve;
    emit heightCurveChanged();
    emit changed();
}

/*!
    \qmlproperty bool Fog::transmitEnabled

    Controls if the fog has a light transmission effect. The default value is
    false.
 */

bool QQuick3DFog::isTransmitEnabled() const
{
    return m_transmitEnabled;
}

void QQuick3DFog::setTransmitEnabled(bool newTransmitEnabled)
{
    if (m_transmitEnabled == newTransmitEnabled)
        return;

    m_transmitEnabled = newTransmitEnabled;
    emit transmitEnabledChanged();
    emit changed();
}

/*!
    \qmlproperty float Fog::tansmitCurve

    Intensity of the light transmission effect. The default value is 1.0.
    Applicable only when transmitEnabled is set to true.
 */

float QQuick3DFog::transmitCurve() const
{
    return m_transmitCurve;
}

void QQuick3DFog::setTransmitCurve(float newTransmitCurve)
{
    if (qFuzzyCompare(m_transmitCurve, newTransmitCurve))
        return;

    m_transmitCurve = newTransmitCurve;
    emit transmitCurveChanged();
    emit changed();
}

QT_END_NAMESPACE
