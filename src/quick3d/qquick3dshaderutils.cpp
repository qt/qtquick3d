// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dshaderutils_p.h"

#include <QtCore/qfile.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlfile.h>

#include "qquick3dviewport_p.h"
#include "qquick3dcustommaterial_p.h"
#include "qquick3deffect_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Shader
    \inherits QtObject
    \inqmlmodule QtQuick3D
    \brief Container component for defining shader code used by post-processing effects.

    The Shader type is used for populating the \l{Pass::shaders}{shaders} list in the
    render \l{Pass}{pass} of an \l Effect.

    A shader is code which is executed directly on the graphic hardware at a particular
    \l{Shader::stage}{stage} of the rendering pipeline.

    \sa Effect
*/
/*!
    \qmlproperty url Shader::shader
    Specifies the name of the shader source file. For details on how to write shader code,
    see the \l Effect documentation.

    \warning Shader snippets are assumed to be trusted content. Application
    developers are advised to carefully consider the potential implications
    before allowing the loading of user-provided content that is not part of the
    application.
*/
/*!
    \qmlproperty enumeration Shader::stage
    Specifies the stage of the rendering pipeline when the shader code will be executed.
    The default is \c Shader.Fragment

    \value Shader.Vertex The shader is a vertex shader. This code is run once per vertex
    in the input geometry and can be used to modify it before the geometry is rasterized
    (scan converted). In the case of effects, the input geometry is always a quad (four
    vertexes representing the corners of the render target).
    \value Shader.Fragment The shader is a fragment shader. After vertex processing,
    the modified geometry is turned into fragments (rasterization). Then a fragment shader
    is executed for each fragment, assigning a color to it. Fragments are a related concept
    to pixels, but with additional information attached. Also, as a result of some
    anti-aliasing strategies, there may be more than one fragment for each pixel in the
    output.
*/

/*!
    \qmltype TextureInput
    \inherits QtObject
    \inqmlmodule QtQuick3D
    \brief Specifies a texture exposed to the shaders of a CustomMaterial or Effect.

    This is a type which can be used for exposing a \l Texture to a shader, either
    in the \l{Pass}{render pass} of an \l Effect, or in a \l CustomMaterial. It exists
    primarily to assign a local name to the \l Texture that can be referenced from
    shaders.

    When a TextureInput property is declared in an \l Effect or a \l CustomMaterial,
    it will automatically be available as a sampler in all shaders by its property
    name.
*/
/*!
    \qmlproperty Texture TextureInput::texture
    The texture for which this TextureInput serves as an indirection.
*/
/*!
    \qmlproperty bool TextureInput::enabled
    The property determines if this TextureInput is enabled. The default value
    is true. When disabled, the shaders of the effect sample a dummy, opaque
    black texture instead of the one specified by \l texture.
*/

/*!
    \qmltype Pass
    \inherits QtObject
    \inqmlmodule QtQuick3D
    \brief Defines a render pass in an Effect.

    An \l Effect may consist of multiple render passes. Each render pass has a
    setup phase where the list of \l{Pass::commands}{render commands} are executed,
    a \l{Pass::output}{output buffer} and a list of \l{Pass::shaders}{shaders} to
    use for rendering the effect.

    See the documentation for \l Effect for more details on how to set up multiple
    rendering passes.
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
    \inherits QtObject
    \inqmlmodule QtQuick3D
    \brief Supertype of commands to be performed as part of a pass in an Effect.

    The Command type should not be instantiated by itself, but only exists as a
    polymorphic supertype for the different actions that can be performed as part
    of a \l{Pass}{render pass}.

    \sa BufferInput, SetUniformValue, Effect
*/

/*!
    \qmltype BufferInput
    \inherits Command
    \inqmlmodule QtQuick3D
    \brief Defines an input buffer to be used as input for a pass of an Effect.

    BufferInput is a \l Command which can be added to the list of commands in the \l Pass of
    an \l Effect. When executed, it will expose the buffer as a sample to the shaders
    in the render pass. The shaders must declare a sampler with the name given in the
    BufferInput's \c sampler property.

    This can be used for sharing intermediate results between the different passes of an
    effect.

    \sa TextureInput
*/
/*!
    \qmlproperty Buffer BufferInput::buffer
    Specifies the \l {Buffer}{buffer} which should be exposed to the shader.
*/
/*!
    \qmlproperty string BufferInput::sampler
    Specifies the name under which the buffer is exposed to the shader.
    When this property is not set, the buffer is exposed with the built-in name \c INPUT.
*/

