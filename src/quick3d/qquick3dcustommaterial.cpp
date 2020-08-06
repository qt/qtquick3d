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

#include "qquick3dcustommaterial_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgshadermaterialadapter_p.h>
#include <QtQuick/QQuickWindow>

#include "qquick3dobject_p.h"
#include "qquick3dviewport_p.h"
#include "qquick3dscenemanager_p.h"

Q_DECLARE_OPAQUE_POINTER(QQuick3DShaderUtilsTextureInput)

QT_BEGIN_NAMESPACE

/*!
    \qmltype CustomMaterial
    \inherits Material
    \inqmlmodule QtQuick3D.Materials
    \brief Base component for creating custom materials used to shade models.

    The custom material allows using custom shader code for a material. A
    vertex, fragment, or both shaders can be provided. The \l vertexShader and
    \l fragmentShader properties are URLs, referencing files containing shader
    snippets, and work very similarly to ShaderEffect or
    \l{Image::source}{Image.source}. Only the \c file and \c qrc schemes are
    supported with custom materials. It is also possible to omit the \c file
    scheme, allowing to specify a relative path in a convenient way. Such a
    path is resolved relative to the component's (the \c{.qml} file's)
    location.

    There are two main types of custom materials. This is specified by the \l
    shadingMode property. In \l{CustomMaterial.Unshaded}{unshaded} custom
    materials the fragment shader outputs a single \c vec4 color, ignoring
    lights, light probes, shadowing in the scene. In
    \l{CustomMaterial.Shaded}{shaded} materials the shader is expected to
    implement certain functions and work with built-in variables to take
    lighting and shadow contribution into account.

    The dynamic properties of the CustomMaterial can be changed and animated
    using QML and Qt Quick facilities, and the values are exposed to the
    shaders automatically. This in practice is very similar ShaderEffect. The
    following list shows how properties are mapped:

    \list
    \li bool, int, real -> bool, int, float
    \li QColor, \l{QtQml::Qt::rgba()}{color} -> vec4
    \li QRect, QRectF, \l{QtQml::Qt::rect()}{rect} -> vec4
    \li QPoint, QPointF, \l{QtQml::Qt::point()}{point}, QSize, QSizeF, \l{QtQml::Qt::size()}{size} -> vec2
    \li QVector2D, \l{QtQml::Qt::vector2d()}{vector2d} -> vec3
    \li QVector3D, \l{QtQml::Qt::vector3d()}{vector3d} -> vec3
    \li QVector4D, \l{QtQml::Qt::vector4d()}{vector4d} -> vec4
    \li QMatrix4x4, \l{QtQml::Qt::matrix4x4()}{matrix4x4} -> mat4
    \li QQuaternion, \l{QtQml::Qt::quaternion()}{quaternion} -> vec4, scalar value is \c w
    \li TextureInput -> sampler2D
    \endlist

    \section1 Unshaded custom materials

    The following is an example of an \l{CustomMaterial.Unshaded}{unshaded}
    custom material.

    \qml
    CustomMaterial {
        // These properties are automatically exposed to the shaders
        property real time: 0.0
        property real amplitude: 5.0
        property real alpha: 1.0
        property TextureInput tex: TextureInput {
            enabled: true
            texture: Texture { source: "image.png" }
        }

        shadingMode: CustomMaterial.Unshaded
        hasTransparency: alpha < 1.0
        sourceBlend: CustomMaterial.SrcAlpha
        destinationBlend: CustomMaterial.OneMinusSrcAlpha
        cullMode: CustomMaterial.BackFaceCulling

        vertexShader: "customshader.vert"
        fragmentShader: "customshader.frag"
    }
    \endqml

    With the above example, the \l{CustomMaterial.Unshaded}{unshaded} vertex
    and fragment shaders snippets could look like the following. Note how the
    shaders do not, and must not, declare uniforms or vertex inputs as that is
    taken care of by Qt when assembling the final shader code.

    \badcode
    VARYING vec3 pos;
    VARYING vec2 texcoord;

    void MAIN()
    {
        pos = VERTEX;
        pos.x += sin(time * 4.0 + pos.y) * amplitude;
        texcoord = UV0;
        POSITION = MODELVIEWPROJECTION_MATRIX * vec4(pos, 1.0);
    }
    \endcode

    \badcode
    VARYING vec3 pos;
    VARYING vec2 texcoord;

    void MAIN()
    {
        vec4 c = texture(tex, texcoord);
        FRAGCOLOR = vec4(pos.x * 0.02, pos.y * 0.02, pos.z * 0.02, alpha) * c;
    }
    \endcode

    The following special, uppercase keywords are available:

    \list

    \li MAIN -> the name of the entry point in the vertex or fragment shader
    snippet must always be \c MAIN. Providing this function is mandatory in
    shader snippets for unshaded custom materials.

    \li VARYING -> declares an output from the vertex shader or an input to the
    fragment shader

    \li POSITION -> vec4, the output from the vertex shader

    \li FRAGCOLOR -> vec4, the output from the fragment shader. Available only
    for unshaded custom materials.

    \li VERTEX -> vec3, the vertex position in the vertex shader.

    \li NORMAL -> vec3, the vertex normal in the vertex shader. When the mesh
    for the associated model does not provide normals, the value is vec3(0.0).

    \li UV0 -> vec2, the first set of texture coordinates in the vertex shader.
    When the mesh for the associated model does not provide texture
    coordinates, the value is vec2(0.0).

    \li UV1 -> vec2, the second set of texture coordinates in the vertex
    shader. When the mesh for the associated model does not provide a second
    set of texture coordinates, the value is vec2(0.0).

    \li COLOR -> vec4, the vertex color in the vertex shader. When the mesh for
    the associated model does not provide per-vertex colors, the value is
    vec4(1.0).

    \li TANGENT -> vec3, tangent in the vertex shader. When the mesh for the
    associated model does not provide tangent data, the value is vec3(0.0).

    \li BINORMAL -> vec3, binormal in the vertex shader. When the mesh for the
    associated model does not provide binormal data, the value is vec3(0.0).

    \li MODELVIEWPROJECTION_MATRIX -> mat4, the model-view-projection matrix

    \li VIEWPROJECTION_MATRIX -> mat4, the view-projection matrix

    \li VIEW_MATRIX -> mat4, the view (camera) matrix

    \li MODEL_MATRIX -> mat4, the model (world) matrix

    \li NORMAL_MATRIX -> mat3, the normal matrix (the transpose of the inverse
    of the top-left 3x3 part of the model matrix)

    \li CAMERA_POSITION -> vec3, the camera position in world space

    \li CAMERA_DIRECTION -> vec3, the camera direction vector

    \li CAMERA_PROPERTIES -> vec2, the near and far clip values for the camera

    \endlist

    \section1 Shaded custom materials

    A \l{CustomMaterial.Shaded}{shaded} material \c augments the shader code
    that would be generated by a PrincipledMaterial. Unlike unshaded materials,
    that provide almost all logic for the vertex and fragment shader main
    functions on their own, preventing adding generated code for lighting,
    shadowing, global illumination, etc., shaded materials let shader
    generation happen normally, as if the CustomMaterial was a
    PrincipledMaterial. The vertex and fragment shader snippets are expected to
    provide optional functions that are then invoked at certain points, giving
    them the possibility to customize the colors and other values that are then
    used for calculating lighting and the final fragment color.

    Rather than implementing just a \c MAIN function, the fragment shader for a
    shaded custom material can implement multiple functions. All functions,
    including \c MAIN, are optional to implement in shaded custom materials. An
    empty shader snippet, or, even, not specifying the
    \l{CustomMaterial::vertexShader}{vertexShader} or
    \l{CustomMaterial::fragmentShader}{fragmentShader} properties at all can be
    perfectly valid too.

    \section2 Vertex shader snippets in a shaded custom material

    The following functions can be implemented in a vertex shader snippet:

    \list

    \li \c{void MAIN()} When present, this function is called in order to set
    the value of \c POSITION, the vec4 output from the vertex shader, and,
    optionally, to modify the values of \c VERTEX, \c COLOR, \c NORMAL, \c UV0,
    \c UV1, \c TANGENT, and \c BINORMAL. Unlike in unshaded materials, writing
    to these makes sense because the modified values are then taken into
    account in the rest of the generated shader code (whereas for unshaded
    materials there is no additional shader code generated). For example, if
    the custom vertex shader displaces the vertices or the normals, it will
    want to store the modified values to \c VERTEX or \c NORMAL, to achieve
    correct lighting calculations afterwards. Additionally, the function can
    write to variables defined with \c VARYING in order to pass interpolated
    data to the fragment shader. When this function is not present, \c POSITION
    is calculated based on \c VERTEX and \c MODELVIEWPROJECTION_MATRIX, just
    like a PrincipledMaterial would do.

    Example, with relying both on QML properties exposed as uniforms, and also
    passing data to the fragment shader:
    \badcode
        VARYING vec3 vNormal;
        VARYING vec3 vViewVec;

        void MAIN()
        {
            VERTEX.x += sin(uTime * 4.0 + VERTEX.y) * uAmplitude;
            vNormal = normalize(NORMAL_MATRIX * NORMAL);
            vViewVec = CAMERA_POSITION - (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
            POSITION = MODELVIEWPROJECTION_MATRIX * vec4(VERTEX, 1.0);
        }
    \endcode

    \endlist

    \section2 Fragment shader snippets in a shaded custom material

    The following functions can be implemented in a fragment shader snippet:

    \list

    \li \c{void AMBIENT_LIGHT()} When present, this function is called once for
    each fragment. The task of the function is to add the total ambient
    contribution to a writable special variable \c DIFFUSE. It can of course
    choose to calculate a different value, or not touch \c DIFFUSE at all (to
    ignore ambient lighting completely). When this function is not present at
    all, the ambient contribution is calculated normally, like a
    PrincipledMaterial would do.

    The function can write to the following special variables:

    \list

    \li vec3 \c DIFFUSE Accumulates the diffuse light contributions, per
    fragment. The light processor functions will typically add (\c{+=}) to it,
    since overwriting the value would lose the contribution from other lights.

    \endlist

    The function can read the following special variables, in addition to the
    matrix (such as, \c MODEL_MATRIX) and vector (such as, \c CAMERA_POSITION)
    uniforms from the table above:

    \list
    \li vec3 \c TOTAL_AMBIENT_COLOR The total ambient contribution in the scene.
    \endlist

    Example:
    \badcode
        void AMBIENT_LIGHT()
        {
            DIFFUSE += TOTAL_AMBIENT_COLOR;
        }
    \endcode

    \li \c{void DIRECTIONAL_LIGHT()} When present, this function is called for
    each active directional light in the scene for each fragment. The task of
    the function is to add the diffuse contribution to a writable special
    variable \c DIFFUSE. The function can also choose to do nothing, in which
    case diffuse contributions from directional lights are ignored. When the
    function is not present at all, the diffuse contributions from directional
    lights are accumulated normally, like a PrincipledMaterial would do.

    The function can write to the following special variables:

    \list

    \li vec3 \c DIFFUSE Accumulates the diffuse light contributions, per
    fragment. The light processor functions will typically add (\c{+=}) to it,
    since overwriting the value would lose the contribution from other lights.

    \endlist

    The function can read the following special variables, in addition to the
    matrix (such as, \c MODEL_MATRIX) and vector (such as, \c CAMERA_POSITION)
    uniforms from the table above:

    \list

    \li vec3 \c LIGHT_COLOR Diffuse light color.
    \li float \c SHADOW_CONTRIB Shadow contribution, or 1.0 if not shadowed at all or not reciving shadows.
    \li vec3 \c TO_LIGHT_DIR Vector pointing towards the light source.

    \endlist

    Example:
    \badcode
        void DIRECTIONAL_LIGHT()
        {
            DIFFUSE += LIGHT_COLOR * SHADOW_CONTRIB * vec3(max(0.0, dot(normalize(VAR_WORLD_NORMAL), TO_LIGHT_DIR)));
        }
    \endcode

    \li \c{void POINT_LIGHT()} When present, this function is called for
    each active point light in the scene for each fragment. The task of
    the function is to add the diffuse contribution to a writable special
    variable \c DIFFUSE. The function can also choose to do nothing, in which
    case diffuse contributions from point lights are ignored. When the
    function is not present at all, the diffuse contributions from point
    lights are accumulated normally, like a PrincipledMaterial would do.

    The function can write to the following special variables:

    \list
    \li vec3 \c DIFFUSE Accumulates the diffuse light contributions, per fragment.
    \endlist

    The function can read the following special variables, in addition to the
    matrix (such as, \c MODEL_MATRIX) and vector (such as, \c CAMERA_POSITION)
    uniforms from the table above:

    \list
    \li vec3 \c LIGHT_COLOR Diffuse light color.
    \li float \c LIGHT_ATTENUATION Light attenuation.
    \li float \c SHADOW_CONTRIB Shadow contribution, or 1.0 if not shadowed at all or not reciving shadows.
    \li vec3 \c TO_LIGHT_DIR Vector pointing towards the light source.
    \endlist

    Example:
    \badcode
        void POINT_LIGHT()
        {
            DIFFUSE += LIGHT_COLOR * LIGHT_ATTENUATION * SHADOW_CONTRIB * vec3(max(0.0, dot(normalize(VAR_WORLD_NORMAL), TO_LIGHT_DIR)));
        }
    \endcode

    \li \c{void SPOT_LIGHT()} When present, this function is called for
    each active spot light in the scene for each fragment. The task of
    the function is to add the diffuse contribution to a writable special
    variable \c DIFFUSE. The function can also choose to do nothing, in which
    case diffuse contributions from spot lights are ignored. When the
    function is not present at all, the diffuse contributions from spot
    lights are accumulated normally, like a PrincipledMaterial would do.

    The function can write to the following special variables:

    \list
    \li vec3 \c DIFFUSE Accumulates the diffuse light contributions, per fragment.
    \endlist

    The function can read the following special variables, in addition to the
    matrix (such as, \c MODEL_MATRIX) and vector (such as, \c CAMERA_POSITION)
    uniforms from the table above:

    \list
    \li vec3 \c LIGHT_COLOR Diffuse light color.
    \li float \c LIGHT_ATTENUATION Light attenuation.
    \li float \c SHADOW_CONTRIB Shadow contribution, or 1.0 if not shadowed at all or not reciving shadows.
    \li vec3 \c TO_LIGHT_DIR Vector pointing towards the light source.
    \li float \c SPOT_FACTOR Spot light factor.
    \endlist

    Example:
    \badcode
        void SPOT_LIGHT()
        {
            DIFFUSE += LIGHT_COLOR * LIGHT_ATTENUATION * SPOT_FACTOR * SHADOW_CONTRIB * vec3(max(0.0, dot(normalize(VAR_WORLD_NORMAL), TO_LIGHT_DIR)));
        }
    \endcode

    \li \c{void AREA_LIGHT()} When present, this function is called for
    each active area light in the scene for each fragment. The task of
    the function is to add the diffuse contribution to a writable special
    variable \c DIFFUSE. The function can also choose to do nothing, in which
    case diffuse contributions from area lights are ignored. When the
    function is not present at all, the diffuse contributions from area
    lights are accumulated normally, like a PrincipledMaterial would do.

    The function can write to the following special variables:

    \list
    \li vec3 \c DIFFUSE Accumulates the diffuse light contributions, per fragment.
    \endlist

    The function can read the following special variables, in addition to the
    matrix (such as, \c MODEL_MATRIX) and vector (such as, \c CAMERA_POSITION)
    uniforms from the table above:

    \list
    \li vec3 \c LIGHT_COLOR Diffuse light color.
    \li float \c LIGHT_ATTENUATION Light attenuation.
    \li float \c SHADOW_CONTRIB Shadow contribution, or 1.0 if not shadowed at all or not reciving shadows.
    \li vec3 \c TO_LIGHT_DIR Vector pointing towards the light source.
    \endlist

    Example:
    \badcode
        void AREA_LIGHT()
        {
            DIFFUSE += LIGHT_COLOR * LIGHT_ATTENUATION * SHADOW_CONTRIB * vec3(max(0.0, dot(normalize(VAR_WORLD_NORMAL), TO_LIGHT_DIR)));
        }
    \endcode

    \li \c{void SPECULAR_LIGHT()} When present, this function is called for
    each active light in the scene for each fragment. The task of the function
    is to add the specular contribution to a writable special variable \c
    SPECULAR. The function can also choose to do nothing, in which case
    specular contributions from lights are ignored. When the function is not
    present at all, the specular contributions from lights are accumulated
    normally, like a PrincipledMaterial would do.

    The function can write to the following special variables:

    \list

    \li vec3 \c SPECULAR Accumulates the specular light contributions, per
    frament. The light processor functions will typically add (\c{+=}) to it,
    since overwriting the value would lose the contribution from other lights.

    \endlist

    The function can read the following special variables, in addition to the
    matrix (such as, \c MODEL_MATRIX) and vector (such as, \c CAMERA_POSITION)
    uniforms from the table above:

    \list
    \li vec3 \c LIGHT_COLOR Specular light color.
    \li float \c LIGHT_ATTENUATION Light attenuation. For directional lights the value is 1.0.
    \li float \c SHADOW_CONTRIB Shadow contribution, or 1.0 if not shadowed at all or not reciving shadows.
    \li vec3 \c TO_LIGHT_DIR Vector pointing towards the light source.
    \endlist

    Example, with uShininess assumed to be a dynamic property on the
    CustomMaterial, declared, for example, as \c{property real uShininess:
    50.0}:

    \badcode
        void SPECULAR_LIGHT()
        {
            vec3 V = normalize(VAR_VIEW_VECTOR);
            vec3 H = normalize(V + TO_LIGHT_DIR);
            float cosAlpha = max(0.0, dot(H, normalize(VAR_WORLD_NORMAL)));
            float shine = pow(cosAlpha, uShininess);
            const vec3 specularColor = vec3(1.0);
            SPECULAR += shine * specularColor;
        }
    \endcode

    \li \c{void MAIN()} When present, this function is called to set the values
    of the special writable variables \c METALNESS, \c ROUGHNESS, \c
    SPECULAR_AMOUNT, and \c FRESNEL_POWER. This is relevant first and foremost
    when there is no custom \c SPECULAR_LIGHT function provided, and when there
    is a light probe set in the SceneEnvironment. In other cases, this function
    does not need to be present.

    The function can write to the following special variables. The values
    written to these will typically be either hardcoded or be calculated based
    on QML properties mapped to uniforms. The semantics are equivalant to
    PrincipledMaterial, meaning in practice one is expected to set either \c
    METALNESS (non-dielectric material) or \c SPECULAR_AMOUNT (dielectric) to a
    non-zero value.

    \list

    \li float \c METALNESS Metalness amount in range 0.0 - 1.0. Must be set to
    a non-zero value to have effect. \c SPECULAR_AMOUNT should be left at its
    default 0.0 value then.

    \li float \c ROUGHNESS Roughness value in range 0.0 - 1.0.

    \li float \c FRESNEL_POWER Specifies the fresnel power. A typical value is
    \c{5.0} as that is what a PrincipledMaterial would use.

    \li float \c SPECULAR_AMOUNT Specular amount in range 0.0 - 1.0. Must be
    set to a non-zero value to have effect. \c METALNESS must be left at its
    default 0.0 value then.

    \li float \c IOR Specifies the index of refraction. Relevant for
    dielectrics (when SPECULAR_AMOUNT is set to a non-zero value).

    \endlist

    \note Unlike with unshaded materials, the fragment \c MAIN for a shaded
    material has no control over \c FRAGCOLOR. Rather, it is the \c DIFFUSE and
    \c SPECULAR values written in the light processor functions that decide
    what the final fragment color is.

    Example:
    \badcode
        void MAIN()
        {
            METALNESS = 1.0;
            ROUGHNESS = 0.5;
            FRESNEL_POWER = 5.0;
        }
    \endcode

    \endlist

    \section2 Additional special keywords

    The custom fragment shader code can freely access uniforms (such as, \c
    CAMERA_DIRECTION or \c CAMERA_POSITION), and varyings passed on from the
    custom vertex shader. Additionally, there are a number of built-in varyings
    available as special keywords. These are optional in the sense that a
    vertex \c MAIN can calculate and pass on these on its own, but to reduce
    duplicated data fragment shaders can also rely on these built-ins instead:

    \list
    \li vec3 \c VAR_WORLD_NORMAL
    \li vec3 \c VAR_VIEW_VECTOR
    \li vec3 \c VAR_WORLD_POS
    \li vec4 \c VAR_COLOR
    \endlist

    The light processor functions also have access to the following:

    \list

    \li vec3 \c WORLD_NORMAL - Unlike VAR_WORLD_NORMAL, which is the
    interpolated normal as-is, this value is potentially adjused for
    double-sidedness: when rendering with culling disabled, the normal will get
    inverted as necessary. Therefore lighting calculations should be using \c
    WORLD_NORMAL instead of \c VAR_WORLD_NORMAL in order behave correctly with
    all culling modes.

    \endlist

    \section1 Screen, depth, and other textures

    The rendering pipeline can expose a number of textures to the custom
    material shaders with content from special render passes. This applies both
    to shaded and unshaded custom materials.

    For example, a shader may want access to a depth texture that contains the
    depth buffer contents for the opaque objects in the scene. This is achieved
    by sampling \c DEPTH_TEXTURE. Such a texture is not normally generated,
    unless there is a real need for it. Therefore, the presence of the
    following keywords in the vertex or fragment shader also acts as a toggle
    for opting in to the - potentially expensive - passes for generating the
    texture in question. (of course, it could be that some of these become
    already enabled due to other settings, such as the ambient occlusion
    parameters in SceneEnvironment or due to a post-processing effect relying
    on the depth texture, in which case the textures in question are generated
    regardless of the custom material and so sampling these special textures in
    the material comes at no extra cost apart from the texture access itself)

    \list

    \li \c SCREEN_TEXTURE - When present, a texture (sampler2D) with the color
    buffer from a rendering pass containing the opaque objects in the scene is
    exposed to the shader under this name.

    \li \c DEPTH_TEXTURE - When present, a texture (sampler2D) with the
    (non-linearized) depth buffer contents is exposed to the shader under this
    name.

    \li \c AO_TEXTURE - When present and screen space ambient occlusion is
    enabled (meaning when the AO strength and distance are both non-zero) in
    SceneEnvironment, the SSAO texture (sampler2D) is exposed to the shader
    under this name.

    \endlist

    \sa {Qt Quick 3D - Custom Shaders Example}, {Qt Quick 3D - Custom Materials Example}
*/

