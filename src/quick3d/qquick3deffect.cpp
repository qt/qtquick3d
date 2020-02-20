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
#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick3D/private/qquick3dobject_p.h>
#include <QtCore/qfile.h>
#include <QtCore/qurl.h>


QT_BEGIN_NAMESPACE


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
    } else {
        Q_ASSERT(0);
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
    } else {
        Q_ASSERT(0);
    }

    return QSSGRenderShaderDataType::Unknown;
}

QQuick3DObject::Type QQuick3DEffect::type() const
{
    return QQuick3DObject::Effect;
}

QQmlListProperty<QQuick3DShaderUtilsRenderPass> QQuick3DEffect::passes()
{
    return QQmlListProperty<QQuick3DShaderUtilsRenderPass>(this,
                                                      nullptr,
                                                      QQuick3DEffect::qmlAppendPass,
                                                      QQuick3DEffect::qmlPassCount,
                                                      QQuick3DEffect::qmlPassAt,
                                                      nullptr);
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

    static const auto addUniform = [](const QMetaProperty &property, QByteArray &uniforms) {
        uniforms += QByteArray("uniform ") + uniformTypeName(property.type()) + " " + property.name() + ";\n";
    };

    const auto &renderContext
            = QSSGRenderContextInterface::getRenderContextInterface(quintptr(window));

    QSSGRenderEffect *effectNode = static_cast<QSSGRenderEffect *>(node);
    if (!effectNode) {
        QByteArray shared = QByteArrayLiteral("#include \"effect.glsllib\"\n");

        markAllDirty();
        effectNode = new QSSGRenderEffect;
        effectNode->setActive(true, *renderContext->effectSystem());

        QMetaMethod propertyDirtyMethod;
        const int idx = metaObject()->indexOfSlot("onPropertyDirty()");
        if (idx != -1)
            propertyDirtyMethod = metaObject()->method(idx);

        // Properties -> uniforms
        QByteArray uniforms;
        const int propCount = metaObject()->propertyCount();
        const int propOffset = metaObject()->propertyOffset();
        QVector<QMetaProperty> textureProperties; // We'll deal with these later
        for (int i = propOffset; i != propCount; ++i) {
            const auto property = metaObject()->property(i);
            if (Q_UNLIKELY(!property.isValid()))
                continue;

            if (property.type() == QVariant::UserType) {
                if (property.userType() == qMetaTypeId<QQuick3DShaderUtilsTextureInput *>())
                    textureProperties.push_back(property);
            } else {
                addUniform(property, uniforms);
                effectNode->properties.push_back({ property.name(), property.read(this), uniformType(property.type()), i});
                // Track the property changes
                if (property.hasNotifySignal() && propertyDirtyMethod.isValid())
                    connect(this, property.notifySignal(), this, propertyDirtyMethod);
            }
        }

        // Textures
        QByteArray textureData;
        for (const auto &property : qAsConst(textureProperties)) {
            QSSGRenderEffect::TextureProperty texProp;
            QQuick3DShaderUtilsTextureInput *texture = property.read(this).value<QQuick3DShaderUtilsTextureInput *>();
            const QByteArray &name = property.name();
            if (name.isEmpty()) // Warnings here will just drown in the shader error messages
                continue;
            QQuick3DTexture *tex = texture->texture(); //
            connect(texture, &QQuick3DShaderUtilsTextureInput::textureDirty, this, &QQuick3DEffect::onTextureDirty);
            texProp.name = name;
            if (texture->enabled)
                texProp.texImage = tex->getRenderImage();
            texProp.shaderDataType = QSSGRenderShaderDataType::Texture2D;
            texProp.clampType = tex->horizontalTiling() == QQuick3DTexture::Repeat ? QSSGRenderTextureCoordOp::Repeat
                                                                                       : (tex->horizontalTiling() == QQuick3DTexture::ClampToEdge) ? QSSGRenderTextureCoordOp::ClampToEdge
                                                                                                                                                   : QSSGRenderTextureCoordOp::MirroredRepeat;
            QSSGShaderUtils::addSnapperSampler(texProp.name, textureData);
            effectNode->textureProperties.push_back(texProp);
        }

        if (!m_passes.isEmpty()) {
            QByteArray vertex, geometry, fragment, shaderCode;
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
                    shared += QSSGShaderUtils::resolveShader(sharedShader->shader, shaderPath);
                if (vertShader)
                    vertex = QSSGShaderUtils::resolveShader(vertShader->shader, shaderPath);
                if (fragShader)
                    fragment = QSSGShaderUtils::resolveShader(fragShader->shader, shaderPath);
                if (geomShader)
                    geometry = QSSGShaderUtils::resolveShader(geomShader->shader, shaderPath);


                shaderCode = QSSGShaderUtils::mergeShaderCode(shared, uniforms, textureData, vertex, geometry, fragment);

                // Bind shader
                effectNode->commands.push_back(new dynamic::QSSGBindShader(shaderPath));
                effectNode->commands.push_back(new dynamic::QSSGApplyInstanceValue());

                // Buffers
                QQuick3DShaderUtilsBuffer *outputBuffer = pass->outputBuffer;
                if (outputBuffer) {
                    const QByteArray &outBufferName = outputBuffer->name;
                    if (outBufferName.isEmpty()) {
                        // default output buffer (with settings)
                        auto outputFormat = QQuick3DShaderUtilsBuffer::mapTextureFormat(outputBuffer->format());
                        effectNode->commands.push_back(new dynamic::QSSGBindTarget(outputFormat));
                        effectNode->outputFormat = outputFormat;
                    } else {
                        // Allocate buffer command
                        effectNode->commands.push_back(outputBuffer->getCommand());
                        // bind buffer
                        effectNode->commands.push_back(new dynamic::QSSGBindBuffer(outBufferName, true));
                    }
                } else {
                    // Use the default output buffer, same format as the source buffer
                    effectNode->commands.push_back(new dynamic::QSSGBindTarget(QSSGRenderTextureFormat::Unknown));
                    effectNode->outputFormat = QSSGRenderTextureFormat::Unknown;
                }

                // Other commands (BufferInput, Blending ... )
                const auto &extraCommands = pass->m_commands;
                for (const auto &command : extraCommands) {
                    const int bufferCount = command->bufferCount();
                    for (int i = 0; i != bufferCount; ++i)
                        effectNode->commands.push_back(command->bufferAt(i)->getCommand());
                    effectNode->commands.push_back(command->getCommand());
                }

                effectNode->commands.push_back(new dynamic::QSSGRender);

                renderContext->effectSystem()->setShaderData(shaderPath, shaderCode, "GLSL", "330", false, false);
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

void QQuick3DEffect::onTextureDirty(QQuick3DShaderUtilsTextureInput *texture)
{
    Q_UNUSED(texture)
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

