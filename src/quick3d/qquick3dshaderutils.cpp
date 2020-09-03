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
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlfile.h>

#include <QtQuick3D/private/qquick3dmaterial_p.h>
#include <QtQuick3D/private/qquick3deffect_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Shader
    \inherits Object
    \inqmlmodule QtQuick3D
    \brief Container component for defining shader code used by post-processing effects.
*/
/*!
    \qmlproperty url Shader::shader
    Specifies the name of the shader source file.
*/
/*!
    \qmlproperty enumeration Shader::stage
    Specifies the shader stage. The default is \c Shader.Fragment

    \value Shader.Vertex The shader is a vertex shader
    \value Shader.Fragment The shader is a fragment shader
*/

/*!
    \qmltype TextureInput
    \inherits Object
    \inqmlmodule QtQuick3D
    \brief Specifies a texture exposed to the shaders of a CustomMaterial or Effect.
*/
/*!
    \qmlproperty Texture TextureInput::texture
    References the texture.
*/
/*!
    \qmlproperty bool TextureInput::enabled
    The property determines if this TextureInput is enabled. The default value
    is true. When disabled, the shaders of the effect sample a dummy, opaque
    black texture instead of the one specified by \l texture.
*/

/*!
    \qmltype Pass
    \inherits Object
    \inqmlmodule QtQuick3D
    \brief Defines a render pass in an Effect.
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
    \brief Defines a command to be performed in a pass of an Effect.
*/

/*!
    \qmltype BufferInput
    \inherits Command
    \inqmlmodule QtQuick3D
    \brief Defines an input texture to be used for a pass of an Effect.
*/
/*!
    \qmlproperty Buffer BufferInput::buffer
    Specifies the \l {Buffer}{buffer} used for the parameter. When not set,
    the associated texture will be the effect's input texture.
*/
/*!
    \qmlproperty string BufferInput::sampler
    Specifies the name under which the texture is exposed in the shader.
    When not set, the texture is exposed with the built-in name \c INPUT.
*/

/*!
    \qmltype Buffer
    \inherits Object
    \inqmlmodule QtQuick3D
    \brief Defines a color buffer to be used for a pass of an Effect.

    In practice a Buffer is backed by a texture, unless the \l name property is
    empty.
*/
/*!
    \qmlproperty enumeration Buffer::format
    Specifies the texture format. The default value is RGBA8.

    \value Buffer.RGBA8
    \value Buffer.RGBA16F
    \value Buffer.RGBA32F
    \value Buffer.R8
    \value Buffer.R16
    \value Buffer.R16F
    \value Buffer.R32F
*/
/*!
    \qmlproperty enumeration Buffer::textureFilterOperation
    Specifies the texture filtering mode when sampling the texture backing the
    Buffer.  The default value is Linear.

    \value Buffer.Nearest Use nearest-neighbor.
    \value Buffer.Linear Use linear filtering.
*/
/*!
    \qmlproperty enumeration Buffer::textureCoordOperation
    Specifies the behavior for texture coordinates outside the [0, 1] range. The default is ClampToEdge.

    \value Buffer.ClampToEdge Clamp coordinate to edge.
    \value Buffer.MirroredRepeat Repeat the coordinate, but flip direction at the beginning and end.
    \value Buffer.Repeat Repeat the coordinate always from the beginning.
*/
/*!
    \qmlproperty real Buffer::sizeMultiplier
    Specifies the size multiplier of the buffer. \c 1.0 creates a buffer with
    the same size as the effect's input texture while \c 0.5 creates buffer with
    the width and height halved. The default value is 1.0
*/
/*!
    \qmlproperty enumeration Buffer::bufferFlags
    Specifies the buffer allocation flags.

    \value Buffer.None No special behavior, this is the default.
    \value Buffer.SceneLifetime The buffer is allocated for the whole lifetime of the scene.
*/
/*!
    \qmlproperty string Buffer::name
    Specifies the name of the buffer.

    It must be set to a non-empty value in most cases. An empty name refers to
    the default output texture of an effect render pass. This is useful to
    override certain settings of the output, such as the texture format,
    without introducing a new, separate intermediate texture.
*/