/*!
    \qmlproperty url CustomMaterial::vertexShader

    Specfies the file with the snippet of custom vertex shader code.

    The value is a URL and must either be a local file or use the qrc scheme to
    access files embedded via the Qt resource system. Relative file paths
    (without a scheme) are also accepted, in which case the file is treated as
    relative to the component (the \c{.qml} file).

    \sa fragmentShader
*/

/*!
    \qmlproperty url CustomMaterial::fragmentShader

    Specfies the file with the snippet of custom fragment shader code.

    The value is a URL and must either be a local file or use the qrc scheme to
    access files embedded via the Qt resource system. Relative file paths
    (without a scheme) are also accepted, in which case the file is treated as
    relative to the component (the \c{.qml} file).

    \sa vertexShader
*/

/*!
    \qmlproperty enumeration CustomMaterial::shadingMode
    Specifies the type of the material. The default value is Shaded.

    \value CustomMaterial.Unshaded
    \value CustomMaterial.Shaded
*/

/*!
    \qmlproperty bool CustomMaterial::hasTransparency

    Specifies that the material has transparency. For example, a material where
    the fragment shader outputs fragment colors with an alpha smaller than 1.0
    will need this to be set to true. The default value is false.
*/

/*!
    \qmlproperty bool CustomMaterial::alwaysDirty
    Specifies that the material state is always dirty, which indicates that the material needs
    to be refreshed every time it is used by the QtQuick3D.
*/

