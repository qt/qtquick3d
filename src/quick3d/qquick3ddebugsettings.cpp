// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3ddebugsettings_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype DebugSettings
    \inherits Object
    \inqmlmodule QtQuick3D
    \brief Used to configure debug settings.

    The renderer can be configured to output many different views to facilitate
    debugging. This component is used to configure these debug views.
*/

/*!
    \qmlproperty enumeration QQuick3D::DebugSettings::materialOverride
    \since 6.5

    This property changes how all materials are rendered to only reflect a
    particular aspect of the overall rendering process. This can be used as
    a debugging tool to get a better understanding of why a material looks
    the way it does.

    The default value is \c DebugSettings.None

    \value DebugSettings.None
        Material overriding is bypassed, rendering occurs as normal.
    \value DebugSettings.BaseColor
        The BaseColor or Diffuse color of a material is passed through
        without any lighting.
    \value DebugSettings.Roughness
        The Roughness of a material is passed through as an unlit
        greyscale value.
    \value DebugSettings.Metalness
        The Metalness of a material is passed through as an unlit
        greyscale value.
    \value DebugSettings.Diffuse
        Only the diffuse contribution of the material after all lighting.
    \value DebugSettings.Specular
        Only the specular contribution of the material after all lighting.
    \value DebugSettings.ShadowOcclusion
        The Occlusion caused by shadows as a greyscale value.
    \value DebugSettings.Emission
        Only the emissive contribution of the material
    \value DebugSettings.AmbientOcclusion
        Only the Ambient Occlusion of the material
    \value DebugSettings.Normals
        The interpolated world space Normal value of the material mapped to an
        RGB color.
    \value DebugSettings.Tangents
        The interpolated world space Tangent value of the material mapped to an
        RGB color. This will only be visible if the Tangent value is used.
    \value DebugSettings.Binormals
        The interpolated world space Binormal value of the material mapped to an
        RGB color. This will only be visible if the Binormal value is used.
    \value DebugSettings.F0
        This represents the Fresnel Reflectance at 0 Degrees. This will only be
        visible for materials that calculate an F0 value.

*/


QQuick3DDebugSettings::QQuick3DDebugSettings(QObject *parent)
    : QObject(parent)
{

}

QQuick3DDebugSettings::QQuick3DMaterialOverrides QQuick3DDebugSettings::materialOverride() const
{
    return m_materialOverride;
}

void QQuick3DDebugSettings::setMaterialOverride(QQuick3DMaterialOverrides newMaterialOverride)
{
    if (m_materialOverride == newMaterialOverride)
        return;
    m_materialOverride = newMaterialOverride;
    emit materialOverrideChanged();
    update();
}

void QQuick3DDebugSettings::update()
{
    emit changed();
}

/*!
    \qmlproperty bool QQuick3D::DebugSettings::wireframeEnabled
    \since 6.5

    This property changes how all materials are rendered by changing the polygon
    fill mode to be lines instead of filled. This appears as a wireframe, but the
    shaded color will still reflect the respective materials of the meshes.

    The default value is \c false.

*/


bool QQuick3DDebugSettings::wireframeEnabled() const
{
    return m_wireframeEnabled;
}

void QQuick3DDebugSettings::setWireframeEnabled(bool newWireframeEnabled)
{
    if (m_wireframeEnabled == newWireframeEnabled)
        return;
    m_wireframeEnabled = newWireframeEnabled;
    emit wireframeEnabledChanged();
    update();
}

QT_END_NAMESPACE

