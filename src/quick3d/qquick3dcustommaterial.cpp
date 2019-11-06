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
#include <QtQuick/QQuickWindow>

#include "qquick3dobject_p_p.h"
#include "qquick3dviewport_p.h"

Q_DECLARE_OPAQUE_POINTER(QQuick3DCustomMaterialTextureInput)

QT_BEGIN_NAMESPACE

/*!
    \qmltype CustomMaterial
    \inherits Material
    \inqmlmodule QtQuick3D.Materials
    \brief Base component for creating custom materials used to shade models.

    The custom material allows the user of QtQuick3D to access its material library and implement
    own materials. There are two types of custom materials, which differ on how they are using the
    material library. First one uses the custom material interface provided by the library to
    implement materials similarly to many of the materials in the material library without
    implementing it's own main function. This type of material must implement all the required
    functions of the material. The second type implements it's own main function, but can still
    use functionality from the material library. See \l {Qt Quick 3D Custom Material Reference}{reference}
    on how to implement the material using the material interface.

    \qml
    CustomMaterial {
        // These properties names need to match the ones in the shader code!
        property bool uEnvironmentMappingEnabled: false
        property bool uShadowMappingEnabled: false
        property real roughness: 0.0
        property vector3d metal_color: Qt.vector3d(0.805, 0.395, 0.305)

        shaderInfo: ShaderInfo {
            version: "330"
            type: "GLSL"
            shaderKey: ShaderInfo.Glossy
        }

        property TextureInput uEnvironmentTexture: TextureInput {
                enabled: uEnvironmentMappingEnabled
                texture: Texture {
                    id: envImage
                    source: "maps/spherical_checker.png"
                }
        }
        property TextureInput uBakedShadowTexture: TextureInput {
                enabled: uShadowMappingEnabled
                texture: Texture {
                    id: shadowImage
                    source: "maps/shadow.png"
                }
        }

        Shader {
            id: copperFragShader
            stage: Shader.Fragment
            shader: "shaders/copper.frag"
        }

        passes: [ Pass {
                shaders: copperFragShader
            }
        ]
    }
    \endqml

    The example here from CopperMaterial shows how the material is built. First, the shader
    parameters are specified as properties. The names and types must match the names in the shader
    code. Textures use TextureInput to assign \l{QtQuick3D::Texture}{texture} into the shader variable.
    The shaderInfo property specifies more information about the shader and also configures some of
    its features on or off when the custom material is built by QtQuick3D shader generator.
    Then the material can use Shader type to specify shader source and shader stage. These are used
    with \l {Pass}{passes} to create the resulting material. The passes can contain multiple
    rendering passes and also other commands. Normally only the fragment shader needs to be passed
    to a pass. The material library generates the vertex shader for the material. The material can
    also create \l {Buffer}{buffers} to store intermediate rendering results. Here is an example
    from GlassRefractiveMaterial:

    \qml
    Buffer {
        id: tempBuffer
        name: "temp_buffer"
        format: Buffer.Unknown
        textureFilterOperation: Buffer.Linear
        textureCoordOperation: Buffer.ClampToEdge
        sizeMultiplier: 1.0
        bufferFlags: Buffer.None // aka frame
    }

    passes: [ Pass {
            shaders: simpleGlassRefractiveFragShader
            commands: [ BufferBlit {
                    destination: tempBuffer
                }, BufferInput {
                    buffer: tempBuffer
                    param: "refractiveTexture"
                }, Blending {
                    srcBlending: Blending.SrcAlpha
                    destBlending: Blending.OneMinusSrcAlpha
                }
            ]
        }
    ]
    \endqml

    Multiple passes can also be specified to create advanced materials. Here is an example from
    FrostedGlassMaterial.

    \qml
    passes: [ Pass {
            shaders: noopShader
            output: dummyBuffer
            commands: [ BufferBlit {
                    destination: frameBuffer
                }
            ]
        }, Pass {
            shaders: preBlurShader
            output: tempBuffer
            commands: [ BufferInput {
                    buffer: frameBuffer
                    param: "OriginBuffer"
                }
            ]
        }, Pass {
            shaders: blurXShader
            output: blurXBuffer
            commands: [ BufferInput {
                    buffer: tempBuffer
                    param: "BlurBuffer"
                }
            ]
        }, Pass {
            shaders: blurYShader
            output: blurYBuffer
            commands: [ BufferInput {
                    buffer: blurXBuffer
                    param: "BlurBuffer"
                }, BufferInput {
                    buffer: tempBuffer
                    param: "OriginBuffer"
                }
            ]
        }, Pass {
            shaders: mainShader
            commands: [BufferInput {
                    buffer: blurYBuffer
                    param: "refractiveTexture"
                }, Blending {
                    srcBlending: Blending.SrcAlpha
                    destBlending: Blending.OneMinusSrcAlpha
                }
            ]
        }
    ]
    \endqml
*/
/*!
    \qmlproperty bool CustomMaterial::hasTransparency
    Specifies that the material has transparency.
*/
/*!
    \qmlproperty bool CustomMaterial::hasRefraction
    Specifies that the material has refraction.
*/
/*!
    \qmlproperty bool CustomMaterial::alwaysDirty
    Specifies that the material state is always dirty, which indicates that the material needs
    to be refreshed every time it is used by the QtQuick3D.
*/
/*!
    \qmlproperty ShaderInfo CustomMaterial::shaderInfo
    Specifies the ShaderInfo of the material.
*/
/*!
    \qmlproperty list CustomMaterial::passes
    Contains a list of render \l {Pass}{passes} implemented by the material.
*/