/*!
    \qmlproperty enumeration CustomMaterial::sourceBlend

    Specifies the source blend factor. The default value is \l
    CustomMaterial.NoBlend. Note that blending is only active when \l
    hasTransparency is enabled and the source and destination blend factors are
    something other than CustomMaterial.NoBlend.

    \value CustomMaterial.NoBlend
    \value CustomMaterial.Zero
    \value CustomMaterial.One
    \value CustomMaterial.SrcColor
    \value CustomMaterial.OneMinusSrcColor
    \value CustomMaterial.DstColor
    \value CustomMaterial.OneMinusDstColor
    \value CustomMaterial.SrcAlpha
    \value CustomMaterial.OneMinusSrcAlpha
    \value CustomMaterial.DstAlpha
    \value CustomMaterial.OneMinusDstAlpha
    \value CustomMaterial.ConstantColor
    \value CustomMaterial.OneMinusConstantColor
    \value CustomMaterial.ConstantAlpha
    \value CustomMaterial.OneMinusConstantAlpha
    \value CustomMaterial.SrcAlphaSaturate
*/

/*!
    \qmlproperty enumeration CustomMaterial::destinationBlend

    Specifies the destination blend factor. The default value is \l
    CustomMaterial.NoBlend. Note that blending is only active when \l
    hasTransparency is enabled and the source and destination blend factors are
    something other than CustomMaterial.NoBlend.

    \value CustomMaterial.NoBlend
    \value CustomMaterial.Zero
    \value CustomMaterial.One
    \value CustomMaterial.SrcColor
    \value CustomMaterial.OneMinusSrcColor
    \value CustomMaterial.DstColor
    \value CustomMaterial.OneMinusDstColor
    \value CustomMaterial.SrcAlpha
    \value CustomMaterial.OneMinusSrcAlpha
    \value CustomMaterial.DstAlpha
    \value CustomMaterial.OneMinusDstAlpha
    \value CustomMaterial.ConstantColor
    \value CustomMaterial.OneMinusConstantColor
    \value CustomMaterial.ConstantAlpha
    \value CustomMaterial.OneMinusConstantAlpha
    \value CustomMaterial.SrcAlphaSaturate
*/

