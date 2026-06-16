// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtQuick3D/private/qquick3dshaderstoragebuffer_p.h>
#include <QtQuick3D/private/qquick3dshaderimage_p.h>
#include <QtQuick3D/private/qquick3dcomputepass_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendercomputepass_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgshadermaterialadapter_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ComputePass
    \inherits Object3D
    \inqmlmodule QtQuick3D
    \brief Defines a custom compute shader pass.
    \since 6.13

    A ComputePass runs a GLSL compute shader as part of the 3D scene's
    rendering pipeline. Unlike a \l RenderPass or an \l Effect, a
    ComputePass does not render any geometry itself; it is used to prepare
    or process data that is later consumed by a material, an \l Effect, or
    another ComputePass -- typically data written into a \l ShaderImage or
    a \l ShaderStorageBuffer.

    A ComputePass becomes active for a scene when it is placed as a child
    of a \l View3D, a \l Node, or another pass. Within a single frame, a
    ComputePass runs after the scene's internal preparation passes, which
    produce the depth, normal, and ambient occlusion textures, the shadow
    maps, the screen texture, the reflection probes, and the textures of
    texture provider extensions, and before the scene's main opaque and
    transparent rendering. Its results are therefore visible to the main
    rendering and the effects of the same frame, but not to those
    preparation passes.

    \note If the results of a ComputePass change what the preparation passes
    render, for example a buffer that a \l CustomMaterial uses to displace
    vertices, those passes use the results of the previous frame: the shadow
    maps, \c DEPTH_TEXTURE, \c SCREEN_TEXTURE, the ambient occlusion, and
    the reflection probes are one frame behind the main rendering, and see
    uninitialized content in the first frame.

    The order against other passes, \l RenderPass and ComputePass alike, is
    expressed by nesting: a nested pass runs before its parent, so a pass
    that produces data for another one is declared inside its consumer. The
    relative order of passes at the same nesting depth is not defined. See
    \l{Pass Ordering} for the details.

    \note A ComputePass requires a graphics API with compute support:
    OpenGL 4.3 or newer, OpenGL ES 3.1 or newer, Vulkan, Metal, or Direct3D.
    On a backend without compute support, Qt Quick 3D skips the pass
    and issues a warning.

    \note OpenGL ES 3.1 only guarantees four shader storage buffers and four
    images per compute shader. Using the \l{Lights}{light data} takes two of
    those storage buffers. \l ShaderImage also has format and memory
    qualifier restrictions on OpenGL ES; see \l{ShaderImage::format}.

    The following example dispatches a compute shader that fills a
    \l ShaderImage with a solid color, one invocation per texel:

    \qml
    import QtQuick3D

    View3D {
        ShaderImage {
            id: myImage
            width: 256
            height: 256
        }

        ComputePass {
            computeShader: "fill.comp"
            property ShaderImage targetImage: myImage
            commands: [
                Dispatch {
                    groupCountX: (myImage.width + 7) / 8
                    groupCountY: (myImage.height + 7) / 8
                }
            ]
        }
    }
    \endqml

    \section1 Exposing data to the shader

    As with \l RenderPass, \l Effect, and \l CustomMaterial, any QML property declared
    on a ComputePass is automatically exposed as a uniform to its compute shader.
    Properties of type \l ShaderImage are exposed to
    the shader as a GLSL image, declared with the \c SHADER_IMAGE macro, and
    properties of type \l ShaderStorageBuffer as a shader storage buffer,
    declared with the \c STORAGE_BUFFER macro.

    \section1 Built-in uniforms

    The compute shader can also use the following built-in uniforms without
    a matching QML property. Referencing an identifier in the shader source
    binds the corresponding uniform automatically, as in \l CustomMaterial
    and \l Effect shaders:

    \list
    \li \c VIEW_MATRIX -> mat4, the view (camera) matrix.
    \li \c INVERSE_VIEW_MATRIX -> mat4, the inverse of the view matrix.
    \li \c PROJECTION_MATRIX -> mat4, the projection matrix.
    \li \c INVERSE_PROJECTION_MATRIX -> mat4, the inverse of the projection matrix.
    \li \c VIEWPROJECTION_MATRIX -> mat4, the combined view-projection matrix.
    \li \c INVERSE_VIEWPROJECTION_MATRIX -> mat4, the inverse of the view-projection matrix.
    \li \c CAMERA_POSITION -> vec3, the camera position in world space.
    \li \c CAMERA_DIRECTION -> vec3, the camera direction vector.
    \li \c CAMERA_PROPERTIES -> vec3, the near and far clip distances for the camera, in \c x
        and \c y (\c z is unused).
    \li \c FRAMEBUFFER_Y_UP -> float, \c{1.0} or \c{-1.0} depending on whether the graphics API's
        framebuffer coordinate system has Y pointing up.
    \li \c NDC_Y_UP -> float, \c{1.0} or \c{-1.0} depending on whether the graphics API's
        normalized device coordinate system has Y pointing up.
    \li \c NEAR_CLIP_VALUE -> float, \c{0.0} or \c{-1.0} depending on whether the graphics API's
        clip space near plane is at \c{0.0} or \c{-1.0}.
    \li \c FRAME -> float, a frame counter, incremented after each rendered frame in the
        View3D.
    \endlist

    \note With \l{Multiview Rendering}{multiview rendering} active, the camera matrix, position,
    and direction uniforms above become arrays. \c CAMERA_PROPERTIES, \c FRAMEBUFFER_Y_UP,
    \c NDC_Y_UP, \c NEAR_CLIP_VALUE, and \c FRAME do not. See
    \l{ComputePass and multiview rendering}.

    \section1 Screen, depth, and other textures

    As with \l CustomMaterial and \l Effect, referencing one of the following identifiers causes
    the corresponding texture to be prepared and bound automatically:

    \table
        \header
            \li Identifier
            \li Type
            \li Description
        \row
            \li \c SCREEN_TEXTURE, \c SCREEN_MIP_TEXTURE
            \li \c sampler2D
            \li A copy of the scene's opaque color buffer, captured in a dedicated
                pass that runs before the ComputePass itself. Because a ComputePass
                always runs before the scene's main opaque and transparent
                rendering (see above), this does \e{not} include transparent
                objects, the skybox, or anything drawn by an Effect.
        \row
            \li \c DEPTH_TEXTURE
            \li \c sampler2D
            \li The scene's opaque depth buffer, subject to the same opaque-only
                caveat as \c SCREEN_TEXTURE.
        \row
            \li \c AO_TEXTURE
            \li \c sampler2D
            \li The ambient occlusion buffer. Referencing this identifier enables
                the SSAO pass for the layer even if
                \l{SceneEnvironment::aoEnabled}{aoEnabled} is \c false.
        \row
            \li \c NORMAL_ROUGHNESS_TEXTURE
            \li \c sampler2D
            \li A texture with the world-space normals (rgb) and roughness (a) of
                the opaque objects in the scene. Always a plain \c sampler2D, even
                with multiview rendering active.
    \endtable

    \section1 Lights

    A ComputePass exposes light data differently from \l CustomMaterial and
    \l Effect. A CustomMaterial exposes light data through per-light
    callback functions, such as \c DIRECTIONAL_LIGHT(), which the engine
    calls once for each active light. An Effect has no lighting model. A
    compute shader has no current fragment to shade, so a ComputePass
    provides accessor functions that take a light index, and the shader
    loops over the lights itself:

    \table
        \header
            \li Identifier
            \li Type
            \li Description
        \row
            \li \c DIRECTIONAL_LIGHT_COUNT
            \li \c uint
            \li The number of active directional lights.
        \row
            \li \c{DIRECTIONAL_LIGHT_DIFFUSE(index)}
            \li \c vec3
            \li The light's diffuse color.
        \row
            \li \c{DIRECTIONAL_LIGHT_DIRECTION(index)}
            \li \c vec3
            \li The light's direction.
        \row
            \li \c POINT_LIGHT_COUNT
            \li \c uint
            \li The number of active point lights.
        \row
            \li \c{POINT_LIGHT_DIFFUSE(index)}
            \li \c vec3
            \li The light's diffuse color.
        \row
            \li \c{POINT_LIGHT_POSITION(index)}
            \li \c vec3
            \li The light's world-space position.
        \row
            \li \c{POINT_LIGHT_ATTENUATION(index)}
            \li \c vec3
            \li The constant, linear, and quadratic attenuation factors, in \c x,
                \c y, and \c z.
        \row
            \li \c SPOT_LIGHT_COUNT
            \li \c uint
            \li The number of active spot lights.
        \row
            \li \c{SPOT_LIGHT_DIFFUSE(index)}
            \li \c vec3
            \li The light's diffuse color.
        \row
            \li \c{SPOT_LIGHT_POSITION(index)}
            \li \c vec3
            \li The light's world-space position.
        \row
            \li \c{SPOT_LIGHT_ATTENUATION(index)}
            \li \c vec3
            \li The constant, linear, and quadratic attenuation factors, in \c x,
                \c y, and \c z.
        \row
            \li \c{SPOT_LIGHT_DIRECTION(index)}
            \li \c vec3
            \li The light's direction.
        \row
            \li \c{SPOT_LIGHT_CONES(index)}
            \li \c vec2
            \li The cosines of the inner and outer cone angles, in \c x and \c y.
    \endtable

    \c index is a \c uint and is zero-based per light type, so a spot light's index does not
    depend on how many point lights the scene has.

    Referencing any of these identifiers makes all of them available to the shader.

    \note There is no separate intensity or brightness identifier. The colors are linear (not
    sRGB) and have \l{Light::brightness}{brightness} premultiplied into them already.

    Directional lights and positional (point and spot) lights are passed in two separate
    fixed-size shader storage buffers, each holding 16 KB of light data. The resulting
    limits are available to the shader as the compile-time constants
    \c QSSG_MAX_DIRECTIONAL_LIGHTS and \c QSSG_MAX_POSITIONAL_LIGHTS, the latter being a limit
    on the number of point and spot lights \e{combined}. Lights beyond these limits are ignored,
    with a warning.

    The two light buffers use shader storage buffer bindings of their own, so they count
    toward the device's storage buffer limit. OpenGL ES 3.1 only guarantees four per compute
    shader, which leaves two for \l ShaderStorageBuffer properties when the light data is used.

    \section1 Shadows

    If a light with \l{Light::castsShadow}{castsShadow} enabled is present, its shadow map can be
    sampled with one of the following macros, each returning an illumination factor between
    \c{0.0} (fully shadowed) and \c{1.0} (fully lit):

    \list
    \li \c{DIRECTIONAL_LIGHT_SHADOW(index, viewSpaceDepth, worldPosition)} -- for the directional
        light at the same \c index the \c DIRECTIONAL_LIGHT_* accessors above take.
        \c viewSpaceDepth (the world position transformed by \c VIEW_MATRIX, \c{.z}) selects the
        correct cascade.
    \li \c{POINT_LIGHT_SHADOW(index, worldPosition)} -- for the point light at \c index.
    \li \c{SPOT_LIGHT_SHADOW(index, worldPosition)} -- for the spot light at \c index.
    \endlist

    \c QSSG_SHADOW_SOFTNESS is a compile-time constant (not a uniform), reflecting the softest
    \l{Light::softShadowQuality}{softShadowQuality} among the shadow-casting lights in the scene
    (\c{0}, \c{4}, \c{8}, or \c{16}).

    \note The \c QSSG_ prefix marks \c QSSG_MAX_DIRECTIONAL_LIGHTS, \c QSSG_MAX_POSITIONAL_LIGHTS,
    and \c QSSG_SHADOW_SOFTNESS as reflecting the renderer's internals rather than a stable API.
    Their values, and the constants themselves, may change in future Qt versions.

    Referencing a \c{_SHADOW} macro also makes the light identifiers from the previous section
    available, so a shader that only samples shadows does not need to reference any of them.

    \section1 ComputePass and multiview rendering

    Multiview rendering is relevant to a compute shader when it is enabled and the compute shader
    uses any screen textures (\c SCREEN_TEXTURE, \c SCREEN_MIP_TEXTURE, \c DEPTH_TEXTURE, and
    \c AO_TEXTURE) or camera uniforms (any VIEW or PROJECTION matrices, camera position and direction,
    but not properties).

    To tell Qt Quick 3D that the shader supports multiview rendering,
    reference \c QSHADER_VIEW_COUNT in the shader. If it is not referenced, a
    warning is issued.

    When multiview is enabled, all of the built-in camera
    uniforms listed above, except \c CAMERA_PROPERTIES, become arrays of size
    \c QSHADER_VIEW_COUNT, and \c SCREEN_TEXTURE, \c SCREEN_MIP_TEXTURE, \c DEPTH_TEXTURE, and
    \c AO_TEXTURE become \c sampler2DArray instead of \c sampler2D, one layer per eye.
    \c NORMAL_ROUGHNESS_TEXTURE always stays a plain \c sampler2D, even with multiview active.

    A ComputePass has no built-in \c VIEW_INDEX, unlike the fragment shader
    of a \l CustomMaterial or an \l Effect. In those shaders, \c VIEW_INDEX
    derives from the vertex stage's \c gl_ViewIndex, and compute has no
    equivalent. Qt Quick 3D dispatches a ComputePass exactly once,
    regardless of the view count. To produce per-eye results under
    multiview rendering, the compute shader must derive the eye index
    itself, for example from one dispatch dimension. It then writes to a
    \l ShaderImage with \l{ShaderImage::arraySize}{arraySize} set to the
    view count, declared as \c image2DArray in the shader.

    \sa ShaderImage, ShaderStorageBuffer, Dispatch, Effect, CustomMaterial