/*!
    \qmltype Buffer
    \inherits QtObject
    \inqmlmodule QtQuick3D
    \brief Creates or references a color buffer to be used for a pass of an Effect.

    A Buffer can be used to create intermediate buffers to share data between
    \l{Pass}{render passes} in an \l Effect.

    \note If the \l name property of the Buffer is empty, it will reference the
    default output texture of the render pass.
*/
/*!
    \qmlproperty enumeration Buffer::format
    Specifies the texture format. The default value is Buffer.RGBA8.

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
    Specifies the texture filtering mode when sampling the contents of the
    Buffer. The default value is Buffer.Linear.

    \value Buffer.Nearest Use nearest-neighbor filtering.
    \value Buffer.Linear Use linear filtering.
*/
/*!
    \qmlproperty enumeration Buffer::textureCoordOperation
    Specifies the behavior for texture coordinates when sampling outside the [0, 1] range.
    The default is Buffer.ClampToEdge.

    \value Buffer.ClampToEdge Clamp coordinates to the edges.
    \value Buffer.Repeat Wrap the coordinates at the edges to tile the texture.
    \value Buffer.MirroredRepeat Wrap the coordinate at the edges, but mirror the texture
    when tiling it.
*/
/*!
    \qmlproperty real Buffer::sizeMultiplier
    Specifies the size multiplier of the buffer. For instance, a value of \c 1.0 creates
    a buffer with the same size as the effect's input texture while \c 0.5 creates buffer
    where both width and height is half as big. The default value is 1.0.
*/
/*!
    \qmlproperty enumeration Buffer::bufferFlags
    Specifies the buffer allocation flags. The default is Buffer.None.

    \value Buffer.None No special behavior.
    \value Buffer.SceneLifetime The buffer is allocated for the whole lifetime of the scene.
*/
/*!
    \qmlproperty string Buffer::name
    Specifies the name of the buffer.

    \note When this property is empty, the Buffer will refer to the default output texture
    of the \l{Pass}{render pass} instead of allocating a buffer. This can be useful to
    override certain settings of the output, such as the texture format, without introducing
    a new, separate intermediate texture.
*/