static inline QRhiGraphicsPipeline::BlendFactor toRhiBlendFactor(QQuick3DCustomMaterial::BlendMode mode)
{
    switch (mode) {
    case QQuick3DCustomMaterial::BlendMode::Zero:
        return QRhiGraphicsPipeline::Zero;
    case QQuick3DCustomMaterial::BlendMode::One:
        return QRhiGraphicsPipeline::One;
    case QQuick3DCustomMaterial::BlendMode::SrcColor:
        return QRhiGraphicsPipeline::SrcColor;
    case QQuick3DCustomMaterial::BlendMode::OneMinusSrcColor:
        return QRhiGraphicsPipeline::OneMinusSrcColor;
    case QQuick3DCustomMaterial::BlendMode::DstColor:
        return QRhiGraphicsPipeline::DstColor;
    case QQuick3DCustomMaterial::BlendMode::OneMinusDstColor:
        return QRhiGraphicsPipeline::OneMinusDstColor;
    case QQuick3DCustomMaterial::BlendMode::SrcAlpha:
        return QRhiGraphicsPipeline::SrcAlpha;
    case QQuick3DCustomMaterial::BlendMode::OneMinusSrcAlpha:
        return QRhiGraphicsPipeline::OneMinusSrcAlpha;
    case QQuick3DCustomMaterial::BlendMode::DstAlpha:
        return QRhiGraphicsPipeline::DstAlpha;
    case QQuick3DCustomMaterial::BlendMode::OneMinusDstAlpha:
        return QRhiGraphicsPipeline::OneMinusDstAlpha;
    case QQuick3DCustomMaterial::BlendMode::ConstantColor:
        return QRhiGraphicsPipeline::ConstantColor;
    case QQuick3DCustomMaterial::BlendMode::OneMinusConstantColor:
        return QRhiGraphicsPipeline::OneMinusConstantColor;
    case QQuick3DCustomMaterial::BlendMode::ConstantAlpha:
        return QRhiGraphicsPipeline::ConstantAlpha;
    case QQuick3DCustomMaterial::BlendMode::OneMinusConstantAlpha:
        return QRhiGraphicsPipeline::OneMinusConstantAlpha;
    case QQuick3DCustomMaterial::BlendMode::SrcAlphaSaturate:
        return QRhiGraphicsPipeline::SrcAlphaSaturate;
    default:
        return QRhiGraphicsPipeline::One;
    }
}