/*!
    \qmltype SetUniformValue
    \inherits Command
    \inqmlmodule QtQuick3D
    \brief Defines a value to be set during a single \l {Pass}{pass}.
    \since 5.15

    \note The value set by this command is will only be set during the \l {Pass}{pass} it occurs in.
    For consecutive passes the value will be revert to the initial value of the uniform as it
    was defined in the Effect item.
*/
/*!
    \qmlproperty string SetUniformValue::target
    Specifies the name of the uniform that will have its value changed during the \l {Pass}{pass}.
    This must match the name of an existing property in the \l Effect.
*/
/*!
    \qmlproperty Variant SetUniformValue::value
    Specifies the value that will be set on the \c target uniform.
*/

QByteArray QSSGShaderUtils::resolveShader(const QUrl &fileUrl, const QQmlContext *context, QByteArray &shaderPathKey)
{
    if (!shaderPathKey.isEmpty())
        shaderPathKey.append('>');

    const QUrl loadUrl = context ? context->resolvedUrl(fileUrl) : fileUrl;
    const QString filename = QQmlFile::urlToLocalFileOrQrc(loadUrl);

    QFile f(filename);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        shaderPathKey += filename.toLatin1();
        return f.readAll();
    } else {
        qWarning("Failed to read shader code from %s", qPrintable(filename));
    }

    return QByteArray();
}

QQuick3DShaderUtilsBuffer::TextureFormat QQuick3DShaderUtilsBuffer::mapRenderTextureFormat(QSSGRenderTextureFormat::Format fmt)
{
    using TextureFormat = QQuick3DShaderUtilsBuffer::TextureFormat;
    switch (fmt) {
    case QSSGRenderTextureFormat::RGBA8: return TextureFormat::RGBA8;
    case QSSGRenderTextureFormat::RGBA16F: return TextureFormat::RGBA16F;
    case QSSGRenderTextureFormat::RGBA32F: return TextureFormat::RGBA32F;
    case QSSGRenderTextureFormat::R8: return TextureFormat::R8;
    case QSSGRenderTextureFormat::R16: return TextureFormat::R16;
    case QSSGRenderTextureFormat::R16F: return TextureFormat::R16F;
    case QSSGRenderTextureFormat::R32F: return TextureFormat::R32F;
    default:
        break;
    }
    return TextureFormat::Unknown;
}

QSSGRenderTextureFormat::Format QQuick3DShaderUtilsBuffer::mapTextureFormat(QQuick3DShaderUtilsBuffer::TextureFormat fmt)
{
    using TextureFormat = QQuick3DShaderUtilsBuffer::TextureFormat;
    switch (fmt) {
    case TextureFormat::RGBA8: return QSSGRenderTextureFormat::RGBA8;
    case TextureFormat::RGBA16F: return QSSGRenderTextureFormat::RGBA16F;
    case TextureFormat::RGBA32F: return QSSGRenderTextureFormat::RGBA32F;
    case TextureFormat::R8: return QSSGRenderTextureFormat::R8;
    case TextureFormat::R16: return QSSGRenderTextureFormat::R16;
    case TextureFormat::R16F: return QSSGRenderTextureFormat::R16F;
    case TextureFormat::R32F: return QSSGRenderTextureFormat::R32F;
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

    // An append implementation CANNOT rely on the object (shader in this case)
    // being complete. When the list references a Shader object living under
    // another Effect, its properties may not be set at the point of this
    // function being called, so accessing shader->stage is not allowed since
    // it may still have its default value, not what is set from QML...

    // the only thing we can do is to append to our list, do not try to be clever
    that->m_shaders.append(shader);
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
    that->m_shaders.clear();
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
    Q_EMIT textureChanged();
}

QT_END_NAMESPACE
