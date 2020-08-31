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

#include "qquick3deffect_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderlibrarymanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>
#include <QtQuick3DRuntimeRender/private/qssgshadermaterialadapter_p.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick3D/private/qquick3dobject_p.h>
#include <QtCore/qfile.h>
#include <QtCore/qurl.h>


QT_BEGIN_NAMESPACE

/*!
    \qmltype Effect
    \inherits Object3D
    \inqmlmodule QtQuick3D
    \instantiates QQuick3DEffect
    \brief Base component for creating a post-processing effect.

    The Effect type allows the user to implement their own post-processing effects for QtQuick3D.
    This is how to create your own effect, using \l GaussianBlur as an example:

    \qml
    Effect {
        // The property name is generated as a uniform to the shader code, so it must match
        // the name and type used in shader code.
        property real amount: 2 // 0 - 10

        // The vertex shaders are defined with the Shader type.
        Shader {
            id: vertical
            stage: Shader.Vertex
            shader: "shaders/blurvertical.vert"
        }
        Shader {
            id: horizontal
            stage: Shader.Vertex
            shader: "shaders/blurhorizontal.vert"
        }

        // The fragment shader is defined with the Shader type.
        Shader {
            id: gaussianblur
            stage: Shader.Fragment
            shader: "shaders/gaussianblur.frag"
        }

        // In this shader we need a temporary buffer to store the output of the first blur pass.
        Buffer {
            id: tempBuffer
            name: "tempBuffer"
            format: Buffer.RGBA8
            textureFilterOperation: Buffer.Linear
            textureCoordOperation: Buffer.ClampToEdge
            bufferFlags: Buffer.None // Lifetime of the buffer is one frame
        }

        // GaussianBlur needs two passes; a horizontal blur and a vertical blur.
        // Only the vertex shader is different in this case, so we can use the same fragment
        // shader for both passes.
        passes: [
            Pass {
                shaders: [ horizontal, gaussianblur ]
                output: tempBuffer
            },
            Pass {
                shaders: [ vertical, gaussianblur ]
                commands: [
                    // We feed the output of the first pass as an input for the second pass.
                    BufferInput {
                        buffer: tempBuffer
                    }
                ]
            }
        ]
    }
    \endqml

    \sa Shader, Buffer, Pass
*/

