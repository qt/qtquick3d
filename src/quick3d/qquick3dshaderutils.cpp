/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qquick3dshaderutils_p.h"

#include <QtCore/qfile.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlcontext.h>

#include <QtQuick3D/private/qquick3dmaterial_p.h>
#include <QtQuick3D/private/qquick3deffect_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Shader
    \inherits Object
    \inqmlmodule QtQuick3D
    \brief Container component for defining shader code used by CustomMaterials and Effects.
*/
/*!
    \qmlproperty string Shader::shader
    Specifies the name of the shader source file.
*/
/*!
    \qmlproperty enumeration Shader::stage
    Specifies the shader stage. The default is \c Shader.Shared

    \value Shader.Shared The shader can be shared among different stages
    \value Shader.Vertex The shader is a vertex shader
    \value Shader.Fragment The shader is a fragment shader
    \value Shader.Geometry The shader is a geometry shader
    \value Shader.Compute The shader is a compute shader
*/

/*!
    \qmltype ShaderInfo
    \inherits Object
    \inqmlmodule QtQuick3D
    \brief Defines basic information about custom shader code for CustomMaterials.
*/
/*!
    \qmlproperty string ShaderInfo::version
    Specifies the shader code version.
*/
/*!
    \qmlproperty string ShaderInfo::type
    Specifies the shader code type.
*/
/*!
    \qmlproperty string ShaderInfo::shaderKey
    Specifies the options used by the shader using the combination of shader key values.

    \value ShaderInfo.Diffuse The shader uses diffuse lighting.
    \value ShaderInfo.Specular The shader uses specular lighting.
    \value ShaderInfo.Cutout The shader uses alpha cutout.
    \value ShaderInfo.Refraction The shader uses refraction.
    \value ShaderInfo.Transparent The shader uses transparency.
    \value ShaderInfo.Displace The shader uses displacement mapping.
    \value ShaderInfo.Transmissive The shader uses transmissiveness.
    \value ShaderInfo.Glossy The shader is default glossy. This is a combination of \c ShaderInfo.Diffuse and
        \c ShaderInfo.Specular.
*/

/*!
    \qmltype TextureInput
    \inherits Object
    \inqmlmodule QtQuick3D
    \brief Defines a texture channel for a Custom Material or an Effect.
*/
/*!
    \qmlproperty Texture TextureInput::texture
    Specifies the Texture to input.
*/
/*!
    \qmlproperty bool TextureInput::enabled
    The property determines if this TextureInput is enabled.
*/

/*!
    \qmltype Pass
    \inherits Object
    \inqmlmodule QtQuick3D
    \brief Defines a render pass in the CustomMaterial or the Effect.
*/
/*!
    \qmlproperty Buffer Pass::output
    Specifies the output \l {Buffer}{buffer} of the pass.
*/
/*!
    \qmlproperty list Pass::commands
    Specifies the list of render \l {Command}{commands} of the pass.
*/
/*!
    \qmlproperty list Pass::shaders
    Specifies the list of \l {Shader}{shaders} of the pass.
*/

/*!
    \qmltype Command
    \inherits Object
    \inqmlmodule QtQuick3D
    \brief Defines a command to be performed in a pass of a CustomMaterial or an Effect.
*/

/*!
    \qmltype BufferInput
    \inherits Command
    \inqmlmodule QtQuick3D
    \brief Defines an input buffer to be used for a pass of a CustomMaterial or an Effect.
*/
/*!
    \qmlproperty Buffer BufferInput::buffer
    Specifies the \l {Buffer}{buffer} used for the parameter.
*/
/*!
    \qmlproperty string BufferInput::param
    Specifies the name of the input parameter in the shader.
*/

/*!
    \qmltype BufferBlit
    \inherits Command
    \inqmlmodule QtQuick3D
    \brief Defines a copy operation between two buffers in a pass of a CustomMaterial or an Effect.
*/
/*!
    \qmlproperty Buffer BufferBlit::source
    Specifies the source \l {Buffer}{buffer} of the copy operation.
*/
/*!
    \qmlproperty Buffer BufferBlit::destination
    Specifies the destination \l {Buffer}{buffer} of the copy operation.
*/