template <QVariant::Type>
struct ShaderType
{
};

template<>
struct ShaderType<QVariant::Double>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::Float; }
    static QByteArray name() { return QByteArrayLiteral("float"); }
};

template<>
struct ShaderType<QVariant::Bool>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::Boolean; }
    static QByteArray name() { return QByteArrayLiteral("bool"); }
};

template<>
struct ShaderType<QVariant::Int>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::Integer; }
    static QByteArray name() { return QByteArrayLiteral("int"); }
};

template<>
struct ShaderType<QVariant::Vector2D>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::Vec2; }
    static QByteArray name() { return QByteArrayLiteral("vec2"); }
};

template<>
struct ShaderType<QVariant::Vector3D>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::Vec3; }
    static QByteArray name() { return QByteArrayLiteral("vec3"); }
};

template<>
struct ShaderType<QVariant::Vector4D>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::Vec4; }
    static QByteArray name() { return QByteArrayLiteral("vec4"); }
};

template<>
struct ShaderType<QVariant::Color>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::Rgba; }
    static QByteArray name() { return QByteArrayLiteral("vec4"); }
};

template<>
struct ShaderType<QVariant::Size>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::Size; }
    static QByteArray name() { return QByteArrayLiteral("vec2"); }
};

template<>
struct ShaderType<QVariant::SizeF>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::SizeF; }
    static QByteArray name() { return QByteArrayLiteral("vec2"); }
};

