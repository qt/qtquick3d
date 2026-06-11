// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtQuick3D/private/qquick3dskymaterial_p.h>
#include <QtQuick3D/private/qquick3dobject_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderskymaterial_p.h>

#include <QtCore/qmath.h>
#include <QtGui/qquaternion.h>

QT_BEGIN_NAMESPACE
/*!
    \qmltype SkyMaterial
    \inqmlmodule QtQuick3D
    \since 6.12
    \brief Renders a procedural sky and generates image-based lighting data.

    SkyMaterial renders a sky environment into a cubemap using custom
    fragment shader code. The resulting cubemap can optionally be filtered
    and used as image-based lighting (IBL) for physically based rendering.

    The shader is evaluated for all six faces of the cubemap and may
    implement arbitrary procedural skies, gradients, or analytical models.

    The generated cubemap is used both as the visible scene background and
    as the source for image-based lighting when enabled.

    \note Changes to shader code or properties may take a frame or more
    before being reflected in the generated IBL.

    \section1 Shader

    The shader has access to the built-in variable \c qt_eyeDir (\c vec3),
    which provides the non-normalized direction vector from the center of
    the cubemap to the current sample direction.

    The fragment shader must define a \c MAIN() entry point, which is
    executed for each fragment generated while rendering the cubemap.

    The final pixel color must be written to \c FRAGCOLOR (\c vec4), which
    is the output variable of the fragment shader.

    A minimal shader example:

    \badcode
    void MAIN()
    {
        vec3 dir = normalize(qt_eyeDir);

        // Output color (encoded direction as RGB)
        FRAGCOLOR = vec4(dir * 0.5 + 0.5, 1.0);
    }
    \endcode

    Custom QML properties declared on SkyMaterial are automatically exposed
    as uniforms in the fragment shader. The property name must match the GLSL
    variable name. No explicit uniform declaration is required.

    The following type mappings are supported:

    \table
    \header
    \li QML Type
    \li Shader Type
    \li Notes
    \row \li real, int, bool \li float, int, bool \li
    \row \li color \li vec4 \li sRGB is converted to linear space
    \row \li vector2d \li vec2 \li
    \row \li vector3d \li vec3 \li
    \row \li vector4d \li vec4 \li
    \row \li matrix4x4 \li mat4 \li
    \row \li quaternion \li vec4 \li w stores scalar component
    \row \li rect \li vec4 \li
    \row \li point, size \li vec2 \li
    \row \li TextureInput \li sampler2D \li
    \endtable
*/

/*!
    \qmlproperty int SkyMaterial::radianceMapSize
    \since 6.12
    \default 512

    Specifies the resolution of the generated environment cubemap used for
    image-based lighting and reflection probes.

    Higher values improve reflection sharpness and lighting quality,
    especially for glossy materials, but increase memory usage and
    rendering cost.

    Values are snapped to the nearest power of two and clamped to the
    range [8, 2048].

    \note With \l skyboxMode set to a \c ScreenSpace value, the visible sky is
    evaluated directly on screen and this property only affects the IBL/reflection
    cubemap. With \c SkyMaterial.Cubemap it also determines the sharpness of the
    visible background. When \l enableIBL is \c false, \l skyboxMode is a
    \c ScreenSpace value, and no reflection probes are present, no cubemap is
    generated at all.

    \sa skyboxMode, enableIBL
*/

/*!
    \qmlproperty url SkyMaterial::fragmentShader
    \since 6.12

    Specifies a fragment shader loaded from a file or resource URL.

    If both \l fragmentShader and \l fragmentShaderCode are set,
    \l fragmentShaderCode takes precedence.

    If no shader is specified, a built-in debug shader is used.

    \sa fragmentShaderCode
*/

/*!
    \qmlproperty string SkyMaterial::fragmentShaderCode
    \since 6.12

    Specifies inline fragment shader source code.

    This behaves identically to \l fragmentShader and uses the same
    execution model, built-ins, and automatic property-to-uniform mapping.

    If both \l fragmentShader and \l fragmentShaderCode are set,
    this property takes precedence.

    \sa fragmentShader
*/

/*!
    \qmlproperty bool SkyMaterial::enableIBL
    \since 6.12
    \default true

    Determines whether the generated sky cubemap is used for image-based
    lighting.

    When enabled, the engine generates filtered radiance and irradiance
    maps used for ambient lighting and reflections in physically based
    rendering.

    When disabled, the sky is only rendered as a visual background and no
    IBL preprocessing is performed.

    \sa SceneEnvironment::lightProbe
*/