/*!
    \qmltype Blending
    \inherits Command
    \inqmlmodule QtQuick3D
    \brief Defines the blending state in a pass of a CustomMaterial or an Effect.
*/
/*!
    \qmlproperty enumeration Blending::srcBlending
    Specifies the source blending function.

    \value Blending.Unknown
    \value Blending.Zero
    \value Blending.One
    \value Blending.SrcColor
    \value Blending.OneMinusSrcColor
    \value Blending.DstColor
    \value Blending.OneMinusDstColor
    \value Blending.SrcAlpha
    \value Blending.OneMinusSrcAlpha
    \value Blending.DstAlpha
    \value Blending.OneMinusDstAlpha
    \value Blending.ConstantColor
    \value Blending.OneMinusConstantColor
    \value Blending.ConstantAlpha
    \value Blending.OneMinusConstantAlpha
    \value Blending.SrcAlphaSaturate

*/
/*!
    \qmlproperty enumeration Blending::destBlending
    Specifies the destination blending function.

    \value Blending.Unknown
    \value Blending.Zero
    \value Blending.One
    \value Blending.SrcColor
    \value Blending.OneMinusSrcColor
    \value Blending.DstColor
    \value Blending.OneMinusDstColor
    \value Blending.SrcAlpha
    \value Blending.OneMinusSrcAlpha
    \value Blending.DstAlpha
    \value Blending.OneMinusDstAlpha
    \value Blending.ConstantColor
    \value Blending.OneMinusConstantColor
    \value Blending.ConstantAlpha
    \value Blending.OneMinusConstantAlpha
*/

/*!
    \qmltype Buffer
    \inherits Object
    \inqmlmodule QtQuick3D
    \brief Defines a buffer to be used for a pass of a CustomMaterial or an Effect.
*/
/*!
    \qmlproperty enumeration Buffer::format
    Specifies the buffer format.

    \value Buffer.Unknown
    \value Buffer.R8
    \value Buffer.R16
    \value Buffer.R16F
    \value Buffer.R32I
    \value Buffer.R32UI
    \value Buffer.R32F
    \value Buffer.RG8
    \value Buffer.RGBA8
    \value Buffer.RGB8
    \value Buffer.SRGB8
    \value Buffer.SRGB8A8
    \value Buffer.RGB565
    \value Buffer.RGBA16F
    \value Buffer.RG16F
    \value Buffer.RG32F
    \value Buffer.RGB32F
    \value Buffer.RGBA32F
    \value Buffer.R11G11B10
    \value Buffer.RGB9E5
    \value Buffer.Depth16
    \value Buffer.Depth24
    \value Buffer.Depth32
    \value Buffer.Depth24Stencil8
*/
/*!
    \qmlproperty enumeration Buffer::textureFilterOperation
    Specifies the filter operation when a render \l {Pass}{pass} is reading the buffer that is
    different size as the current output buffer.

    \value Buffer.Unknown Value not set.
    \value Buffer.Nearest Use nearest-neighbor.
    \value Buffer.Linear Use linear filtering.
*/
/*!
    \qmlproperty enumeration Buffer::textureCoordOperation
    Specifies the texture coordinate operation for coordinates outside [0, 1] range.

    \value Buffer.Unknown Value not set.
    \value Buffer.ClampToEdge Clamp coordinate to edge.
    \value Buffer.MirroredRepeat Repeat the coordinate, but flip direction at the beginning and end.
    \value Buffer.Repeat Repeat the coordinate always from the beginning.
*/
/*!
    \qmlproperty real Buffer::sizeMultiplier
    Specifies the size multiplier of the buffer. \c 1.0 creates buffer with the same size while
    \c 0.5 creates buffer with width and height halved.
*/
/*!
    \qmlproperty enumeration Buffer::bufferFlags
    Specifies the buffer allocation flags.

    \value Buffer.None Value not set.
    \value Buffer.SceneLifetime The buffer is allocated for the whole lifetime of the scene.
*/
/*!
    \qmlproperty string Buffer::name
    Specifies the name of the buffer
*/