*/

QQuick3DComputePass::QQuick3DComputePass(QQuick3DObject *parent)
    : QQuick3DObject(*(new QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::ComputePass, QQuick3DObjectPrivate::Flags::RequiresSecondaryUpdate)), parent)
    , QQuick3DPropertyChangedTracker(this, QQuick3DSuperClassInfo<QQuick3DComputePass>())
{
}

QSSGRenderGraphObject *QQuick3DComputePass::updateSpatialNode(QSSGRenderGraphObject *node)
{
    QSSGRenderComputePass *computePassNode = static_cast<QSSGRenderComputePass *>(node);

    if (!computePassNode) {
        computePassNode = new QSSGRenderComputePass;
        markAllDirty();
    }

    auto &uniformProps = computePassNode->computeShader.propertyUniforms;

    if (m_dirtyAttributes & Dirty::ResourceDirty) {
        // Properties -> uniforms.
        // NOTE: Calling extractProperties clears existing properties
        uniformProps = extractProperties();
        static const QMetaMethod onResourceDirty = staticMetaObject.method(staticMetaObject.indexOfSlot("onShaderResourceDirty()"));
        m_shaderResources.trackProperties(staticMetaObject.propertyCount(), onResourceDirty);
        clearDirty(Dirty::ResourceDirty);
        // The images and buffers declared in the shader depend on the properties.
        m_dirtyAttributes |= Dirty::ComputeShaderDirty;
    }

    if (m_dirtyAttributes & Dirty::CommandsDirty) {
        computePassNode->resetCommands();
        for (QQuick3DShaderUtilsRenderCommand *command : std::as_const(m_commands))
            computePassNode->commands.append(command->cloneCommand());
        clearDirty(Dirty::CommandsDirty);
        computePassNode->clearWarnings();
    }

    if (m_dirtyAttributes & Dirty::ComputeShaderDirty) {
        clearDirty(Dirty(Dirty::ComputeShaderDirty | Dirty::PropertyDirty));
        // processMetadata() writes the shader's image and buffer declarations
        // into the property values, so these are resolved again every time the
        // shader is processed.
        QByteArrayList metadata;
        const bool allHaveNodes = QSSGShaderUtils::resolveResourceProperties(this, uniformProps, metadata);
        if (!allHaveNodes)
            markDirty(Dirty::PropertyDirty, true); // Try again next time

        if (!m_computeShader.isEmpty()) {
            const QQmlContext *context = qmlContext(this);
            QByteArray shaderPathKey("compute shader --");
            QByteArray shaderData = QSSGShaderUtils::resolveShader(m_computeShader, context, shaderPathKey);

            computePassNode->computeShader.metadata.clear();
            if (!processMetadata(uniformProps, metadata, computePassNode->computeShader.metadata, shaderData)) {
                qWarning("ComputePass: Shader Image and/or Buffer processing failed in compute shader.");
                return computePassNode;
            }

            computePassNode->computeShader.body = shaderData;
        } else {
            computePassNode->computeShader.body = {};
            computePassNode->computeShader.metadata.clear();
        }
        computePassNode->markDirty(QSSGRenderComputePass::DirtyFlag::ShaderDirty);
        computePassNode->clearWarnings();
    } else if (m_dirtyAttributes & Dirty::PropertyDirty) {
        clearDirty(Dirty::PropertyDirty);
        if (!m_shaderResources.updateResources())
            markDirty(Dirty::PropertyDirty, true); // Try again next time
        QSSGShaderUtils::updateResourceProperties(this, uniformProps);
    }
    return computePassNode;
}