/*!
    \qmlproperty int SkyMaterial::iblSampleCount
    \since 6.12
    \default 32

    The total number of GGX importance samples evaluated per output texel
    of the filtered radiance map. Higher values reduce shimmering on
    high-frequency features such as a sun disk, at the cost of generation
    time.

    Values are clamped to the range [1, 1024].

    \note This property has no effect when \l enableIBL is \c false.

    \sa iblRenderFrames, enableIBL
*/

/*!
    \qmlproperty int SkyMaterial::iblRenderFrames
    \since 6.12
    \default 0

    Controls how the IBL prefilter work is spread across frames.

    \list
    \li \b 0 — Everything in one frame: sample accumulation, normalization, and
        irradiance are all evaluated in the same frame.
    \li \b {N ≥ 1} — The \l iblSampleCount samples are divided across N slice
        frames (\c ceil(iblSampleCount / N) samples per frame), followed by a
        dedicated normalization and irradiance frame.
    \endlist

    Higher values amortize the GGX prefilter cost across more frames, which is
    useful when the sky changes infrequently and a high \l iblSampleCount is
    required for quality.

    \note This property has no effect when \l enableIBL is \c false.

    \sa iblSampleCount, enableIBL
*/

/*!
    \qmlproperty enumeration SkyMaterial::skyboxMode
    \since 6.12
    \default SkyMaterial.Cubemap

    Selects what the SkyMaterial produces for the visible background.

    \value SkyMaterial.Cubemap          The background samples the radiance
        cubemap (the same cube used for image-based lighting), whose resolution
        follows \l radianceMapSize. For a static sky the cube is rendered once and
        then sampled cheaply every frame, so this is the lowest per-frame cost when
        nothing but the camera is moving. Sharpness is capped at \l radianceMapSize.
    \value SkyMaterial.ScreenSpaceFull    The sky shader is evaluated directly on
        screen at full viewport resolution every frame (sharpest, most costly).
    \value SkyMaterial.ScreenSpaceHalf    Direct screen-space evaluation at half
        resolution per axis (quarter the pixels), upscaled to the viewport.
    \value SkyMaterial.ScreenSpaceQuarter Direct screen-space evaluation at quarter
        resolution per axis (one sixteenth the pixels), upscaled to the viewport.

    The \c ScreenSpace modes decouple the visible background from the IBL cube and
    are best for dynamic skies (e.g. moving sun, animated volumetric clouds);
    \c Cubemap is best for static skies. \c Cubemap forces the cubemap to be
    generated even when \l enableIBL is \c false.

    \sa radianceMapSize, enableIBL
*/

QQuick3DSkyMaterial::QQuick3DSkyMaterial(QQuick3DObject *parent)
    : QQuick3DObject(parent), QQuick3DPropertyChangedTracker(this, QQuick3DSuperClassInfo<QQuick3DSkyMaterial>())
{
}

int QQuick3DSkyMaterial::radianceMapSize() const
{
    return m_radianceMapSize;
}

QSSGRenderGraphObject *QQuick3DSkyMaterial::updateSpatialNode(QSSGRenderGraphObject *node)
{
    QSSGRenderSkyMaterial *material = static_cast<QSSGRenderSkyMaterial *>(node);
    if (!material) {
        material = new QSSGRenderSkyMaterial;
        material->isDirty = true;
    }

    // isDirty tracks whether the procedural cubemap content needs to be regenerated.
    // Toggling enableIBL alone reuses the same cubemap, so it isn't a content change.
    if (material->radianceMapSize != m_radianceMapSize || (m_dirtyFlag & (Dirty::FragmentShader | Dirty::TrackedProperty))) {
        material->isDirty = true;
    }

    material->radianceMapSize = m_radianceMapSize;
    material->enableIBL = m_enableIBL;
    material->iblSampleCount = m_iblSampleCount;
    // Convert user-facing "frames to converge" to the render-side per-frame sample budget.
    // 1 frame = no time-slicing (mapped to 0, the render struct's "single-frame" sentinel).
    // Otherwise ceil(sampleCount/frameCount) so accumulation always finishes within the
    // requested frame budget, even when the division isn't exact.
    material->iblSamplesPerFrame = (m_iblRenderFrames <= 1) ? 0 : (m_iblSampleCount + m_iblRenderFrames - 1) / m_iblRenderFrames;
    material->iblRenderFrames = m_iblRenderFrames;
    material->skyboxMode = static_cast<QSSGRenderSkyMaterial::SkyboxMode>(m_skyboxMode);

    if (m_dirtyFlag & Dirty::FragmentShader) {
        const QQmlContext *context = qmlContext(this);
        material->fragmentShaderSource = !m_fragmentShaderCode.isEmpty() ? m_fragmentShaderCode.toUtf8()
                : !m_fragmentShader.isEmpty() ? QSSGShaderUtils::resolveShader(m_fragmentShader, context, material->shaderPathKey)
                                              : QByteArray();
        material->isFragmentShaderDirty = true;
        material->isBackgroundShaderDirty = true;
    }

    if (m_dirtyFlag & Dirty::TrackedProperty) {
        material->propertyUniforms = extractProperties();
    }

    m_dirtyFlag = 0;

    return QQuick3DObject::updateSpatialNode(material);
}