/*!
    \qmltype RenderState
    \inherits Command
    \inqmlmodule QtQuick3D
    \brief Defines the render state to be disabled in a pass of a CustomMaterial or an Effect.
*/
/*!
    \qmlproperty enumeration RenderState::renderState
    Specifies the render state to enable/disable in a \l {Pass}{pass}.

    \value RenderState.Unknown
    \value RenderState.Blend
    \value RenderState.CullFace
    \value RenderState.DepthTest
    \value RenderState.StencilTest
    \value RenderState.ScissorTest
    \value RenderState.DepthWrite
    \value RenderState.Multisample
*/
/*!
    \qmlproperty bool RenderState::enable
    Specifies if the state is enabled or disabled.
*/

/*!
    \qmltype CullMode
    \inherits Command
    \inqmlmodule QtQuick3D
    \brief Defines the cull mode for render pass.
    \since 5.15

    \note This command can only be used with the CustomMaterial.
*/
/*!
    \qmlproperty enumeration CullMode::cullMode
    Specifies the culling mode in a \l {Pass}{pass} when \c RenderState.CullFace is enabled.
    The material culling mode is overridden.

    \value Material.BackFaceCulling
    \value Material.FrontFaceCulling
    \value Material.NoCulling
*/

/*!
    \qmltype DepthInput
    \inherits Command
    \inqmlmodule QtQuick3D
    \brief Defines the output texture for the depth buffer.
    \since 5.15
*/
/*!
    \qmlproperty string DepthInput::param
    Specifies the name of the texture the depth buffer will bind to.
*/

/*!
    \qmltype SetUniformValue
    \inherits Command
    \inqmlmodule QtQuick3D
    \brief Defines a value to be set during a single \l {Pass}{pass}.
    \since 5.15

    \note The value set by this command is will only be set during the \l {Pass}{pass} it occurs in.
    For consecutive passes the value will be revert to the initial value of the uniform as it
    was defined in the effect or custom material item.
*/
/*!
    \qmlproperty string SetUniformValue::target
    Specifies the name of the uniform that will have its value changed during the \l {Pass}{pass}.
*/
/*!
    \qmlproperty Variant SetUniformValue::value
    Specifies the value that will be set on the \c target uniform.
*/

void QSSGShaderUtils::addSnapperSampler(const QByteArray &texName, QByteArray &shaderPrefix)
{
    const char *filter = "linear";
    const char *clamp = "clamp";
    // Output macro so we can change the set of variables used for this
    // independent of the
    // meta data system.
    shaderPrefix.append("SNAPPER_SAMPLER2D(");
    shaderPrefix.append(texName);
    shaderPrefix.append(", ");
    shaderPrefix.append(texName);
    shaderPrefix.append(", ");
    shaderPrefix.append(filter);
    shaderPrefix.append(", ");
    shaderPrefix.append(clamp);
    shaderPrefix.append(", ");
    shaderPrefix.append("false )\n");
}

QByteArray QSSGShaderUtils::resolveShader(const QByteArray &shader, QByteArray &shaderPath,
                                          const QObject *qmlObj)
{
    if (!shaderPath.isEmpty())
        shaderPath.append('>');

    int offset = -1;
    if (shader.startsWith("qrc:/"))
        offset = 3;
    else if (shader.startsWith("file:/"))
        offset = 6;
    else if (shader.startsWith(":/"))
        offset = 0;

    QString path;
    if (offset == -1) {
        QUrl u(QString::fromUtf8(shader));
        if (u.isLocalFile())
            path = u.toLocalFile();
    }

    if (offset == -1 && path.isEmpty())
        path = QString::fromLatin1(":/") + QString::fromLocal8Bit(shader);
    else
        path = QString::fromLocal8Bit(shader.constData() + offset);

    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        shaderPath += path.toLatin1();
        return f.readAll();
    } else if (offset == -1) {
        // Plain schemeless string can also be a local file instead of resource, so let's try to
        // load a local file relative to qml context
        QQmlContext *context = qmlContext(qmlObj);
        if (context) {
            QUrl resolvedUrl = context->resolvedUrl(QUrl(QString::fromUtf8(shader)));
            path = resolvedUrl.toLocalFile();
            QFile file(path);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                shaderPath += path.toLatin1();
                return file.readAll();
            }
        }
    }

    shaderPath += QByteArrayLiteral("Inline_") + QByteArray::number(qHash(shader, uint(qGlobalQHashSeed())));

    return shader;
}