/*!
    \qmltype SetUniformValue
    \inherits Command
    \inqmlmodule QtQuick3D
    \brief Defines a value to be set during a single \l {Pass}{pass}.
    \since 5.15

    SetUniformValue is a \l Command which can be added to the list of commands in a \l Pass. When
    executed, it will set the uniform given by the \l{SetUniformValue::target}{target} property
    to \l{SetUniformValue::value}{value}.

    \note The value set by this command is will only be set during the \l {Pass}{pass} it occurs in.
    For consecutive passes the value will be revert to the initial value of the uniform as it
    was defined in the \l Effect item.

    \sa BufferInput
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

namespace QSSGShaderUtils {

ResolveFunction resolveShaderOverride = nullptr;

void setResolveFunction(ResolveFunction fn)
{
    resolveShaderOverride = fn;
}

QByteArray resolveShader(const QUrl &fileUrl, const QQmlContext *context, QByteArray &shaderPathKey)
{
    if (resolveShaderOverride) {
        QByteArray shaderData;
        if (resolveShaderOverride(fileUrl, context, shaderData, shaderPathKey))
            return shaderData;
    }

    if (!shaderPathKey.isEmpty())
        shaderPathKey.append('>');

    const QUrl loadUrl = context ? context->resolvedUrl(fileUrl) : fileUrl;
    const QString filePath = QQmlFile::urlToLocalFileOrQrc(loadUrl);

    QFile f(filePath);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        shaderPathKey += loadUrl.fileName().toUtf8();
        return f.readAll();
    } else {
        qWarning("Failed to read shader code from %s", qPrintable(filePath));
    }

    return QByteArray();
}

// These are the QMetaTypes that we convert into uniforms.
static constexpr QMetaType::Type qssg_metatype_list[] {
        QMetaType::Double,
        QMetaType::Bool,
        QMetaType::QVector2D,
        QMetaType::QVector3D,
        QMetaType::QVector4D,
        QMetaType::Int,
        QMetaType::QColor,
        QMetaType::QSize,
        QMetaType::QSizeF,
        QMetaType::QPoint,
        QMetaType::QPointF,
        QMetaType::QRect,
        QMetaType::QRectF,
        QMetaType::QQuaternion,
        QMetaType::QMatrix4x4
};

template<>
struct ShaderType<QMetaType::Double>
{
    static constexpr QSSGRenderShaderValue::Type type() { return QSSGRenderShaderValue::Float; }
    static QByteArray name() { return QByteArrayLiteral("float"); }
};

template<>
struct ShaderType<QMetaType::Bool>
{
    static constexpr QSSGRenderShaderValue::Type type() { return QSSGRenderShaderValue::Boolean; }
    static QByteArray name() { return QByteArrayLiteral("bool"); }
};

template<>
struct ShaderType<QMetaType::Int>
{
    static constexpr QSSGRenderShaderValue::Type type() { return QSSGRenderShaderValue::Integer; }
    static QByteArray name() { return QByteArrayLiteral("int"); }
};

template<>
struct ShaderType<QMetaType::QVector2D>
{
    static constexpr QSSGRenderShaderValue::Type type() { return QSSGRenderShaderValue::Vec2; }
    static QByteArray name() { return QByteArrayLiteral("vec2"); }
};

template<>
struct ShaderType<QMetaType::QVector3D>
{
    static constexpr QSSGRenderShaderValue::Type type() { return QSSGRenderShaderValue::Vec3; }
    static QByteArray name() { return QByteArrayLiteral("vec3"); }
};

template<>
struct ShaderType<QMetaType::QVector4D>
{
    static constexpr QSSGRenderShaderValue::Type type() { return QSSGRenderShaderValue::Vec4; }
    static QByteArray name() { return QByteArrayLiteral("vec4"); }
};

template<>
struct ShaderType<QMetaType::QColor>
{
    static constexpr QSSGRenderShaderValue::Type type() { return QSSGRenderShaderValue::Rgba; }
    static QByteArray name() { return QByteArrayLiteral("vec4"); }
};

template<>
struct ShaderType<QMetaType::QSize>
{
    static constexpr QSSGRenderShaderValue::Type type() { return QSSGRenderShaderValue::Size; }
    static QByteArray name() { return QByteArrayLiteral("vec2"); }
};

template<>
struct ShaderType<QMetaType::QSizeF>
{
    static constexpr QSSGRenderShaderValue::Type type() { return QSSGRenderShaderValue::SizeF; }
    static QByteArray name() { return QByteArrayLiteral("vec2"); }
};

template<>
struct ShaderType<QMetaType::QPoint>
{
    static constexpr QSSGRenderShaderValue::Type type() { return QSSGRenderShaderValue::Point; }
    static QByteArray name() { return QByteArrayLiteral("vec2"); }
};

template<>
struct ShaderType<QMetaType::QPointF>
{
    static constexpr QSSGRenderShaderValue::Type type() { return QSSGRenderShaderValue::PointF; }
    static QByteArray name() { return QByteArrayLiteral("vec2"); }
};

template<>
struct ShaderType<QMetaType::QRect>
{
    static constexpr QSSGRenderShaderValue::Type type() { return QSSGRenderShaderValue::Rect; }
    static QByteArray name() { return QByteArrayLiteral("vec4"); }
};

template<>
struct ShaderType<QMetaType::QRectF>
{
    static constexpr QSSGRenderShaderValue::Type type() { return QSSGRenderShaderValue::RectF; }
    static QByteArray name() { return QByteArrayLiteral("vec4"); }
};

template<>
struct ShaderType<QMetaType::QQuaternion>
{
    static constexpr QSSGRenderShaderValue::Type type() { return QSSGRenderShaderValue::Quaternion; }
    static QByteArray name() { return QByteArrayLiteral("vec4"); }
};

template<>
struct ShaderType<QMetaType::QMatrix4x4>
{
    static constexpr QSSGRenderShaderValue::Type type() { return QSSGRenderShaderValue::Matrix4x4; }
    static QByteArray name() { return QByteArrayLiteral("mat4"); }
};

QByteArray uniformTypeName(QMetaType type)
{
    switch (type.id()) {
    case QMetaType::Double:
    case QMetaType::Float:
        return ShaderType<QMetaType::Double>::name();
    case QMetaType::Bool:
        return ShaderType<QMetaType::Bool>::name();
    case QMetaType::QVector2D:
        return ShaderType<QMetaType::QVector2D>::name();
    case QMetaType::QVector3D:
        return ShaderType<QMetaType::QVector3D>::name();
    case QMetaType::QVector4D:
        return ShaderType<QMetaType::QVector4D>::name();
    case QMetaType::Int:
        return ShaderType<QMetaType::Int>::name();
    case QMetaType::QColor:
        return ShaderType<QMetaType::QColor>::name();
    case QMetaType::QSize:
        return ShaderType<QMetaType::QSize>::name();
    case QMetaType::QSizeF:
        return ShaderType<QMetaType::QSizeF>::name();
    case QMetaType::QPoint:
        return ShaderType<QMetaType::QPoint>::name();
    case QMetaType::QPointF:
        return ShaderType<QMetaType::QPointF>::name();
    case QMetaType::QRect:
        return ShaderType<QMetaType::QRect>::name();
    case QMetaType::QRectF:
        return ShaderType<QMetaType::QRectF>::name();
    case QMetaType::QQuaternion:
        return ShaderType<QMetaType::QQuaternion>::name();
    case QMetaType::QMatrix4x4:
        return ShaderType<QMetaType::QMatrix4x4>::name();
    default:
        return QByteArray();
    }
}

QByteArray uniformTypeName(QSSGRenderShaderValue::Type type)
{
    switch (type) {
    case QSSGRenderShaderValue::Float:
        return ShaderType<QMetaType::Double>::name();
    case QSSGRenderShaderValue::Boolean:
        return ShaderType<QMetaType::Bool>::name();
    case QSSGRenderShaderValue::Integer:
        return ShaderType<QMetaType::Int>::name();
    case QSSGRenderShaderValue::Vec2:
        return ShaderType<QMetaType::QVector2D>::name();
    case QSSGRenderShaderValue::Vec3:
        return ShaderType<QMetaType::QVector3D>::name();
    case QSSGRenderShaderValue::Vec4:
        return ShaderType<QMetaType::QVector4D>::name();
    case QSSGRenderShaderValue::Rgba:
        return ShaderType<QMetaType::QColor>::name();
    case QSSGRenderShaderValue::Size:
        return ShaderType<QMetaType::QSize>::name();
    case QSSGRenderShaderValue::SizeF:
        return ShaderType<QMetaType::QSizeF>::name();
    case QSSGRenderShaderValue::Point:
        return ShaderType<QMetaType::QPoint>::name();
    case QSSGRenderShaderValue::PointF:
        return ShaderType<QMetaType::QPointF>::name();
    case QSSGRenderShaderValue::Rect:
        return ShaderType<QMetaType::QRect>::name();
    case QSSGRenderShaderValue::RectF:
        return ShaderType<QMetaType::QRectF>::name();
    case QSSGRenderShaderValue::Quaternion:
        return ShaderType<QMetaType::QQuaternion>::name();
    case QSSGRenderShaderValue::Matrix4x4:
        return ShaderType<QMetaType::QMatrix4x4>::name();
    default:
        return QByteArray();
    }
}

QSSGRenderShaderValue::Type uniformType(QMetaType type)
{
    switch (type.id()) {
    case QMetaType::Double:
    case QMetaType::Float:
        return ShaderType<QMetaType::Double>::type();
    case QMetaType::Bool:
        return ShaderType<QMetaType::Bool>::type();
    case QMetaType::QVector2D:
        return ShaderType<QMetaType::QVector2D>::type();
    case QMetaType::QVector3D:
        return ShaderType<QMetaType::QVector3D>::type();
    case QMetaType::QVector4D:
        return ShaderType<QMetaType::QVector4D>::type();
    case QMetaType::Int:
        return ShaderType<QMetaType::Int>::type();
    case QMetaType::QColor:
        return ShaderType<QMetaType::QColor>::type();
    case QMetaType::QSize:
        return ShaderType<QMetaType::QSize>::type();
    case QMetaType::QSizeF:
        return ShaderType<QMetaType::QSizeF>::type();
    case QMetaType::QPoint:
        return ShaderType<QMetaType::QPoint>::type();
    case QMetaType::QPointF:
        return ShaderType<QMetaType::QPointF>::type();
    case QMetaType::QRect:
        return ShaderType<QMetaType::QRect>::type();
    case QMetaType::QRectF:
        return ShaderType<QMetaType::QRectF>::type();
    case QMetaType::QQuaternion:
        return ShaderType<QMetaType::QQuaternion>::type();
    case QMetaType::QMatrix4x4:
        return ShaderType<QMetaType::QMatrix4x4>::type();
    default:
        return QSSGRenderShaderValue::Unknown;
    }
}

MetaTypeList supportedMetatypes()
{
    return {std::begin(qssg_metatype_list), std::end(qssg_metatype_list)};
}

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
                                                                              qsizetype index)
{
    QQuick3DShaderUtilsRenderPass *that = qobject_cast<QQuick3DShaderUtilsRenderPass *>(list->object);
    return that->m_commands.at(index);
}

qsizetype QQuick3DShaderUtilsRenderPass::qmlCommandCount(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list)
{
    QQuick3DShaderUtilsRenderPass *that = qobject_cast<QQuick3DShaderUtilsRenderPass *>(list->object);
    return that->m_commands.size();
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

    connect(shader, &QQuick3DShaderUtilsShader::shaderChanged, that, &QQuick3DShaderUtilsRenderPass::changed);
    connect(shader, &QQuick3DShaderUtilsShader::stageChanged, that, &QQuick3DShaderUtilsRenderPass::changed);

    emit that->changed();
}

QQuick3DShaderUtilsShader *QQuick3DShaderUtilsRenderPass::qmlShaderAt(QQmlListProperty<QQuick3DShaderUtilsShader> *list,
                                                                      qsizetype index)
{
    QQuick3DShaderUtilsRenderPass *that = qobject_cast<QQuick3DShaderUtilsRenderPass *>(list->object);
    return that->m_shaders.at(index);
}

qsizetype QQuick3DShaderUtilsRenderPass::qmlShaderCount(QQmlListProperty<QQuick3DShaderUtilsShader> *list)
{
    QQuick3DShaderUtilsRenderPass *that = qobject_cast<QQuick3DShaderUtilsRenderPass *>(list->object);
    return that->m_shaders.size();
}

void QQuick3DShaderUtilsRenderPass::qmlShaderClear(QQmlListProperty<QQuick3DShaderUtilsShader> *list)
{
    QQuick3DShaderUtilsRenderPass *that = qobject_cast<QQuick3DShaderUtilsRenderPass *>(list->object);

    for (QQuick3DShaderUtilsShader *shader : that->m_shaders)
        shader->disconnect(that);

    that->m_shaders.clear();

    emit that->changed();
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

QQuick3DShaderUtilsTextureInput::QQuick3DShaderUtilsTextureInput(QObject *p) : QObject(p) {}

QQuick3DShaderUtilsTextureInput::~QQuick3DShaderUtilsTextureInput()
{
}

void QQuick3DShaderUtilsTextureInput::setTexture(QQuick3DTexture *texture)
{
    if (m_texture == texture)
        return;

    QObject *p = parent();
    while (p != nullptr) {
        if (QQuick3DCustomMaterial *mat = qobject_cast<QQuick3DCustomMaterial *>(p)) {
            mat->setDynamicTextureMap(this);
            QQuick3DObjectPrivate::attachWatcherPriv(mat, this, &QQuick3DShaderUtilsTextureInput::setTexture, texture, m_texture);
            break;
        } else if (QQuick3DEffect *efx = qobject_cast<QQuick3DEffect *>(p)) {
            efx->setDynamicTextureMap(this);
            QQuick3DObjectPrivate::attachWatcherPriv(efx, this, &QQuick3DShaderUtilsTextureInput::setTexture, texture, m_texture);
            break;
        }
        p = p->parent();
    }

    if (p == nullptr) {
        qWarning("A TextureInput was defined without a CustomMaterial or Effect ancestor. This should be avoided.");
    }

    m_texture = texture;
    Q_EMIT textureChanged();
}

QT_END_NAMESPACE