void QQuick3DSkyMaterial::markTrackedPropertyDirty(QMetaProperty property, DirtyPropertyHint hint)
{
    Q_UNUSED(property);
    Q_UNUSED(hint);
    markDirty(Dirty::TrackedProperty);
}

void QQuick3DSkyMaterial::markDirty(Dirty v)
{
    m_dirtyFlag |= v;
    update();
}

void QQuick3DSkyMaterial::setRadianceMapSize(int radianceMapSize)
{
    // Snap to the nearest power of two, then clamp to [8, 2048]. We store the
    // normalized value so reading the property back reflects the resolution the
    // renderer will actually use.
    const int upper = qNextPowerOfTwo(qMax(1, radianceMapSize));
    const int lower = upper >> 1;
    radianceMapSize = (radianceMapSize - lower < upper - radianceMapSize) ? lower : upper;
    radianceMapSize = qBound(8, radianceMapSize, 2048);
    if (m_radianceMapSize == radianceMapSize)
        return;

    m_radianceMapSize = radianceMapSize;
    emit radianceMapSizeChanged();
    update();
}

QUrl QQuick3DSkyMaterial::fragmentShader() const
{
    return m_fragmentShader;
}

void QQuick3DSkyMaterial::setFragmentShader(const QUrl &newFragmentShader)
{
    if (m_fragmentShader == newFragmentShader)
        return;
    m_fragmentShader = newFragmentShader;
    emit fragmentShaderChanged();
    markDirty(Dirty::FragmentShader);
}

QString QQuick3DSkyMaterial::fragmentShaderCode() const
{
    return m_fragmentShaderCode;
}

void QQuick3DSkyMaterial::setFragmentShaderCode(const QString &newFragmentShaderCode)
{
    if (m_fragmentShaderCode == newFragmentShaderCode)
        return;
    m_fragmentShaderCode = newFragmentShaderCode;
    emit fragmentShaderCodeChanged();
    markDirty(Dirty::FragmentShader);
}

bool QQuick3DSkyMaterial::enableIBL() const
{
    return m_enableIBL;
}

void QQuick3DSkyMaterial::setEnableIBL(bool newEnableIBL)
{
    if (m_enableIBL == newEnableIBL)
        return;
    m_enableIBL = newEnableIBL;
    emit enableIBLChanged();
    update();
}

int QQuick3DSkyMaterial::iblSampleCount() const
{
    return m_iblSampleCount;
}

void QQuick3DSkyMaterial::setIblSampleCount(int newIblSampleCount)
{
    const int clamped = qBound(1, newIblSampleCount, 1024);
    if (m_iblSampleCount == clamped)
        return;
    m_iblSampleCount = clamped;
    emit iblSampleCountChanged();
    update();
}

int QQuick3DSkyMaterial::iblRenderFrames() const
{
    return m_iblRenderFrames;
}

void QQuick3DSkyMaterial::setIblRenderFrames(int newIblRenderFrames)
{
    const int clamped = qMax(0, newIblRenderFrames);
    if (m_iblRenderFrames == clamped)
        return;
    m_iblRenderFrames = clamped;
    emit iblRenderFramesChanged();
    update();
}

QQuick3DSkyMaterial::SkyboxMode QQuick3DSkyMaterial::skyboxMode() const
{
    return m_skyboxMode;
}

void QQuick3DSkyMaterial::setSkyboxMode(SkyboxMode skyboxMode)
{
    if (m_skyboxMode == skyboxMode)
        return;
    m_skyboxMode = skyboxMode;
    emit skyboxModeChanged();
    update();
}

QT_END_NAMESPACE