QByteArray QSSGShaderUtils::mergeShaderCode(const QByteArray &shared,
                                            const QByteArray &uniforms,
                                            const QByteArray &textures,
                                            const QByteArray &vertex,
                                            const QByteArray &geometry,
                                            const QByteArray &fragment)
{
    QByteArray shaderCode;
    // Shared
    if (!shared.isEmpty())
        shaderCode.append(shared);

    if (!textures.isEmpty())
        shaderCode.append(textures);

    if (!uniforms.isEmpty())
        shaderCode.append(uniforms);

    // Vetex
    shaderCode.append(QByteArrayLiteral("\n#ifdef VERTEX_SHADER\n"));
    if (!vertex.isEmpty())
        shaderCode.append(vertex);
    else
        shaderCode.append(QByteArrayLiteral("void vert(){}"));
    shaderCode.append(QByteArrayLiteral("\n#endif\n"));

    // Geometry
    if (!geometry.isEmpty()) {
        shaderCode.append(QByteArrayLiteral("\n#ifdef USER_GEOMETRY_SHADER\n"));
        shaderCode.append(geometry);
        shaderCode.append(QByteArrayLiteral("\n#endif\n"));
    }

    // Fragment
    shaderCode.append(QByteArrayLiteral("\n#ifdef FRAGMENT_SHADER\n"));
    if (!fragment.isEmpty())
        shaderCode.append(fragment);
    else
        shaderCode.append(QByteArrayLiteral("void frag(){}"));
    shaderCode.append(QByteArrayLiteral("\n#endif\n"));

    return shaderCode;
}

QQuick3DShaderUtilsBuffer::TextureFormat QQuick3DShaderUtilsBuffer::mapRenderTextureFormat(QSSGRenderTextureFormat::Format fmt)
{
    using TextureFormat = QQuick3DShaderUtilsBuffer::TextureFormat;
    switch (fmt) {
    case QSSGRenderTextureFormat::R8: return TextureFormat::R8;
    case QSSGRenderTextureFormat::R16: return TextureFormat::R16;
    case QSSGRenderTextureFormat::R16F: return TextureFormat::R16F;
    case QSSGRenderTextureFormat::R32I: return TextureFormat::R32I;
    case QSSGRenderTextureFormat::R32UI: return TextureFormat::R32UI;
    case QSSGRenderTextureFormat::R32F: return TextureFormat::R32F;
    case QSSGRenderTextureFormat::RG8: return TextureFormat::RG8;
    case QSSGRenderTextureFormat::RGBA8: return TextureFormat::RGBA8;
    case QSSGRenderTextureFormat::RGB8: return TextureFormat::RGB8;
    case QSSGRenderTextureFormat::SRGB8: return TextureFormat::SRGB8;
    case QSSGRenderTextureFormat::SRGB8A8: return TextureFormat::SRGB8A8;
    case QSSGRenderTextureFormat::RGB565: return TextureFormat::RGB565;
    case QSSGRenderTextureFormat::RGBA16F: return TextureFormat::RGBA16F;
    case QSSGRenderTextureFormat::RG16F: return TextureFormat::RG16F;
    case QSSGRenderTextureFormat::RG32F: return TextureFormat::RG32F;
    case QSSGRenderTextureFormat::RGB32F: return TextureFormat::RGB32F;
    case QSSGRenderTextureFormat::RGBA32F: return TextureFormat::RGBA32F;
    case QSSGRenderTextureFormat::R11G11B10: return TextureFormat::R11G11B10;
    case QSSGRenderTextureFormat::RGB9E5: return TextureFormat::RGB9E5;
    case QSSGRenderTextureFormat::Depth16: return TextureFormat::Depth16;
    case QSSGRenderTextureFormat::Depth24: return TextureFormat::Depth24;
    case QSSGRenderTextureFormat::Depth32: return TextureFormat::Depth32;
    case QSSGRenderTextureFormat::Depth24Stencil8: return TextureFormat::Depth24Stencil8;
    default:
        break;
    }
    return TextureFormat::Unknown;
}

