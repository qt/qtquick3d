// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dlightmapper_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Lightmapper
    \inherits Object
    \inqmlmodule QtQuick3D
    \brief Specifies lightmap baking settings for a scene.
    \since 6.4

    Used when baking direct and indirect lighting. These settings are not
    relevant at other times, such as when using already generated lightmaps to
    render a scene.

    \note As of Qt 6.4, lightmap baking is in an early technical preview state.
    Changes to features, quality, and API are likely to happen in future releases.

    The Lightmapper object works in combination with:

    \list
    \li \l Model::bakedLightmap and the associated \l BakedLightmap,
    \li \l Model::usedInBakedLighting and \l Model::lightmapBaseResolution,
    \li \l Light::bakeMode,
    \li the engine's built-in lightmap baker.
    \endlist

    \sa {Lightmaps and Global Illumination}, {Qt Quick 3D - Baked Lightmap Example}
 */

/*!
    \qmlproperty float Lightmapper::opacityThreshold

    The opacity (alpha) threshold below which an object is ignored in ray -
    mesh intersections when calculating lighting via raytracing. When the
    opacity falls below the threshold, the model (submesh) will not occlude
    lights and thus will not generate shadows either.

    The default value is 0.5.

    \note The lightmapper takes the \l{PrincipledMaterial::opacity}{material's
    opacity} and the \l{PrincipledMaterial::baseColor}{baseColor alpha}
    combined with the \l{PrincipledMaterial::baseColorMap}{base color map's
    alpha} into account. Other sources of semi-transparency, such as the
    opacity map or alpha cut-off settings are ignored during the lightmap
    baking process.
 */

/*!
    \qmlproperty float Lightmapper::bias

    Raycasting bias used during baking. Adapt the value in case artifacts
    occur, for example in order to reduce undesired shadowing patterns. In many
    cases the default value is sufficient.

    The default value is 0.005.
 */

/*!
    \qmlproperty bool Lightmapper::adaptiveBiasEnabled

    Enables applying an additional, dynamic bias based on the surface normal.

    The default value is true.
 */

/*!
    \qmlproperty bool Lightmapper::indirectLightEnabled

    Normally there is no need to change this value. The default value is true.
    Setting this property to false disables indirect light computation during
    lightmap baking. Thus the resulting texture maps will only contain direct
    light information. At run time, the engine will continue to use the maps
    normally, assuming they contain both direct and indirect lighting.
 */

/*!
    \qmlproperty int Lightmapper::samples

    The number of samples per lightmap texel.

    The default value is 256.

    The value heavily affects both the performance and quality of the resulting
    lightmaps during lightmap baking.
 */

/*!
    \qmlproperty int Lightmapper::indirectLightWorkgroupSize

    The size of the sample workgroups. These workgroups are attempted to be
    executed in parallel. (the exact behavior depends on the number of CPU
    cores and the QThreadPool configuration)

    The default value is 32. With the default sample count of 256 this means
    attempting to run 8 groups in parallel per model.
 */

/*!
    \qmlproperty int Lightmapper::bounces

    The maximum number of indirect light bounces per sample. The value should
    at least be 1, no point in indirect light calculation otherwise.

    The default value is 3.

    The value heavily affects both the performance and quality of the resulting
    lightmaps during lightmap baking.
*/

/*!
    \qmlproperty float Lightmapper::indirectLightFactor

    Multiplier for the indirect light amount. While it is the value of 1 (i.e.,
    not affecting the indirect light amount calculation) that provides the
    strictly correct rendering results, a slightly higher value can often give
    better looking results when using the lightmap, even with a lower number of
    bounces.

    The default value is 1.
 */

float QQuick3DLightmapper::opacityThreshold() const
{
    return m_opacityThreshold;
}

float QQuick3DLightmapper::bias() const
{
    return m_bias;
}

bool QQuick3DLightmapper::isAdaptiveBiasEnabled() const
{
    return m_adaptiveBias;
}

bool QQuick3DLightmapper::isIndirectLightEnabled() const
{
    return m_indirectLight;
}

int QQuick3DLightmapper::samples() const
{
    return m_samples;
}

int QQuick3DLightmapper::indirectLightWorkgroupSize() const
{
    return m_workgroupSize;
}

int QQuick3DLightmapper::bounces() const
{
    return m_bounces;
}

float QQuick3DLightmapper::indirectLightFactor() const
{
    return m_indirectFactor;
}

void QQuick3DLightmapper::setOpacityThreshold(float opacity)
{
    if (m_opacityThreshold == opacity)
        return;

    m_opacityThreshold = opacity;
    emit opacityThresholdChanged();
    emit changed();
}

void QQuick3DLightmapper::setBias(float bias)
{
    if (m_bias == bias)
        return;

    m_bias = bias;
    emit biasChanged();
    emit changed();
}

void QQuick3DLightmapper::setAdaptiveBiasEnabled(bool enabled)
{
    if (m_adaptiveBias == enabled)
        return;

    m_adaptiveBias = enabled;
    emit adaptiveBiasEnabledChanged();
    emit changed();
}

void QQuick3DLightmapper::setIndirectLightEnabled(bool enabled)
{
    if (m_indirectLight == enabled)
        return;

    m_indirectLight = enabled;
    emit indirectLightEnabledChanged();
    emit changed();
}

void QQuick3DLightmapper::setSamples(int count)
{
    if (m_samples == count)
        return;

    m_samples = count;
    emit samplesChanged();
    emit changed();
}

void QQuick3DLightmapper::setIndirectLightWorkgroupSize(int size)
{
    if (m_workgroupSize == size)
        return;

    m_workgroupSize = size;
    emit indirectLightWorkgroupSizeChanged();
    emit changed();
}

void QQuick3DLightmapper::setBounces(int count)
{
    if (m_bounces == count)
        return;

    m_bounces = count;
    emit bouncesChanged();
    emit changed();
}

void QQuick3DLightmapper::setIndirectLightFactor(float factor)
{
    if (m_indirectFactor == factor)
        return;

    m_indirectFactor = factor;
    emit indirectLightFactorChanged();
    emit changed();
}

QT_END_NAMESPACE