/*!
    \qmlproperty list Effect::passes
    Contains a list of render \l {Pass}{passes} implemented by the effect.
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

static QByteArray uniformTypeName(QVariant::Type type)
{
    if (type == QVariant::Double) {
        return ShaderType<QVariant::Double>::name();
    } else if (type == QVariant::Bool) {
        return ShaderType<QVariant::Bool>::name();
    } else if (type == QVariant::Vector2D) {
        return ShaderType<QVariant::Vector2D>::name();
    } else if (type == QVariant::Vector3D) {
        return ShaderType<QVariant::Vector3D>::name();
    } else if (type == QVariant::Vector4D) {
        return ShaderType<QVariant::Vector4D>::name();
    } else if (type == QVariant::Int) {
        return ShaderType<QVariant::Int>::name();
    } else if (type == QVariant::Color) {
        return ShaderType<QVariant::Color>::name();
    } else if (type == QVariant::Size) {
        return ShaderType<QVariant::Size>::name();
    } else if (type == QVariant::SizeF) {
        return ShaderType<QVariant::SizeF>::name();
    } else if (type == QVariant::Point) {
        return ShaderType<QVariant::Point>::name();
    } else if (type == QVariant::PointF) {
        return ShaderType<QVariant::PointF>::name();
    } else if (type == QVariant::Rect) {
        return ShaderType<QVariant::Rect>::name();
    } else if (type == QVariant::RectF) {
        return ShaderType<QVariant::RectF>::name();
    } else if (type == QVariant::Quaternion) {
        return ShaderType<QVariant::Quaternion>::name();
    } else if (type == QVariant::Matrix4x4) {
        return ShaderType<QVariant::Matrix4x4>::name();
    }

    return QByteArray();
}

static QSSGRenderShaderDataType uniformType(QVariant::Type type)
{
    if (type == QVariant::Double) {
        return ShaderType<QVariant::Double>::type();
    } else if (type == QVariant::Bool) {
        return ShaderType<QVariant::Bool>::type();
    } else if (type == QVariant::Vector2D) {
        return ShaderType<QVariant::Vector2D>::type();
    } else if (type == QVariant::Vector3D) {
        return ShaderType<QVariant::Vector3D>::type();
    } else if (type == QVariant::Vector4D) {
        return ShaderType<QVariant::Vector4D>::type();
    } else if (type == QVariant::Int) {
        return ShaderType<QVariant::Int>::type();
    } else if (type == QVariant::Color) {
        return ShaderType<QVariant::Color>::type();
    } else if (type == QVariant::Size) {
        return ShaderType<QVariant::Size>::type();
    } else if (type == QVariant::SizeF) {
        return ShaderType<QVariant::SizeF>::type();
    } else if (type == QVariant::Point) {
        return ShaderType<QVariant::Point>::type();
    } else if (type == QVariant::PointF) {
        return ShaderType<QVariant::PointF>::type();
    } else if (type == QVariant::Rect) {
        return ShaderType<QVariant::Rect>::type();
    } else if (type == QVariant::RectF) {
        return ShaderType<QVariant::RectF>::type();
    } else if (type == QVariant::Quaternion) {
        return ShaderType<QVariant::Quaternion>::type();
    } else if (type == QVariant::Matrix4x4) {
        return ShaderType<QVariant::Matrix4x4>::type();
    }

    return QSSGRenderShaderDataType::Unknown;
}

QQuick3DEffect::QQuick3DEffect(QQuick3DObject *parent)
    : QQuick3DObject(*(new QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::Effect)), parent)
{
}

QQmlListProperty<QQuick3DShaderUtilsRenderPass> QQuick3DEffect::passes()
{
    return QQmlListProperty<QQuick3DShaderUtilsRenderPass>(this,
                                                      nullptr,
                                                      QQuick3DEffect::qmlAppendPass,
                                                      QQuick3DEffect::qmlPassCount,
                                                      QQuick3DEffect::qmlPassAt,
                                                      QQuick3DEffect::qmlPassClear);
}

// Default vertex and fragment shader code that is used when no corresponding
// Shader is present in the Effect. These go through the usual processing so
// should use the user-facing builtins.

static const char *default_effect_vertex_shader =
        "void MAIN()\n"
        "{\n"
        "}\n";

static const char *default_effect_fragment_shader =
        "void MAIN()\n"
        "{\n"
        "    FRAGCOLOR = texture(INPUT, INPUT_UV);\n"
        "}\n";

// Suffix snippets added to the end of the shader strings. These are appended
// after processing so it must be valid GLSL as-is, no more magic keywords.

static const char *effect_vertex_main_pre =
        "void main()\n"
        "{\n"
        "    qt_inputUV = attr_uv;\n"
        "    qt_textureUV = qt_effectTextureMapUV(attr_uv);\n"
        "    vec4 qt_vertPosition = vec4(attr_pos, 1.0);\n"
        "    qt_customMain(qt_vertPosition.xyz);\n";

static const char *effect_vertex_main_position =
        "    gl_Position = qt_modelViewProjection * qt_vertPosition;\n";

static const char *effect_vertex_main_post =
        "}\n";

static const char *effect_fragment_main =
        "void main()\n"
        "{\n"
        "    qt_customMain();\n"
        "}\n";

static inline void insertVertexMainArgs(QByteArray &snippet)
{
    static const char *argKey =  "/*%QT_ARGS_MAIN%*/";
    const int argKeyLen = int(strlen(argKey));
    const int argKeyPos = snippet.indexOf(argKey);
    if (argKeyPos >= 0)
        snippet = snippet.left(argKeyPos) + QByteArrayLiteral("inout vec3 VERTEX") + snippet.mid(argKeyPos + argKeyLen);
}