template<>
struct ShaderType<QVariant::Point>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::Point; }
    static QByteArray name() { return QByteArrayLiteral("vec2"); }
};

template<>
struct ShaderType<QVariant::PointF>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::PointF; }
    static QByteArray name() { return QByteArrayLiteral("vec2"); }
};

template<>
struct ShaderType<QVariant::Rect>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::Rect; }
    static QByteArray name() { return QByteArrayLiteral("vec4"); }
};

template<>
struct ShaderType<QVariant::RectF>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::RectF; }
    static QByteArray name() { return QByteArrayLiteral("vec4"); }
};

template<>
struct ShaderType<QVariant::Quaternion>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::Quaternion; }
    static QByteArray name() { return QByteArrayLiteral("vec4"); }
};

template<>
struct ShaderType<QVariant::Matrix4x4>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::Matrix4x4; }
    static QByteArray name() { return QByteArrayLiteral("mat4"); }
};

QQuick3DCustomMaterial::QQuick3DCustomMaterial(QQuick3DObject *parent)
    : QQuick3DMaterial(*(new QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::CustomMaterial)), parent)
{
}

QQuick3DCustomMaterial::~QQuick3DCustomMaterial() {}

bool QQuick3DCustomMaterial::hasTransparency() const
{
    return m_hasTransparency;
}

void QQuick3DCustomMaterial::setHasTransparency(bool hasTransparency)
{
    if (m_hasTransparency == hasTransparency)
        return;

    m_hasTransparency = hasTransparency;
    update();
    emit hasTransparencyChanged();
}

QQuick3DCustomMaterial::BlendMode QQuick3DCustomMaterial::srcBlend() const
{
    return m_srcBlend;
}

void QQuick3DCustomMaterial::setSrcBlend(BlendMode mode)
{
    if (m_srcBlend == mode)
        return;

    m_srcBlend = mode;
    update();
    emit srcBlendChanged();
}

QQuick3DCustomMaterial::BlendMode QQuick3DCustomMaterial::dstBlend() const
{
    return m_dstBlend;
}

void QQuick3DCustomMaterial::setDstBlend(BlendMode mode)
{
    if (m_dstBlend == mode)
        return;

    m_dstBlend = mode;
    update();
    emit dstBlendChanged();
}

QQuick3DCustomMaterial::ShadingMode QQuick3DCustomMaterial::shadingMode() const
{
    return m_shadingMode;
}

void QQuick3DCustomMaterial::setShadingMode(ShadingMode mode)
{
    if (m_shadingMode == mode)
        return;

    m_shadingMode = mode;
    markDirty(Dirty::ShaderSettingsDirty);
    emit shadingModeChanged();
}

QUrl QQuick3DCustomMaterial::vertexShader() const
{
    return m_vertexShader;
}

void QQuick3DCustomMaterial::setVertexShader(const QUrl &url)
{
    if (m_vertexShader == url)
        return;

    m_vertexShader = url;
    markDirty(Dirty::ShaderSettingsDirty);
    emit vertexShaderChanged();
}

QUrl QQuick3DCustomMaterial::fragmentShader() const
{
    return m_fragmentShader;
}

void QQuick3DCustomMaterial::setFragmentShader(const QUrl &url)
{
    if (m_fragmentShader == url)
        return;

    m_fragmentShader = url;
    markDirty(Dirty::ShaderSettingsDirty);
    emit fragmentShaderChanged();
}

void QQuick3DCustomMaterial::markAllDirty()
{
    m_dirtyAttributes = 0xffffffff;
    QQuick3DMaterial::markAllDirty();
}

bool QQuick3DCustomMaterial::alwaysDirty() const
{
    return m_alwaysDirty;
}

void QQuick3DCustomMaterial::setAlwaysDirty(bool alwaysDirty)
{
    if (m_alwaysDirty == alwaysDirty)
        return;

    m_alwaysDirty = alwaysDirty;
    update();
    emit alwaysDirtyChanged();
}