QSSGRenderTextureFormat::Format QQuick3DShaderUtilsBuffer::mapTextureFormat(QQuick3DShaderUtilsBuffer::TextureFormat fmt)
{
    using TextureFormat = QQuick3DShaderUtilsBuffer::TextureFormat;
    switch (fmt) {
    case TextureFormat::R8: return QSSGRenderTextureFormat::R8;
    case TextureFormat::R16: return QSSGRenderTextureFormat::R16;
    case TextureFormat::R16F: return QSSGRenderTextureFormat::R16F;
    case TextureFormat::R32I: return QSSGRenderTextureFormat::R32I;
    case TextureFormat::R32UI: return QSSGRenderTextureFormat::R32UI;
    case TextureFormat::R32F: return QSSGRenderTextureFormat::R32F;
    case TextureFormat::RG8: return QSSGRenderTextureFormat::RG8;
    case TextureFormat::RGBA8: return QSSGRenderTextureFormat::RGBA8;
    case TextureFormat::RGB8: return QSSGRenderTextureFormat::RGB8;
    case TextureFormat::SRGB8: return QSSGRenderTextureFormat::SRGB8;
    case TextureFormat::SRGB8A8: return QSSGRenderTextureFormat::SRGB8A8;
    case TextureFormat::RGB565: return QSSGRenderTextureFormat::RGB565;
    case TextureFormat::RGBA16F: return QSSGRenderTextureFormat::RGBA16F;
    case TextureFormat::RG16F: return QSSGRenderTextureFormat::RG16F;
    case TextureFormat::RG32F: return QSSGRenderTextureFormat::RG32F;
    case TextureFormat::RGB32F: return QSSGRenderTextureFormat::RGB32F;
    case TextureFormat::RGBA32F: return QSSGRenderTextureFormat::RGBA32F;
    case TextureFormat::R11G11B10: return QSSGRenderTextureFormat::R11G11B10;
    case TextureFormat::RGB9E5: return QSSGRenderTextureFormat::RGB9E5;
    case TextureFormat::Depth16: return QSSGRenderTextureFormat::Depth16;
    case TextureFormat::Depth24: return QSSGRenderTextureFormat::Depth24;
    case TextureFormat::Depth32: return QSSGRenderTextureFormat::Depth32;
    case TextureFormat::Depth24Stencil8: return QSSGRenderTextureFormat::Depth24Stencil8;
    default:
        break;
    }
    return QSSGRenderTextureFormat::Unknown;
}

QQuick3DShaderUtilsBuffer::TextureFormat QQuick3DShaderUtilsBuffer::format() const
{
    return mapRenderTextureFormat(command.m_format.format);
}

void QQuick3DShaderUtilsBuffer::setFormat(TextureFormat format)
{
    command.m_format = mapTextureFormat(format);
}

void QQuick3DShaderUtilsRenderPass::qmlAppendCommand(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list,
                                                     QQuick3DShaderUtilsRenderCommand *command)
{
    if (!command)
        return;

    QQuick3DShaderUtilsRenderPass *that = qobject_cast<QQuick3DShaderUtilsRenderPass *>(list->object);
    that->m_commands.push_back(command);
}

QQuick3DShaderUtilsRenderCommand *QQuick3DShaderUtilsRenderPass::qmlCommandAt(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list,
                                                                              int index)
{
    QQuick3DShaderUtilsRenderPass *that = qobject_cast<QQuick3DShaderUtilsRenderPass *>(list->object);
    return that->m_commands.at(index);
}

