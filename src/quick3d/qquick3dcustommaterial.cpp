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

QQuick3DCustomMaterial::QQuick3DCustomMaterial(QQuick3DObject *parent)
    : QQuick3DMaterial(*(new QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::CustomMaterial)), parent)
{
}

QQuick3DCustomMaterial::~QQuick3DCustomMaterial() {}

bool QQuick3DCustomMaterial::hasTransparency() const
{
    return m_hasTransparency;
}

bool QQuick3DCustomMaterial::hasRefraction() const
{
    return m_hasRefraction;
}

QQuick3DShaderUtilsShaderInfo *QQuick3DCustomMaterial::shaderInfo() const
{
    return m_shaderInfo;
}

QQmlListProperty<QQuick3DShaderUtilsRenderPass> QQuick3DCustomMaterial::passes()
{
    return QQmlListProperty<QQuick3DShaderUtilsRenderPass>(this,
                                                            nullptr,
                                                            QQuick3DCustomMaterial::qmlAppendPass,
                                                            QQuick3DCustomMaterial::qmlPassCount,
                                                            QQuick3DCustomMaterial::qmlPassAt,
                                                            QQuick3DCustomMaterial::qmlPassClear);
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

void QQuick3DCustomMaterial::setShaderInfo(QQuick3DShaderUtilsShaderInfo *shaderInfo)
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
    static const auto appendShaderUniform = [](const QByteArray &type, const QByteArray &name, QByteArray *shaderPrefix) {
        shaderPrefix->append(QByteArrayLiteral("uniform ") + type + " " + name + ";\n");
    };

    // Sanity check(s)
    if (!m_shaderInfo || !m_shaderInfo->isValid()) {
        qWarning("ShaderInfo is not valid!");
        return node;
    }

    QSSGRenderCustomMaterial *customMaterial = static_cast<QSSGRenderCustomMaterial *>(node);
    if (!customMaterial) {
        markAllDirty();
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
        shaderInfo.shaderPrefix = QByteArrayLiteral("#include \"customMaterial.glsllib\"\n");

        QMetaMethod propertyDirtyMethod;
        const int idx = metaObject()->indexOfSlot("onPropertyDirty()");
        if (idx != -1)
            propertyDirtyMethod = metaObject()->method(idx);

        // Properties
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

            // Track the property changes
            if (property.hasNotifySignal() && propertyDirtyMethod.isValid())
                connect(this, property.notifySignal(), this, propertyDirtyMethod);

            QVariant::Type propType = property.type();
            QVariant propValue = property.read(this);
            if (static_cast<QMetaType::Type>(propType) == QMetaType::QVariant)
                propType = propValue.type();

            if (propType == QVariant::Double) {
                appendShaderUniform(ShaderType<QVariant::Double>::name(), property.name(), &shaderInfo.shaderPrefix);
                customMaterial->properties.push_back({ property.name(), propValue, ShaderType<QVariant::Double>::type(), i});
            } else if (propType == QVariant::Bool) {
                appendShaderUniform(ShaderType<QVariant::Bool>::name(), property.name(), &shaderInfo.shaderPrefix);
                customMaterial->properties.push_back({ property.name(), propValue, ShaderType<QVariant::Bool>::type(), i});
            } else if (propType == QVariant::Vector2D) {
                appendShaderUniform(ShaderType<QVariant::Vector2D>::name(), property.name(), &shaderInfo.shaderPrefix);
                customMaterial->properties.push_back({ property.name(), propValue, ShaderType<QVariant::Vector2D>::type(), i});
            } else if (propType == QVariant::Vector3D) {
                appendShaderUniform(ShaderType<QVariant::Vector3D>::name(), property.name(), &shaderInfo.shaderPrefix);
                customMaterial->properties.push_back({ property.name(), propValue, ShaderType<QVariant::Vector3D>::type(), i});
            } else if (propType == QVariant::Vector4D) {
                appendShaderUniform(ShaderType<QVariant::Vector4D>::name(), property.name(), &shaderInfo.shaderPrefix);
                customMaterial->properties.push_back({ property.name(), propValue, ShaderType<QVariant::Vector4D>::type(), i});
            } else if (propType == QVariant::Int) {
                appendShaderUniform(ShaderType<QVariant::Int>::name(), property.name(), &shaderInfo.shaderPrefix);
                customMaterial->properties.push_back({ property.name(), propValue, ShaderType<QVariant::Int>::type(), i});
            } else if (propType == QVariant::Color) {
                appendShaderUniform(ShaderType<QVariant::Color>::name(), property.name(), &shaderInfo.shaderPrefix);
                customMaterial->properties.push_back({ property.name(), propValue, ShaderType<QVariant::Color>::type(), i});
            } else if (propType == QVariant::UserType) {
                if (property.userType() == qMetaTypeId<QQuick3DShaderUtilsTextureInput *>())
                    userProperties.push_back(property);
            } else if (static_cast<QMetaType::Type>(propType) == QMetaType::QObjectStar) {
                QObject *obj = qobject_cast<QQuick3DShaderUtilsTextureInput *>(propValue.value<QObject *>());
                if (obj)
                    userProperties.push_back(property);
            } else {
                qWarning("No know uniform convertion found for property %s. Skipping", property.name());
            }
        }

        // Textures
        for (const auto &userProperty : qAsConst(userProperties)) {
            QSSGRenderCustomMaterial::TextureProperty textureData;
            QQuick3DShaderUtilsTextureInput *texture = userProperty.read(this).value<QQuick3DShaderUtilsTextureInput *>();
            const QByteArray &name = userProperty.name();
            if (name.isEmpty()) // Warnings here will just drown in the shader error messages
                continue;
            texture->name = name;
            QQuick3DTexture *tex = texture->texture(); //
            connect(texture, &QQuick3DShaderUtilsTextureInput::textureDirty, this, &QQuick3DCustomMaterial::onTextureDirty);
            textureData.name = name;
            if (texture->enabled)
                textureData.texImage = tex->getRenderImage();
            textureData.shaderDataType = QSSGRenderShaderDataType::Texture2D;
            textureData.clampType = tex->horizontalTiling() == QQuick3DTexture::Repeat ? QSSGRenderTextureCoordOp::Repeat
                                                                                     : (tex->horizontalTiling() == QQuick3DTexture::ClampToEdge) ? QSSGRenderTextureCoordOp::ClampToEdge
                                                                                                                                               : QSSGRenderTextureCoordOp::MirroredRepeat;
            QSSGShaderUtils::addSnapperSampler(textureData.name, shaderInfo.shaderPrefix);
            customMaterial->textureProperties.push_back(textureData);
        }

        QByteArray &shared = shaderInfo.shaderPrefix;
        QByteArray vertex, geometry, fragment, shaderCode;
        if (!m_passes.isEmpty()) {
            for (const auto &pass : qAsConst(m_passes)) {
                QQuick3DShaderUtilsShader *sharedShader = pass->m_shaders.at(int(QQuick3DShaderUtilsShader::Stage::Shared));
                QQuick3DShaderUtilsShader *vertShader = pass->m_shaders.at(int(QQuick3DShaderUtilsShader::Stage::Vertex));
                QQuick3DShaderUtilsShader *fragShader = pass->m_shaders.at(int(QQuick3DShaderUtilsShader::Stage::Fragment));
                QQuick3DShaderUtilsShader *geomShader = pass->m_shaders.at(int(QQuick3DShaderUtilsShader::Stage::Geometry));
                if (!sharedShader && !vertShader && !fragShader && !geomShader) {
                    qWarning("Pass with no shader attatched!");
                    continue;
                }

                // Build up shader code
                QByteArray shaderPath;
                if (sharedShader)
                    shared += QSSGShaderUtils::resolveShader(sharedShader->shader, shaderPath, this);
                if (vertShader)
                    vertex = QSSGShaderUtils::resolveShader(vertShader->shader, shaderPath, this);
                if (fragShader)
                    fragment = QSSGShaderUtils::resolveShader(fragShader->shader, shaderPath, this);
                if (geomShader)
                    geometry = QSSGShaderUtils::resolveShader(geomShader->shader, shaderPath, this);

                shaderCode = QSSGShaderUtils::mergeShaderCode(shared, QByteArray(), QByteArray(), vertex, geometry, fragment);

                // Bind shader
                customMaterial->commands.push_back(new dynamic::QSSGBindShader(shaderPath));
                customMaterial->commands.push_back(new dynamic::QSSGApplyInstanceValue());

                // Buffers
                QQuick3DShaderUtilsBuffer *outputBuffer = pass->outputBuffer;
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

                customMaterial->shaders.insert(shaderPath, shaderCode);
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

    m_dirtyAttributes = 0;

    return customMaterial;
}

void QQuick3DCustomMaterial::onPropertyDirty()
{
    markDirty(Dirty::PropertyDirty);
    update();
}

void QQuick3DCustomMaterial::onTextureDirty(QQuick3DShaderUtilsTextureInput *texture)
{
    Q_UNUSED(texture)
    markDirty(Dirty::TextureDirty);
    update();
}

void QQuick3DCustomMaterial::qmlAppendPass(QQmlListProperty<QQuick3DShaderUtilsRenderPass> *list, QQuick3DShaderUtilsRenderPass *pass)
{
    if (!pass)
        return;

    QQuick3DCustomMaterial *that = qobject_cast<QQuick3DCustomMaterial *>(list->object);
    that->m_passes.push_back(pass);
}

QQuick3DShaderUtilsRenderPass *QQuick3DCustomMaterial::qmlPassAt(QQmlListProperty<QQuick3DShaderUtilsRenderPass> *list, int index)
{
    QQuick3DCustomMaterial *that = qobject_cast<QQuick3DCustomMaterial *>(list->object);
    return that->m_passes.at(index);
}

int QQuick3DCustomMaterial::qmlPassCount(QQmlListProperty<QQuick3DShaderUtilsRenderPass> *list)
{
    QQuick3DCustomMaterial *that = qobject_cast<QQuick3DCustomMaterial *>(list->object);
    return that->m_passes.count();
}

void QQuick3DCustomMaterial::qmlPassClear(QQmlListProperty<QQuick3DShaderUtilsRenderPass> *list)
{
    QQuick3DCustomMaterial *that = qobject_cast<QQuick3DCustomMaterial *>(list->object);
    that->m_passes.clear();
}


QT_END_NAMESPACE