QSSGRenderGraphObject *QQuick3DCustomMaterial::updateSpatialNode(QSSGRenderGraphObject *node)
{
    // Find the parent window
    QQuickWindow *window = nullptr;
    if (const auto &manager = QQuick3DObjectPrivate::get(this)->sceneManager)
        window = manager->window();

    QSSGShaderCustomMaterialAdapter::UniformList uniforms;
    QSSGRenderCustomMaterial *customMaterial = static_cast<QSSGRenderCustomMaterial *>(node);
    bool newBackendNode = false;
    bool shadersMayChange = false;
    if (!customMaterial) {
        customMaterial = new QSSGRenderCustomMaterial;
        newBackendNode = true;
    } else if (m_dirtyAttributes & ShaderSettingsDirty) {
        shadersMayChange = true;
    }

    if (newBackendNode || shadersMayChange) {
        markAllDirty();

        customMaterial->m_properties.clear();
        customMaterial->m_textureProperties.clear();

        customMaterial->m_shadingMode = QSSGRenderCustomMaterial::ShadingMode(int(m_shadingMode));

        QMetaMethod propertyDirtyMethod;
        const int idx = metaObject()->indexOfSlot("onPropertyDirty()");
        if (idx != -1)
            propertyDirtyMethod = metaObject()->method(idx);

        const int propCount = metaObject()->propertyCount();
        int propOffset = metaObject()->propertyOffset();

        // Custom materials can have multilayered inheritance structure, so find the actual propOffset
        const QMetaObject *superClass = metaObject()->superClass();
        while (superClass && qstrcmp(superClass->className(), "QQuick3DCustomMaterial") != 0)  {
            propOffset = superClass->propertyOffset();
            superClass = superClass->superClass();
        }

        QVector<QMetaProperty> userProperties;
        for (int i = propOffset; i != propCount; ++i) {
            const auto property = metaObject()->property(i);
            if (Q_UNLIKELY(!property.isValid()))
                continue;

            if (newBackendNode) {
                // Track the property changes
                if (property.hasNotifySignal() && propertyDirtyMethod.isValid())
                    connect(this, property.notifySignal(), this, propertyDirtyMethod);
            } // else already connected

            switch (property.type()) {
            case QVariant::Double:
                uniforms.append({ ShaderType<QVariant::Double>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Double>::type(), i});
                break;
            case QVariant::Bool:
                uniforms.append({ ShaderType<QVariant::Bool>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Bool>::type(), i});
                break;
            case QVariant::Vector2D:
                uniforms.append({ ShaderType<QVariant::Vector2D>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Vector2D>::type(), i});
                break;
            case QVariant::Vector3D:
                uniforms.append({ ShaderType<QVariant::Vector3D>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Vector3D>::type(), i});
                break;
            case QVariant::Vector4D:
                uniforms.append({ ShaderType<QVariant::Vector4D>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Vector4D>::type(), i});
                break;
            case QVariant::Int:
                uniforms.append({ ShaderType<QVariant::Int>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Int>::type(), i});
                break;
            case QVariant::Color:
                uniforms.append({ ShaderType<QVariant::Color>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Color>::type(), i});
                break;
            case QVariant::Size:
                uniforms.append({ ShaderType<QVariant::Size>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Size>::type(), i});
                break;
            case QVariant::SizeF:
                uniforms.append({ ShaderType<QVariant::SizeF>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::SizeF>::type(), i});
                break;
            case QVariant::Point:
                uniforms.append({ ShaderType<QVariant::Point>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Point>::type(), i});
                break;
            case QVariant::PointF:
                uniforms.append({ ShaderType<QVariant::PointF>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::PointF>::type(), i});
                break;
            case QVariant::Rect:
                uniforms.append({ ShaderType<QVariant::Rect>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Rect>::type(), i});
                break;
            case QVariant::RectF:
                uniforms.append({ ShaderType<QVariant::RectF>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::RectF>::type(), i});
                break;
            case QVariant::Quaternion:
                uniforms.append({ ShaderType<QVariant::Quaternion>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Quaternion>::type(), i});
                break;
            case QVariant::Matrix4x4:
                uniforms.append({ ShaderType<QVariant::Matrix4x4>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Matrix4x4>::type(), i});
                break;
            case QVariant::UserType:
                if (property.userType() == qMetaTypeId<QQuick3DShaderUtilsTextureInput *>())
                    userProperties.push_back(property);
                break;
            default:
                // ### figure out how _not_ to warn when there are no dynamic
                // properties defined (because warnings like Skipping vertexShader etc. are not helpful)
                //qWarning("No known uniform conversion found for custom material property %s. Skipping", property.name());
                break;
            }
        }

        for (const auto &userProperty : qAsConst(userProperties)) {
            QQuick3DShaderUtilsTextureInput *texture = userProperty.read(this).value<QQuick3DShaderUtilsTextureInput *>();
            const QByteArray &name = userProperty.name();
            if (name.isEmpty())
                continue;

            texture->name = name;

            QSSGRenderCustomMaterial::TextureProperty textureData;
            textureData.texInput = texture;
            textureData.name = name;
            textureData.shaderDataType = QSSGRenderShaderDataType::Texture2D;

            if (newBackendNode) {
                connect(texture, &QQuick3DShaderUtilsTextureInput::enabledChanged, this, &QQuick3DCustomMaterial::onTextureDirty);
                connect(texture, &QQuick3DShaderUtilsTextureInput::textureChanged, this, &QQuick3DCustomMaterial::onTextureDirty);
            } // else already connected

            uniforms.append({ QByteArrayLiteral("sampler2D"), textureData.name });
            customMaterial->m_textureProperties.push_back(textureData);
        }

        const QQmlContext *context = qmlContext(this);
        QByteArray vertex, fragment;
        QSSGCustomShaderMetaData vertexMeta, fragmentMeta;
        QByteArray shaderPathKey;

        customMaterial->m_renderFlags = {};

        if (!m_vertexShader.isEmpty()) {
            vertex = QSSGShaderUtils::resolveShader(m_vertexShader, context, shaderPathKey);
            QByteArray shaderCodeMeta;
            auto result = QSSGShaderCustomMaterialAdapter::prepareCustomShader(shaderCodeMeta,
                                                                               vertex,
                                                                               QSSGShaderCache::ShaderType::Vertex,
                                                                               uniforms);
            vertex = result.first;
            vertex.append(shaderCodeMeta);
            vertexMeta = result.second;

            if (vertexMeta.flags.testFlag(QSSGCustomShaderMetaData::UsesScreenTexture))
                customMaterial->m_renderFlags.setFlag(QSSGRenderCustomMaterial::RenderFlag::ScreenTexture, true);
            if (vertexMeta.flags.testFlag(QSSGCustomShaderMetaData::UsesDepthTexture))
                customMaterial->m_renderFlags.setFlag(QSSGRenderCustomMaterial::RenderFlag::DepthTexture, true);
            if (vertexMeta.flags.testFlag(QSSGCustomShaderMetaData::UsesAoTexture))
                customMaterial->m_renderFlags.setFlag(QSSGRenderCustomMaterial::RenderFlag::AoTexture, true);
        }

        if (!m_fragmentShader.isEmpty()) {
            fragment = QSSGShaderUtils::resolveShader(m_fragmentShader, context, shaderPathKey);
            QByteArray shaderCodeMeta;
            auto result = QSSGShaderCustomMaterialAdapter::prepareCustomShader(shaderCodeMeta,
                                                                               fragment,
                                                                               QSSGShaderCache::ShaderType::Fragment,
                                                                               uniforms);
            fragment = result.first;
            fragment.append(shaderCodeMeta);
            fragmentMeta = result.second;

            if (fragmentMeta.flags.testFlag(QSSGCustomShaderMetaData::UsesScreenTexture))
                customMaterial->m_renderFlags.setFlag(QSSGRenderCustomMaterial::RenderFlag::ScreenTexture, true);
            if (fragmentMeta.flags.testFlag(QSSGCustomShaderMetaData::UsesDepthTexture))
                customMaterial->m_renderFlags.setFlag(QSSGRenderCustomMaterial::RenderFlag::DepthTexture, true);
            if (fragmentMeta.flags.testFlag(QSSGCustomShaderMetaData::UsesAoTexture))
                customMaterial->m_renderFlags.setFlag(QSSGRenderCustomMaterial::RenderFlag::AoTexture, true);
        }

        // At this point we have snippets that look like this:
        //   - the original code, with VARYING ... lines removed
        //   - followed by QQ3D_SHADER_META block for uniforms
        //   - followed by QQ3D_SHADER_META block for inputs/outputs

        customMaterial->m_customShaderPresence = {};
        if (!vertex.isEmpty() || !fragment.isEmpty()) {
            const auto &renderContext = QSSGRenderContextInterface::getRenderContextInterface(quintptr(window));

            customMaterial->m_shaderPathKey = shaderPathKey;

            if (!vertex.isEmpty()) {
                customMaterial->m_customShaderPresence.setFlag(QSSGRenderCustomMaterial::CustomShaderPresenceFlag::Vertex);
                renderContext->shaderLibraryManager()->setShaderSource(shaderPathKey, QSSGShaderCache::ShaderType::Vertex, vertex, vertexMeta);
            }

            if (!fragment.isEmpty()) {
                customMaterial->m_customShaderPresence.setFlag(QSSGRenderCustomMaterial::CustomShaderPresenceFlag::Fragment);
                renderContext->shaderLibraryManager()->setShaderSource(shaderPathKey, QSSGShaderCache::ShaderType::Fragment, fragment, fragmentMeta);
            }
        }
    }

    customMaterial->m_alwaysDirty = m_alwaysDirty;
    customMaterial->m_hasTransparency = m_hasTransparency;
    if (m_hasTransparency && m_srcBlend != BlendMode::NoBlend && m_dstBlend != BlendMode::NoBlend) {
        customMaterial->m_renderFlags.setFlag(QSSGRenderCustomMaterial::RenderFlag::Blending, true);
        customMaterial->m_srcBlend = toRhiBlendFactor(m_srcBlend);
        customMaterial->m_dstBlend = toRhiBlendFactor(m_dstBlend);
    } else {
        customMaterial->m_renderFlags.setFlag(QSSGRenderCustomMaterial::RenderFlag::Blending, false);
    }

    QQuick3DMaterial::updateSpatialNode(customMaterial);

    if (m_dirtyAttributes & Dirty::PropertyDirty) {
        for (auto &prop : customMaterial->m_properties) {
            auto p = metaObject()->property(prop.pid);
            if (Q_LIKELY(p.isValid()))
                prop.value = p.read(this);
        }
    }

    if (m_dirtyAttributes & Dirty::TextureDirty) {
        for (auto &prop : customMaterial->m_textureProperties) {
            QQuick3DTexture *tex = prop.texInput->texture();
            if (tex) {
                if (prop.texInput->enabled)
                    prop.texImage = tex->getRenderImage();
                else
                    prop.texImage = nullptr;
                prop.clampType = tex->horizontalTiling() == QQuick3DTexture::Repeat ? QSSGRenderTextureCoordOp::Repeat
                                                            : (tex->horizontalTiling() == QQuick3DTexture::ClampToEdge) ? QSSGRenderTextureCoordOp::ClampToEdge
                                                            : QSSGRenderTextureCoordOp::MirroredRepeat;
            } else {
                prop.texImage = nullptr;
            }
        }
    }

    m_dirtyAttributes = 0;

    return customMaterial;
}

void QQuick3DCustomMaterial::onPropertyDirty()
{
    markDirty(Dirty::PropertyDirty);
    update();
}

void QQuick3DCustomMaterial::onTextureDirty()
{
    markDirty(Dirty::TextureDirty);
    update();
}

QT_END_NAMESPACE