QSSGRenderGraphObject *QQuick3DEffect::updateSpatialNode(QSSGRenderGraphObject *node)
{
    // Find the parent window
    QObject *p = this;
    QQuickWindow *window = nullptr;
    while (p != nullptr && window == nullptr) {
        p = p->parent();
        if ((window = qobject_cast<QQuickWindow *>(p)))
            break;
    }
    if (!window) {
        qWarning("QQuick3DEffect: No window?");
        return nullptr;
    }

    QSSGRenderContextInterface *renderContext = QSSGRenderContextInterface::getRenderContextInterface(quintptr(window)).data();

    QSSGRenderEffect *effectNode = static_cast<QSSGRenderEffect *>(node);
    if (!effectNode) {
        markAllDirty();
        effectNode = new QSSGRenderEffect;
        effectNode->setActive(true);

        QMetaMethod propertyDirtyMethod;
        const int idx = metaObject()->indexOfSlot("onPropertyDirty()");
        if (idx != -1)
            propertyDirtyMethod = metaObject()->method(idx);

        // Properties -> uniforms
        QSSGShaderCustomMaterialAdapter::StringPairList uniforms;
        const int propCount = metaObject()->propertyCount();
        int propOffset = metaObject()->propertyOffset();

        // Effect can have multilayered inheritance structure, so find the actual propOffset
        const QMetaObject *superClass = metaObject()->superClass();
        while (superClass && qstrcmp(superClass->className(), "QQuick3DEffect") != 0)  {
            propOffset = superClass->propertyOffset();
            superClass = superClass->superClass();
        }

        QVector<QMetaProperty> textureProperties; // We'll deal with these later
        for (int i = propOffset; i != propCount; ++i) {
            const auto property = metaObject()->property(i);
            if (Q_UNLIKELY(!property.isValid()))
                continue;

            if (property.type() == QVariant::UserType) {
                if (property.userType() == qMetaTypeId<QQuick3DShaderUtilsTextureInput *>())
                    textureProperties.push_back(property);
            } else {
                const auto type = uniformType(property.type());
                if (type != QSSGRenderShaderDataType::Unknown) {
                    uniforms.append({ uniformTypeName(property.type()), property.name() });
                    effectNode->properties.push_back({ property.name(), uniformTypeName(property.type()),
                                                       property.read(this), uniformType(property.type()), i});
                    // Track the property changes
                    if (property.hasNotifySignal() && propertyDirtyMethod.isValid())
                        connect(this, property.notifySignal(), this, propertyDirtyMethod);
                } else {
                    // ### figure out how _not_ to warn when there are no dynamic
                    // properties defined (because warnings like Blah blah objectName etc. are not helpful)
                    //qWarning("No known uniform conversion found for effect property %s. Skipping", property.name());
                }
            }
        }

        // Textures
        QByteArray textureData;
        for (const auto &property : qAsConst(textureProperties)) {
            QSSGRenderEffect::TextureProperty texProp;
            QQuick3DShaderUtilsTextureInput *texture = property.read(this).value<QQuick3DShaderUtilsTextureInput *>();
            const QByteArray &name = property.name();
            if (name.isEmpty())
                continue;
            QQuick3DTexture *tex = texture->texture();
            connect(texture, &QQuick3DShaderUtilsTextureInput::enabledChanged, this, &QQuick3DEffect::onTextureDirty);
            connect(texture, &QQuick3DShaderUtilsTextureInput::textureChanged, this, &QQuick3DEffect::onTextureDirty);
            texProp.name = name;
            if (texture->enabled)
                texProp.texImage = tex->getRenderImage();

            texProp.shaderDataType = QSSGRenderShaderDataType::Texture2D;

            texProp.minFilterType = tex->minFilter() == QQuick3DTexture::Nearest ? QSSGRenderTextureFilterOp::Nearest
                                                                                 : QSSGRenderTextureFilterOp::Linear;
            texProp.magFilterType = tex->magFilter() == QQuick3DTexture::Nearest ? QSSGRenderTextureFilterOp::Nearest
                                                                                 : QSSGRenderTextureFilterOp::Linear;
            texProp.mipFilterType = tex->generateMipmaps() ? (tex->mipFilter() == QQuick3DTexture::Nearest ? QSSGRenderTextureFilterOp::Nearest
                                                                                                           : QSSGRenderTextureFilterOp::Linear)
                                                           : QSSGRenderTextureFilterOp::None;
            texProp.clampType = tex->horizontalTiling() == QQuick3DTexture::Repeat ? QSSGRenderTextureCoordOp::Repeat
                                                                                   : (tex->horizontalTiling() == QQuick3DTexture::ClampToEdge ? QSSGRenderTextureCoordOp::ClampToEdge
                                                                                                                                              : QSSGRenderTextureCoordOp::MirroredRepeat);

            uniforms.append({ QByteArrayLiteral("sampler2D"), texProp.name });
            effectNode->textureProperties.push_back(texProp);
        }

        // built-ins
        uniforms.append({ "mat4", "qt_modelViewProjection" });
        uniforms.append({ "sampler2D", "qt_inputTexture" });
        uniforms.append({ "vec2", "qt_inputSize" });
        uniforms.append({ "vec2", "qt_outputSize" });
        uniforms.append({ "float", "qt_frame_num" });
        uniforms.append({ "float", "qt_fps" });
        uniforms.append({ "vec2", "qt_cameraProperties" });
        uniforms.append({ "float", "qt_normalAdjustViewportFactor" });
        uniforms.append({ "float", "qt_nearClipValue" });

        QSSGShaderCustomMaterialAdapter::StringPairList builtinVertexInputs;
        builtinVertexInputs.append({ "vec3", "attr_pos" });
        builtinVertexInputs.append({ "vec2", "attr_uv" });

        QSSGShaderCustomMaterialAdapter::StringPairList builtinVertexOutputs;
        builtinVertexOutputs.append({ "vec2", "qt_inputUV" });
        builtinVertexOutputs.append({ "vec2", "qt_textureUV" });

        // fragOutput is added automatically by the program generator

        if (!m_passes.isEmpty()) {
            const QQmlContext *context = qmlContext(this);
            int passIndex = 0;
            for (QQuick3DShaderUtilsRenderPass *pass : qAsConst(m_passes)) {
                // Have a key composed more or less of the vertex and fragment filenames.
                // The shaderLibraryManager uses stage+shaderPathKey as the key.
                // Thus shaderPathKey is then sufficient to look up both the vertex and fragment shaders later on.
                // Note that this key is not suitable as a unique key for the graphics resources because the same
                // set of shader files can be used in multiple different passes, or in multiple active effects.
                // But that's the effect system's problem.
                QByteArray shaderPathKey;
                QByteArray shaderSource[2];
                QSSGCustomShaderMetaData shaderMeta[2];
                for (QQuick3DShaderUtilsShader::Stage stage : { QQuick3DShaderUtilsShader::Stage::Vertex, QQuick3DShaderUtilsShader::Stage::Fragment }) {
                    QQuick3DShaderUtilsShader *shader = nullptr;
                    for (QQuick3DShaderUtilsShader *s : pass->m_shaders) {
                        if (s->stage == stage) {
                            shader = s;
                            break;
                        }
                    }

                    // just how many enums does one need for the exact same thing...
                    QSSGShaderCache::ShaderType type = QSSGShaderCache::ShaderType::Vertex;
                    if (stage == QQuick3DShaderUtilsShader::Stage::Fragment)
                        type = QSSGShaderCache::ShaderType::Fragment;

                    // Will just use the custom material infrastructure. Some
                    // substitutions are common between custom materials and effects.
                    //
                    // Substitutions relevant to us here:
                    //   MAIN -> qt_customMain
                    //   FRAGCOLOR -> fragOutput
                    //   POSITION -> gl_Position
                    //   MODELVIEWPROJECTION_MATRIX -> qt_modelViewProjection
                    //
                    //   INPUT -> qt_inputTexture
                    //   INPUT_UV -> qt_inputUV
                    //   ... other effect specifics
                    //
                    // Built-in uniforms, inputs and outputs will be baked into
                    // metadata comment blocks in the resulting source code.
                    // Same goes for inputs/outputs declared with VARYING.

                    QByteArray code;
                    if (shader) {
                        code = QSSGShaderUtils::resolveShader(shader->shader, context, shaderPathKey); // appends to shaderPathKey
                    } else {
                        if (!shaderPathKey.isEmpty())
                            shaderPathKey.append('>');
                        shaderPathKey += "DEFAULT";
                        if (type == QSSGShaderCache::ShaderType::Vertex)
                            code = default_effect_vertex_shader;
                        else
                            code = default_effect_fragment_shader;
                    }

                    QByteArray shaderCodeMeta;
                    QSSGShaderCustomMaterialAdapter::ShaderCodeAndMetaData result;
                    if (type == QSSGShaderCache::ShaderType::Vertex) {
                        result = QSSGShaderCustomMaterialAdapter::prepareCustomShader(shaderCodeMeta, code, type,
                                                                                      uniforms, builtinVertexInputs, builtinVertexOutputs);
                    } else {
                        result = QSSGShaderCustomMaterialAdapter::prepareCustomShader(shaderCodeMeta, code, type,
                                                                                      uniforms, builtinVertexOutputs);
                    }
                    code = result.first;
                    code.append(shaderCodeMeta);

                    if (type == QSSGShaderCache::ShaderType::Vertex) {
                        // qt_customMain() has an argument list which gets injected here
                        insertVertexMainArgs(code);
                        // add the real main(), with or without assigning gl_Position at the end
                        code.append(effect_vertex_main_pre);
                        if (!result.second.flags.testFlag(QSSGCustomShaderMetaData::OverridesPosition))
                            code.append(effect_vertex_main_position);
                        code.append(effect_vertex_main_post);
                    } else {
                        code.append(effect_fragment_main);
                    }

                    shaderSource[int(type)] = code;
                    shaderMeta[int(type)] = result.second;
                }

                // Now that the final shaderPathKey is known, store the source and
                // related data; it will be retrieved later by the QSSGRhiEffectSystem.
                for (QSSGShaderCache::ShaderType type : { QSSGShaderCache::ShaderType::Vertex, QSSGShaderCache::ShaderType::Fragment }) {
                    renderContext->shaderLibraryManager()->setShaderSource(shaderPathKey, type,
                                                                           shaderSource[int(type)], shaderMeta[int(type)]);
                }

                effectNode->commands.push_back(new QSSGBindShader(shaderPathKey, effectNode, passIndex));
                effectNode->commands.push_back(new QSSGApplyInstanceValue);

                // Buffers
                QQuick3DShaderUtilsBuffer *outputBuffer = pass->outputBuffer;
                if (outputBuffer) {
                    const QByteArray &outBufferName = outputBuffer->name;
                    if (outBufferName.isEmpty()) {
                        // default output buffer (with settings)
                        auto outputFormat = QQuick3DShaderUtilsBuffer::mapTextureFormat(outputBuffer->format());
                        effectNode->commands.push_back(new QSSGBindTarget(outputFormat));
                        effectNode->outputFormat = outputFormat;
                    } else {
                        // Allocate buffer command
                        effectNode->commands.push_back(outputBuffer->getCommand());
                        // bind buffer
                        effectNode->commands.push_back(new QSSGBindBuffer(outBufferName));
                    }
                } else {
                    // Use the default output buffer, same format as the source buffer
                    effectNode->commands.push_back(new QSSGBindTarget(QSSGRenderTextureFormat::Unknown));
                    effectNode->outputFormat = QSSGRenderTextureFormat::Unknown;
                }

                // Other commands (BufferInput, Blending ... )
                const auto &extraCommands = pass->m_commands;
                bool needsDepthTexture = false;
                for (const auto &command : extraCommands) {
                    const int bufferCount = command->bufferCount();
                    for (int i = 0; i != bufferCount; ++i)
                        effectNode->commands.push_back(command->bufferAt(i)->getCommand());
                    effectNode->commands.push_back(command->getCommand());
                    needsDepthTexture |= (qobject_cast<QQuick3DShaderApplyDepthValue *>(command) != nullptr);
                }
                effectNode->requiresDepthTexture = needsDepthTexture;

                effectNode->commands.push_back(new QSSGRender);

                ++passIndex;
            }
        }
    }

    if (m_dirtyAttributes & Dirty::PropertyDirty) {
        for (const auto &prop : qAsConst(effectNode->properties)) {
            auto p = metaObject()->property(prop.pid);
            if (Q_LIKELY(p.isValid()))
                prop.value = p.read(this);
        }
    }

    m_dirtyAttributes = 0;

    return effectNode;
}