void QQuick3DComputePass::itemChange(ItemChange change, const ItemChangeData &value)
{
    if (change == QQuick3DObject::ItemSceneChange) {
        m_shaderResources.updateSceneManager(value.sceneManager);
    } else if (change == QQuick3DObject::ItemParentHasChanged) {
        // The parent determines the pass's validity and nesting depth, both
        // derived in synchronize(). A same-manager reparent does not travel
        // through pass registration or removal, so mark the set dirty here.
        if (QQuick3DSceneManager *sceneManager = QQuick3DObjectPrivate::get(this)->sceneManager)
            sceneManager->userRenderPassesDirty = true;
    }
}

void QQuick3DComputePass::markTrackedPropertyDirty(QMetaProperty property, DirtyPropertyHint hint)
{
    Q_UNUSED(property);

    // FIXME: As with the property tracking for Effects and Custom materials we
    // should really track which property changed and only update that one.
    if (hint == DirtyPropertyHint::Reference) {
        // FIXME: We should verify that the property is actually a texture property.
        markDirty(Dirty::ResourceDirty);
    } else {
        markDirty(Dirty::PropertyDirty);
    }
}

void QQuick3DComputePass::onShaderResourceDirty()
{
    // Swapping one object for another only changes the binding, but a property
    // gaining or losing its object, or an image of another format, changes the
    // generated shader.
    if (m_shaderResources.hasResourceLayoutChanged())
        markResourcesDirty();
    else
        markDirty(Dirty::PropertyDirty);
}