/*!
    \qmltype Shader
    \inherits Object
    \inqmlmodule QtQuick3D.Materials
    \brief Container component for defining shader code used by CustomMaterials.
*/
/*!
    \qmlproperty string Shader::shader
    Specifies the name of the shader source file.
*/
/*!
    \qmlproperty enumeration Shader::stage
    Specifies the shader stage.

    \value Shader.Shared The shader can be shared among different stages
    \value Shader.Vertex The shader is a vertex shader
    \value Shader.Fragment The shader is a fragment shader
    \value Shader.Geometry The shader is a geometry shader
    \value Shader.Compute The shader is a compute shader
*/

/*!
    \qmltype ShaderInfo
    \inherits Object
    \inqmlmodule QtQuick3D.Materials
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
    \inqmlmodule QtQuick3D.Materials
    \brief Defines a texture channel for a Custom Material.
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
    \inqmlmodule QtQuick3D.Materials
    \brief Defines a render pass in the CustomMaterial.
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
    \inqmlmodule QtQuick3D.Materials
    \brief Defines a command to be performed in a pass of a CustomMaterial.
*/

/*!
    \qmltype BufferInput
    \inherits Command
    \inqmlmodule QtQuick3D.Materials
    \brief Defines an input buffer to be used for a pass of a CustomMaterial.
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
    \inqmlmodule QtQuick3D.Materials
    \brief Defines a copy operation between two buffers in a pass of a CustomMaterial.
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
    \inqmlmodule QtQuick3D.Materials
    \brief Defines the blending state in a pass of a CustomMaterial.
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
    \inqmlmodule QtQuick3D.Materials
    \brief Defines a buffer to be used for a pass of a CustomMaterial.
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
    \inqmlmodule QtQuick3D.Materials
    \brief Defines the render state to be disabled in a pass of a CustomMaterial.
*/
/*!
    \qmlproperty enumeration RenderState::renderState
    Specifies the render state to enable/disable in a \l {Pass}{pass}.

    \value RenderState.Unknown
    \value RenderState.Blend
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

QQuick3DCustomMaterialBuffer::TextureFormat QQuick3DCustomMaterialBuffer::format() const
{
    return mapRenderTextureFormat(command.m_format.format);
}

void QQuick3DCustomMaterialBuffer::setFormat(TextureFormat format)
{
    command.m_format = mapTextureFormat(format);
}

QQuick3DCustomMaterialBuffer::TextureFormat QQuick3DCustomMaterialBuffer::mapRenderTextureFormat(
        QSSGRenderTextureFormat::Format fmt)
{
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

QSSGRenderTextureFormat::Format QQuick3DCustomMaterialBuffer::mapTextureFormat(
        QQuick3DCustomMaterialBuffer::TextureFormat fmt)
{
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

QQuick3DCustomMaterial::QQuick3DCustomMaterial() {}

QQuick3DCustomMaterial::~QQuick3DCustomMaterial() {}

QQuick3DObject::Type QQuick3DCustomMaterial::type() const
{
    return QQuick3DObject::CustomMaterial;
}

bool QQuick3DCustomMaterial::hasTransparency() const
{
    return m_hasTransparency;
}

bool QQuick3DCustomMaterial::hasRefraction() const
{
    return m_hasRefraction;
}

QQuick3DCustomMaterialShaderInfo *QQuick3DCustomMaterial::shaderInfo() const
{
    return m_shaderInfo;
}

QQmlListProperty<QQuick3DCustomMaterialRenderPass> QQuick3DCustomMaterial::passes()
{
    return QQmlListProperty<QQuick3DCustomMaterialRenderPass>(this,
                                                            nullptr,
                                                            QQuick3DCustomMaterial::qmlAppendPass,
                                                            QQuick3DCustomMaterial::qmlPassCount,
                                                            QQuick3DCustomMaterial::qmlPassAt,
                                                            nullptr);
}

bool QQuick3DCustomMaterial::alwaysDirty() const
{
    return m_alwaysDirty;
}

void QQuick3DCustomMaterial::setHasTransparency(bool hasTransparency)
{
    if (m_hasTransparency == hasTransparency)
        return;

    m_hasTransparency = hasTransparency;
    emit hasTransparencyChanged(m_hasTransparency);
}

void QQuick3DCustomMaterial::setHasRefraction(bool hasRefraction)
{
    if (m_hasRefraction == hasRefraction)
        return;

    m_hasRefraction = hasRefraction;
    emit hasRefractionChanged(m_hasRefraction);
}

void QQuick3DCustomMaterial::setShaderInfo(QQuick3DCustomMaterialShaderInfo *shaderInfo)
{
    m_shaderInfo = shaderInfo;
}

void QQuick3DCustomMaterial::setAlwaysDirty(bool alwaysDirty)
{
    if (m_alwaysDirty == alwaysDirty)
        return;

    m_alwaysDirty = alwaysDirty;
    emit alwaysDirtyChanged(m_alwaysDirty);
}

QSSGRenderGraphObject *QQuick3DCustomMaterial::updateSpatialNode(QSSGRenderGraphObject *node)
{
    static const auto updateShaderPrefix = [](QByteArray &shaderPrefix, const QByteArray &name) {
        const char *filter = "linear";
        const char *clamp = "clamp";
        // Output macro so we can change the set of variables used for this
        // independent of the
        // meta data system.
        shaderPrefix.append("SNAPPER_SAMPLER2D(");
        shaderPrefix.append(name);
        shaderPrefix.append(", ");
        shaderPrefix.append(name);
        shaderPrefix.append(", ");
        shaderPrefix.append(filter);
        shaderPrefix.append(", ");
        shaderPrefix.append(clamp);
        shaderPrefix.append(", ");
        shaderPrefix.append("false )\n");
    };

    static const auto appendShaderUniform = [](const QByteArray &type, const QByteArray &name, QByteArray *shaderPrefix) {
        shaderPrefix->append(QByteArrayLiteral("uniform ") + type + " " + name + ";\n");
    };

    static const auto resolveShader = [](const QByteArray &shader) -> QByteArray {
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
        if (f.open(QIODevice::ReadOnly | QIODevice::Text))
            return f.readAll();

        return shader;
    };

    static const auto mergeShaderCode = [](const QByteArray &shared, const QByteArray &vertex, const QByteArray &geometry, const QByteArray &fragment) {
        QByteArray shaderCode;
            // Shared
            if (!shared.isEmpty())
                shaderCode.append(shared);

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
    };


    // Sanity check(s)
    if (!m_shaderInfo || !m_shaderInfo->isValid()) {
        qWarning("ShaderInfo is not valid!");
        return node;
    }

    // Find the parent window
    QObject *p = this;
    QQuickWindow *window = nullptr;
    while (p != nullptr && window == nullptr) {
        p = p->parent();
        if ((window = qobject_cast<QQuickWindow *>(p)))
            break;
    }

    QSSGRenderContextInterface::QSSGRenderContextInterfacePtr renderContext
                = QSSGRenderContextInterface::getRenderContextInterface(quintptr(window));

    QSSGRenderCustomMaterial *customMaterial = static_cast<QSSGRenderCustomMaterial *>(node);
    if (!customMaterial) {
        customMaterial = new QSSGRenderCustomMaterial;
        customMaterial->m_shaderKeyValues = static_cast<QSSGRenderCustomMaterial::MaterialShaderKeyFlags>(m_shaderInfo->shaderKey);
        customMaterial->className = metaObject()->className();
        customMaterial->m_alwaysDirty = m_alwaysDirty;
        customMaterial->m_hasTransparency = m_hasTransparency;
        customMaterial->m_hasRefraction = m_hasRefraction;

        // Shader info
        auto &shaderInfo = customMaterial->shaderInfo;
        shaderInfo.type = m_shaderInfo->type;
        shaderInfo.version = m_shaderInfo->version;
        shaderInfo.shaderPrefix = m_shaderInfo->shaderPrefix;

        QMetaMethod propertyDirtyMethod;
        const int idx = metaObject()->indexOfSlot("onPropertyDirty()");
        if (idx != -1)
            propertyDirtyMethod = metaObject()->method(idx);

        // Properties
        const int propCount = metaObject()->propertyCount();
        const int propOffset = metaObject()->propertyOffset();
        QVector<QMetaProperty> userProperties;
        for (int i = propOffset; i != propCount; ++i) {
            const auto property = metaObject()->property(i);
            if (Q_UNLIKELY(!property.isValid()))
                continue;

            // Track the property changes
            if (property.hasNotifySignal() && propertyDirtyMethod.isValid())
                connect(this, property.notifySignal(), this, propertyDirtyMethod);

            if (property.type() == QVariant::Double) {
                appendShaderUniform(ShaderType<QVariant::Double>::name(), property.name(), &shaderInfo.shaderPrefix);
                customMaterial->properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Double>::type(), i});
            } else if (property.type() == QVariant::Bool) {
                appendShaderUniform(ShaderType<QVariant::Bool>::name(), property.name(), &shaderInfo.shaderPrefix);
                customMaterial->properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Bool>::type(), i});
            } else if (property.type() == QVariant::Vector2D) {
                appendShaderUniform(ShaderType<QVariant::Vector2D>::name(), property.name(), &shaderInfo.shaderPrefix);
                customMaterial->properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Vector2D>::type(), i});
            } else if (property.type() == QVariant::Vector3D) {
                appendShaderUniform(ShaderType<QVariant::Vector3D>::name(), property.name(), &shaderInfo.shaderPrefix);
                customMaterial->properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Vector3D>::type(), i});
            } else if (property.type() == QVariant::Vector4D) {
                appendShaderUniform(ShaderType<QVariant::Vector4D>::name(), property.name(), &shaderInfo.shaderPrefix);
                customMaterial->properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Vector4D>::type(), i});
            } else if (property.type() == QVariant::Int) {
                appendShaderUniform(ShaderType<QVariant::Int>::name(), property.name(), &shaderInfo.shaderPrefix);
                customMaterial->properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Int>::type(), i});
            } else if (property.type() == QVariant::Color) {
                appendShaderUniform(ShaderType<QVariant::Color>::name(), property.name(), &shaderInfo.shaderPrefix);
                customMaterial->properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Color>::type(), i});
            } else if (property.type() == QVariant::UserType) {
                if (property.userType() == qMetaTypeId<QQuick3DCustomMaterialTextureInput *>())
                    userProperties.push_back(property);
            } else {
                Q_ASSERT(0);
            }
        }

        // Textures
        for (const auto &userProperty : qAsConst(userProperties)) {
            QSSGRenderCustomMaterial::TextureProperty textureData;
            QQuick3DCustomMaterialTextureInput *texture = userProperty.read(this).value<QQuick3DCustomMaterialTextureInput *>();
            const QByteArray &name = userProperty.name();
            if (name.isEmpty()) // Warnings here will just drown in the shader error messages
                continue;
            QQuick3DTexture *tex = texture->texture(); //
            connect(texture, &QQuick3DCustomMaterialTextureInput::textureDirty, this, &QQuick3DCustomMaterial::onTextureDirty);
            textureData.name = name;
            if (texture->enabled)
                textureData.texImage = tex->getRenderImage();
            textureData.shaderDataType = QSSGRenderShaderDataType::Texture2D;
            textureData.clampType = tex->horizontalTiling() == QQuick3DTexture::Repeat ? QSSGRenderTextureCoordOp::Repeat
                                                                                     : (tex->horizontalTiling() == QQuick3DTexture::ClampToEdge) ? QSSGRenderTextureCoordOp::ClampToEdge
                                                                                                                                               : QSSGRenderTextureCoordOp::MirroredRepeat;
            updateShaderPrefix(shaderInfo.shaderPrefix, textureData.name);
            customMaterial->textureProperties.push_back(textureData);
        }

        QByteArray &shared = shaderInfo.shaderPrefix;
        QByteArray vertex, geometry, fragment, shaderCode;
        if (!m_passes.isEmpty()) {
            for (const auto &pass : qAsConst(m_passes)) {
                QQuick3DCustomMaterialShader *sharedShader = pass->m_shaders.at(int(QQuick3DCustomMaterialShader::Stage::Shared));
                QQuick3DCustomMaterialShader *vertShader = pass->m_shaders.at(int(QQuick3DCustomMaterialShader::Stage::Vertex));
                QQuick3DCustomMaterialShader *fragShader = pass->m_shaders.at(int(QQuick3DCustomMaterialShader::Stage::Fragment));
                QQuick3DCustomMaterialShader *geomShader = pass->m_shaders.at(int(QQuick3DCustomMaterialShader::Stage::Geometry));
                if (!sharedShader && !vertShader && !fragShader && !geomShader) {
                    qWarning("Pass with no shader attatched!");
                    continue;
                }

                // Build up shader code
                const QByteArray &shaderName = fragShader ? fragShader->shader : vertShader->shader;
                Q_ASSERT(!shaderName.isEmpty());

                if (sharedShader)
                    shared += resolveShader(sharedShader->shader);
                if (vertShader)
                    vertex = resolveShader(vertShader->shader);
                if (fragShader)
                    fragment = resolveShader(fragShader->shader);
                if (geomShader)
                    geometry = resolveShader(geomShader->shader);

                shaderCode = mergeShaderCode(shared, vertex, geometry, fragment);

                // Bind shader
                customMaterial->commands.push_back(new dynamic::QSSGBindShader(shaderName));
                customMaterial->commands.push_back(new dynamic::QSSGApplyInstanceValue());

                // Buffers
                QQuick3DCustomMaterialBuffer *outputBuffer = pass->outputBuffer;
                if (outputBuffer) {
                    const QByteArray &outBufferName = outputBuffer->name;
                    Q_ASSERT(!outBufferName.isEmpty());
                    // Allocate buffer command
                    customMaterial->commands.push_back(outputBuffer->getCommand());
                    // bind buffer
                    customMaterial->commands.push_back(new dynamic::QSSGBindBuffer(outBufferName, true));
                } else {
                    customMaterial->commands.push_back(new dynamic::QSSGBindTarget(QSSGRenderTextureFormat::RGBA8));
                }

                // Other commands (BufferInput, Blending ... )
                const auto &extraCommands = pass->m_commands;
                for (const auto &command : extraCommands) {
                    const int bufferCount = command->bufferCount();
                    for (int i = 0; i != bufferCount; ++i)
                        customMaterial->commands.push_back(command->bufferAt(i)->getCommand());
                    customMaterial->commands.push_back(command->getCommand());
                }

                // ... and finaly the render command
                customMaterial->commands.push_back(new dynamic::QSSGRender);

                renderContext->customMaterialSystem()->setMaterialClassShader(shaderName, shaderInfo.type, shaderInfo.version, shaderCode, false, false);
            }
        }
    }

    QQuick3DMaterial::updateSpatialNode(customMaterial);

    if (m_dirtyAttributes & Dirty::PropertyDirty) {
        for (const auto &prop : qAsConst(customMaterial->properties)) {
            auto p = metaObject()->property(prop.pid);
            if (Q_LIKELY(p.isValid()))
                prop.value = p.read(this);
        }
    }

    if (m_dirtyAttributes & Dirty::TextureDirty) {
        // TODO:
    }

    return customMaterial;
}

void QQuick3DCustomMaterial::onPropertyDirty()
{
    markDirty(Dirty::PropertyDirty);
    update();
}

void QQuick3DCustomMaterial::onTextureDirty(QQuick3DCustomMaterialTextureInput *texture)
{
    Q_UNUSED(texture)
    markDirty(Dirty::TextureDirty);
    update();
}

void QQuick3DCustomMaterial::qmlAppendPass(QQmlListProperty<QQuick3DCustomMaterialRenderPass> *list, QQuick3DCustomMaterialRenderPass *pass)
{
    if (!pass)
        return;

    QQuick3DCustomMaterial *that = qobject_cast<QQuick3DCustomMaterial *>(list->object);
    that->m_passes.push_back(pass);
}

QQuick3DCustomMaterialRenderPass *QQuick3DCustomMaterial::qmlPassAt(QQmlListProperty<QQuick3DCustomMaterialRenderPass> *list, int index)
{
    QQuick3DCustomMaterial *that = qobject_cast<QQuick3DCustomMaterial *>(list->object);
    return that->m_passes.at(index);
}

int QQuick3DCustomMaterial::qmlPassCount(QQmlListProperty<QQuick3DCustomMaterialRenderPass> *list)
{
    QQuick3DCustomMaterial *that = qobject_cast<QQuick3DCustomMaterial *>(list->object);
    return that->m_passes.count();
}

void QQuick3DCustomMaterialRenderPass::qmlAppendCommand(QQmlListProperty<QQuick3DCustomMaterialRenderCommand> *list, QQuick3DCustomMaterialRenderCommand *command)
{
    if (!command)
        return;

    QQuick3DCustomMaterialRenderPass *that = qobject_cast<QQuick3DCustomMaterialRenderPass *>(list->object);
    that->m_commands.push_back(command);
}

QQuick3DCustomMaterialRenderCommand *QQuick3DCustomMaterialRenderPass::qmlCommandAt(QQmlListProperty<QQuick3DCustomMaterialRenderCommand> *list, int index)
{
    QQuick3DCustomMaterialRenderPass *that = qobject_cast<QQuick3DCustomMaterialRenderPass *>(list->object);
    return that->m_commands.at(index);
}

int QQuick3DCustomMaterialRenderPass::qmlCommandCount(QQmlListProperty<QQuick3DCustomMaterialRenderCommand> *list)
{
    QQuick3DCustomMaterialRenderPass *that = qobject_cast<QQuick3DCustomMaterialRenderPass *>(list->object);
    return that->m_commands.count();
}

QQmlListProperty<QQuick3DCustomMaterialRenderCommand> QQuick3DCustomMaterialRenderPass::commands()
{
    return QQmlListProperty<QQuick3DCustomMaterialRenderCommand>(this,
                                                               nullptr,
                                                               QQuick3DCustomMaterialRenderPass::qmlAppendCommand,
                                                               QQuick3DCustomMaterialRenderPass::qmlCommandCount,
                                                               QQuick3DCustomMaterialRenderPass::qmlCommandAt,
                                                               nullptr);
}

void QQuick3DCustomMaterialRenderPass::qmlAppendShader(QQmlListProperty<QQuick3DCustomMaterialShader> *list, QQuick3DCustomMaterialShader *shader)
{
    if (!shader)
        return;

    QQuick3DCustomMaterialRenderPass *that = qobject_cast<QQuick3DCustomMaterialRenderPass *>(list->object);
    that->m_shaders[int(shader->stage)] = shader;
}

QQuick3DCustomMaterialShader *QQuick3DCustomMaterialRenderPass::qmlShaderAt(QQmlListProperty<QQuick3DCustomMaterialShader> *list, int index)
{
    QQuick3DCustomMaterialRenderPass *that = qobject_cast<QQuick3DCustomMaterialRenderPass *>(list->object);
    return that->m_shaders.at(index);
}

int QQuick3DCustomMaterialRenderPass::qmlShaderCount(QQmlListProperty<QQuick3DCustomMaterialShader> *list)
{
    QQuick3DCustomMaterialRenderPass *that = qobject_cast<QQuick3DCustomMaterialRenderPass *>(list->object);
    return that->m_shaders.count();
}

void QQuick3DCustomMaterialRenderPass::qmlShaderClear(QQmlListProperty<QQuick3DCustomMaterialShader> *list)
{
    QQuick3DCustomMaterialRenderPass *that = qobject_cast<QQuick3DCustomMaterialRenderPass *>(list->object);
    auto it = that->m_shaders.begin();
    const auto end = that->m_shaders.end();
    for (;it != end; ++it)
        *it = nullptr;
}

QQmlListProperty<QQuick3DCustomMaterialShader> QQuick3DCustomMaterialRenderPass::shaders()
{
    return QQmlListProperty<QQuick3DCustomMaterialShader>(this,
                                                        nullptr,
                                                        QQuick3DCustomMaterialRenderPass::qmlAppendShader,
                                                        QQuick3DCustomMaterialRenderPass::qmlShaderCount,
                                                        QQuick3DCustomMaterialRenderPass::qmlShaderAt,
                                                        QQuick3DCustomMaterialRenderPass::qmlShaderClear);
}

QT_END_NAMESPACE