void QQuick3DEffect::onPropertyDirty()
{
    markDirty(Dirty::PropertyDirty);
}

void QQuick3DEffect::onTextureDirty()
{
    markDirty(Dirty::TextureDirty);
}

void QQuick3DEffect::markDirty(QQuick3DEffect::Dirty type)
{
    if (!(m_dirtyAttributes & quint32(type))) {
        m_dirtyAttributes |= quint32(type);
        update();
    }
}

void QQuick3DEffect::updateSceneManager(const QSharedPointer<QQuick3DSceneManager> &sceneManager)
{
    if (sceneManager) {
        for (auto it : m_dynamicTextureMaps)
            QQuick3DObjectPrivate::refSceneManager(it, sceneManager);
    } else {
        for (auto it : m_dynamicTextureMaps)
            QQuick3DObjectPrivate::derefSceneManager(it);
    }
}

void QQuick3DEffect::itemChange(QQuick3DObject::ItemChange change, const QQuick3DObject::ItemChangeData &value)
{
    if (change == QQuick3DObject::ItemSceneChange)
        updateSceneManager(value.sceneManager);
}

void QQuick3DEffect::qmlAppendPass(QQmlListProperty<QQuick3DShaderUtilsRenderPass> *list, QQuick3DShaderUtilsRenderPass *pass)
{
    if (!pass)
        return;

    QQuick3DEffect *that = qobject_cast<QQuick3DEffect *>(list->object);
    that->m_passes.push_back(pass);
}