int QQuick3DShaderUtilsRenderPass::qmlCommandCount(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list)
{
    QQuick3DShaderUtilsRenderPass *that = qobject_cast<QQuick3DShaderUtilsRenderPass *>(list->object);
    return that->m_commands.count();
}

void QQuick3DShaderUtilsRenderPass::qmlCommandClear(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list)
{
    QQuick3DShaderUtilsRenderPass *that = qobject_cast<QQuick3DShaderUtilsRenderPass *>(list->object);
    that->m_commands.clear();
}

QQmlListProperty<QQuick3DShaderUtilsRenderCommand> QQuick3DShaderUtilsRenderPass::commands()
{
    return QQmlListProperty<QQuick3DShaderUtilsRenderCommand>(this,
                                                             nullptr,
                                                             QQuick3DShaderUtilsRenderPass::qmlAppendCommand,
                                                             QQuick3DShaderUtilsRenderPass::qmlCommandCount,
                                                             QQuick3DShaderUtilsRenderPass::qmlCommandAt,
                                                             QQuick3DShaderUtilsRenderPass::qmlCommandClear);
}

void QQuick3DShaderUtilsRenderPass::qmlAppendShader(QQmlListProperty<QQuick3DShaderUtilsShader> *list,
                                                    QQuick3DShaderUtilsShader *shader)
{
    if (!shader)
        return;

    QQuick3DShaderUtilsRenderPass *that = qobject_cast<QQuick3DShaderUtilsRenderPass *>(list->object);
    that->m_shaders[int(shader->stage)] = shader;
}

QQuick3DShaderUtilsShader *QQuick3DShaderUtilsRenderPass::qmlShaderAt(QQmlListProperty<QQuick3DShaderUtilsShader> *list,
                                                                      int index)
{
    QQuick3DShaderUtilsRenderPass *that = qobject_cast<QQuick3DShaderUtilsRenderPass *>(list->object);
    return that->m_shaders.at(index);
}

int QQuick3DShaderUtilsRenderPass::qmlShaderCount(QQmlListProperty<QQuick3DShaderUtilsShader> *list)
{
    QQuick3DShaderUtilsRenderPass *that = qobject_cast<QQuick3DShaderUtilsRenderPass *>(list->object);
    return that->m_shaders.count();
}

void QQuick3DShaderUtilsRenderPass::qmlShaderClear(QQmlListProperty<QQuick3DShaderUtilsShader> *list)
{
    QQuick3DShaderUtilsRenderPass *that = qobject_cast<QQuick3DShaderUtilsRenderPass *>(list->object);
    auto it = that->m_shaders.begin();
    const auto end = that->m_shaders.end();
    for (;it != end; ++it)
        *it = nullptr;
}

QQmlListProperty<QQuick3DShaderUtilsShader> QQuick3DShaderUtilsRenderPass::shaders()
{
    return QQmlListProperty<QQuick3DShaderUtilsShader>(this,
                                                      nullptr,
                                                      QQuick3DShaderUtilsRenderPass::qmlAppendShader,
                                                      QQuick3DShaderUtilsRenderPass::qmlShaderCount,
                                                      QQuick3DShaderUtilsRenderPass::qmlShaderAt,
                                                      QQuick3DShaderUtilsRenderPass::qmlShaderClear);
}

void QQuick3DShaderUtilsTextureInput::setTexture(QQuick3DTexture *texture)
{
    if (m_texture == texture)
        return;

    QObject *p = parent();
    while (p != nullptr) {
        if (QQuick3DMaterial *mat = qobject_cast<QQuick3DMaterial *>(p)) {
            mat->setDynamicTextureMap(texture, name);
            break;
        } else if (QQuick3DEffect *efx = qobject_cast<QQuick3DEffect *>(p)) {
            efx->setDynamicTextureMap(texture, name);
            break;
        }
        p = p->parent();
    }

    if (p == nullptr) {
        qWarning("A texture was defined out of Material or Effect");
    }

    m_texture = texture;
    Q_EMIT textureDirty(this);
}

QT_END_NAMESPACE