void QQuick3DComputePass::markResourcesDirty()
{
    // extractProperties() only re-reads dirty properties, and one that was
    // null at the first extraction has no watcher of its own.
    for (int pid : m_shaderResources.resourceProperties())
        addDirtyProperty(pid);
    markDirty(Dirty::ResourceDirty);
}

void QQuick3DComputePass::markDirty(DirtyT type,  bool requestSecondaryUpdate)
{
    if (!(m_dirtyAttributes & type)) {
        m_dirtyAttributes |= type;
        update();
    }

    if (requestSecondaryUpdate)
        QQuick3DObjectPrivate::get(this)->requestSecondaryUpdate();
}

void QQuick3DComputePass::clearDirty(DirtyT type)
{
    m_dirtyAttributes &= ~type;
}

void QQuick3DComputePass::onCommandChanged()
{
    markDirty(CommandsDirty);
}

void QQuick3DComputePass::qmlAppendCommand(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list, QQuick3DShaderUtilsRenderCommand *command)
{
    if (!command)
        return;

    QQuick3DComputePass *that = qobject_cast<QQuick3DComputePass *>(list->object);

    if (!command->parentItem())
        command->setParentItem(that);

    that->m_commands.push_back(command);
    that->markDirty(CommandsDirty);

    QObject::connect(command, &QQuick3DShaderUtilsRenderCommand::changed, that, &QQuick3DComputePass::onCommandChanged);
}