QQuick3DShaderUtilsRenderPass *QQuick3DEffect::qmlPassAt(QQmlListProperty<QQuick3DShaderUtilsRenderPass> *list, int index)
{
    QQuick3DEffect *that = qobject_cast<QQuick3DEffect *>(list->object);
    return that->m_passes.at(index);
}

int QQuick3DEffect::qmlPassCount(QQmlListProperty<QQuick3DShaderUtilsRenderPass> *list)
{
    QQuick3DEffect *that = qobject_cast<QQuick3DEffect *>(list->object);
    return that->m_passes.count();
}

void QQuick3DEffect::qmlPassClear(QQmlListProperty<QQuick3DShaderUtilsRenderPass> *list)
{
    QQuick3DEffect *that = qobject_cast<QQuick3DEffect *>(list->object);
    that->m_passes.clear();
}

void QQuick3DEffect::setDynamicTextureMap(QQuick3DTexture *textureMap, const QByteArray &name)
{
    if (!textureMap)
        return;

    auto it = m_dynamicTextureMaps.begin();
    const auto end = m_dynamicTextureMaps.end();
    for (; it != end; ++it) {
        if (*it == textureMap)
            break;
    }

    if (it != end)
        return;

    updatePropertyListener(textureMap, nullptr, QQuick3DObjectPrivate::get(this)->sceneManager, name, m_connections, [this, name](QQuick3DObject *n) {
        setDynamicTextureMap(qobject_cast<QQuick3DTexture *>(n), name);
    });

    m_dynamicTextureMaps.push_back(textureMap);
    update();
}

QT_END_NAMESPACE