QQuick3DShaderUtilsRenderCommand *QQuick3DComputePass::qmlCommandAt(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list, qsizetype index)
{
    QQuick3DComputePass *that = qobject_cast<QQuick3DComputePass *>(list->object);
    return that->m_commands.at(index);
}

qsizetype QQuick3DComputePass::qmlCommandCount(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list)
{
    QQuick3DComputePass *that = qobject_cast<QQuick3DComputePass *>(list->object);
    return that->m_commands.size();
}

void QQuick3DComputePass::qmlCommandClear(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list)
{
    QQuick3DComputePass *that = qobject_cast<QQuick3DComputePass *>(list->object);
    for (QQuick3DShaderUtilsRenderCommand *cmd : std::as_const(that->m_commands))
        QObject::disconnect(cmd, nullptr, that, nullptr);
    that->m_commands.clear();
    that->markDirty(CommandsDirty);
}

/*!
    \qmlproperty list<Command> ComputePass::commands
    This property holds the list of commands for the compute pass.

    The commands in the list are executed in the order they appear in the
    list. The only commands relevant to a ComputePass are
    \l Dispatch and \l AddDefine.

    Dispatch specifies how many work groups to dispatch the
    compute shader with. A ComputePass without a Dispatch command does not
    execute its shader. If more than one Dispatch command is present, only
    the first one is executed. Setting any group count of the Dispatch
    command to \c 0 disables the ComputePass.

    AddDefine injects a \c {#define} into the shader compilation for
    a \l ComputePass. This lets you branch shader behavior at compile time
    depending on which pass is being executed.

    \sa Dispatch, AddDefine
*/

QQmlListProperty<QQuick3DShaderUtilsRenderCommand> QQuick3DComputePass::commands()
{
    return QQmlListProperty<QQuick3DShaderUtilsRenderCommand>(this,
                                                              nullptr,
                                                              QQuick3DComputePass::qmlAppendCommand,
                                                              QQuick3DComputePass::qmlCommandCount,
                                                              QQuick3DComputePass::qmlCommandAt,
                                                              QQuick3DComputePass::qmlCommandClear);
}

/*!
    \qmlproperty url ComputePass::computeShader
    This property holds the URL of the compute shader used by the pass.

    As with the shader properties of \l RenderPass, \l Effect, and
    \l CustomMaterial, relative URLs are resolved relative to the
    component's (the \c{.qml} file's) location, and it is possible to omit
    the \c file scheme.
*/
QUrl QQuick3DComputePass::computeShader() const
{
    return m_computeShader;
}

void QQuick3DComputePass::setComputeShader(const QUrl &computeShader)
{
    if (m_computeShader == computeShader)
        return;
    m_computeShader = computeShader;
    emit computeShaderChanged();
    markDirty(ComputeShaderDirty);
}

QT_END_NAMESPACE
