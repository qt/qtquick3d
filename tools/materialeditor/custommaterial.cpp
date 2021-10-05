/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "custommaterial.h"

#include <QtQuick3DAssetUtils/private/qssgrtutilities_p.h>
#include <QtQuick3DAssetUtils/private/qssgqmlutilities_p.h>

#include <QtQuick3D/private/qquick3dshaderutils_p.h>

QT_BEGIN_NAMESPACE

namespace QSSGSceneDesc {

struct TextureInput : QSSGSceneDesc::Node
{
    explicit TextureInput(QByteArrayView uName, QImage *image)
        : Node("TextureInput", Node::Type::Texture, Node::RuntimeType::Unknown)
        , uniformName(uName)
        , texture(image) {}
    using type = QQuick3DShaderUtilsTextureInput;
    QByteArrayView uniformName;
    QImage *texture = nullptr;
};
QSSG_DECLARE_NODE(TextureInput)

}

using TextureStore = QHash<QString, QImage *>;
Q_GLOBAL_STATIC(TextureStore, s_textureStore);

static QByteArrayView fromQString(QSSGSceneDesc::Scene::Allocator &allocator, const QString &s)
{
    const auto string = s.toUtf8();
    const qsizetype size = string.size();
    if (size > 0) {
        const qsizetype asize = size + 1;
        char *data = reinterpret_cast<char *>(allocator.allocate(asize));
        qstrncpy(data, string.constData(), size + 1);
        return QByteArrayView{data, size};
    }

    return QByteArrayView();
}

template<typename T>
static void setProperty(QQuick3DObject &obj, const char *name, T v)
{
    if (QQuick3DObjectPrivate::get(&obj)->type == QQuick3DObjectPrivate::Type::CustomMaterial) {
        if constexpr (std::is_same_v<T, QSSGSceneDesc::TextureInput *>) {
            auto ti = static_cast<QSSGSceneDesc::TextureInput *>(v);
            if (!ti->texture->isNull()) {
                // Sigh...
                auto texData = new QQuick3DTextureData(&obj);
                const auto d = (const char *)ti->texture->constBits();
                const auto s = ti->texture->sizeInBytes();
                texData->setTextureData({d, s});
                texData->setSize(ti->texture->size());
                auto tex = new QQuick3DTexture(&obj);
                tex->setTextureData(texData);
                auto texUniform = new QQuick3DShaderUtilsTextureInput(&obj);
                texUniform->setTexture(tex);
                obj.setProperty(name, QVariant::fromValue(texUniform));
            }
        } else {
            obj.setProperty(name, QVariant::fromValue(v));
        }
    }
}

void CustomMaterial::setUniform(QSSGSceneDesc::Material &material, const Uniform &uniform)
{
    if (uniform.name.isEmpty()) {
        qWarning() << "Uniform without name! Skipping";
        return;
    }

    constexpr auto Dynamic = QSSGSceneDesc::Property::Type::Dynamic;

    switch (uniform.type) {
    case Uniform::Type::Bool:
        QSSGSceneDesc::setProperty(material, uniform.name.constData(), &setProperty<bool>, uniform.b, Dynamic);
        break;
    case Uniform::Type::Int:
        QSSGSceneDesc::setProperty(material, uniform.name.constData(), &setProperty<int>, uniform.i, Dynamic);
        break;
    case Uniform::Type::Float:
        QSSGSceneDesc::setProperty(material, uniform.name.constData(), &setProperty<float>, uniform.f, Dynamic);
        break;
    case Uniform::Type::Vec2:
        QSSGSceneDesc::setProperty(material, uniform.name.constData(), &setProperty<QVector2D>, uniform.vec2, Dynamic);
        break;
    case Uniform::Type::Vec3:
        QSSGSceneDesc::setProperty(material, uniform.name.constData(), &setProperty<QVector3D>, uniform.vec3, Dynamic);
        break;
    case Uniform::Type::Vec4:
        QSSGSceneDesc::setProperty(material, uniform.name.constData(), &setProperty<QVector4D>, uniform.vec4, Dynamic);
        break;
    case Uniform::Type::Mat44:
        QSSGSceneDesc::setProperty(material, uniform.name.constData(), &setProperty<QMatrix4x4>, uniform.m44, Dynamic);
        break;
    case Uniform::Type::Sampler:
    {
        const auto &path = uniform.imagePath;
        if (!path.isEmpty()) {
            auto it = s_textureStore->constFind(path);
            const auto end = s_textureStore->constEnd();
            if (it == end) {
                QImage image;
                if (image.load(path))
                    it = s_textureStore->insert(path, new QImage(image));
                else
                    qWarning("Failed to load image %s", qPrintable(path));
            }

            if (it != end) {
                auto texInputNode = material.scene->create<QSSGSceneDesc::TextureInput>(uniform.name.constData(), *it);
                material.scene->resources.push_back(texInputNode);
                QSSGSceneDesc::setProperty(material, uniform.name.constData(), &setProperty<QSSGSceneDesc::TextureInput *>, texInputNode, Dynamic);
            }
        }
        break;
    }
    case Uniform::Type::Last:
        Q_UNREACHABLE();
        break;
    }
}

QPointer<QQuick3DCustomMaterial> CustomMaterial::create(QQuick3DObject &parent, const UniformTable &uniforms, const Properties &properties, const Shaders &shaders)
{
    using namespace QSSGSceneDesc;
    QQuick3DCustomMaterial *ret = nullptr;
    if (scene.root)
        scene.reset();

    QSSGSceneDesc::Material *material = scene.create<Material>(Material::RuntimeType::CustomMaterial);
    addNode(scene, *material);

    // Set uniforms
    for (const auto &uniform : uniforms)
        setUniform(*material, uniform);

    // Set common properties
    const Properties def;
    if (def.cullMode != properties.cullMode)
        setProperty(*material, "cullMode", &QQuick3DCustomMaterial::setCullMode, properties.cullMode);
    if (def.depthDrawMode != properties.depthDrawMode)
        setProperty(*material, "depthDrawMode", &QQuick3DCustomMaterial::setDepthDrawMode, properties.depthDrawMode);
    if (def.shadingMode != properties.shadingMode)
        setProperty(*material, "shadingMode", &QQuick3DCustomMaterial::setShadingMode, properties.shadingMode);
    if (def.sourceBlend != properties.sourceBlend)
        setProperty(*material, "sourceBlend", &QQuick3DCustomMaterial::setSrcBlend, properties.sourceBlend);
    if (def.destinationBlend != properties.destinationBlend)
        setProperty(*material, "destinationBlend", &QQuick3DCustomMaterial::setDstBlend, properties.destinationBlend);

    if (!shaders.vert.isEmpty())
        setProperty(*material, "vertexShader", &QQuick3DCustomMaterial::setVertexShader, QSSGSceneDesc::UrlView{ {fromQString(scene.allocator, shaders.vert.toString())} });
    if (!shaders.frag.isEmpty())
        setProperty(*material, "fragmentShader", &QQuick3DCustomMaterial::setFragmentShader, QSSGSceneDesc::UrlView{ {fromQString(scene.allocator, shaders.frag.toString())} });

    auto resourceParent = std::make_unique<QQuick3DNode>();
    QSSGRuntimeUtils::createGraphObject(*material, parent);
    if (auto customMaterial = qobject_cast<QQuick3DCustomMaterial *>(material->obj)) {
        resourceParent.release()->setParent(&parent);
        ret = customMaterial;
    }

    return ret;
}

bool CustomMaterial::isValid() const { return scene.root != nullptr && scene.root->runtimeType == QSSGSceneDesc::Material::RuntimeType::CustomMaterial; }

QDataStream &CustomMaterial::readFromDataStream(QDataStream &stream, Uniform &uniform)
{
    stream >> uniform.type >> uniform.name;
    switch (uniform.type) {
    case Uniform::Type::Bool:
        stream >> uniform.b;
        break;
    case Uniform::Type::Int:
        stream >> uniform.i;
        break;
    case Uniform::Type::Float:
        stream >> uniform.f;
        break;
    case Uniform::Type::Vec2:
        stream >> uniform.vec2;
        break;
    case Uniform::Type::Vec3:
        stream >> uniform.vec3;
        break;
    case Uniform::Type::Vec4:
        stream >> uniform.vec4;
        break;
    case Uniform::Type::Mat44:
        stream >> uniform.m44;
        break;
    case Uniform::Type::Sampler:
    {
        QImage image;
        stream >> uniform.imagePath >> image;
        s_textureStore->insert(uniform.imagePath, new QImage(image));
    }
        break;
    case Uniform::Type::Last:
        Q_UNREACHABLE();
        break;
    }
    return stream;
}

QDataStream &CustomMaterial::writeToDataStream(QDataStream &stream, const Uniform &uniform)
{
    stream << uniform.type << uniform.name;
    switch (uniform.type) {
    case Uniform::Type::Bool:
        stream << uniform.b;
        break;
    case Uniform::Type::Int:
        stream << uniform.i;
        break;
    case Uniform::Type::Float:
        stream << uniform.f;
        break;
    case Uniform::Type::Vec2:
        stream << uniform.vec2;
        break;
    case Uniform::Type::Vec3:
        stream << uniform.vec3;
        break;
    case Uniform::Type::Vec4:
        stream << uniform.vec4;
        break;
    case Uniform::Type::Mat44:
        stream << uniform.m44;
        break;
    case Uniform::Type::Sampler:
    {
        const auto &path = uniform.imagePath;
        auto it = s_textureStore->constFind(path);
        const auto end = s_textureStore->cend();
        if (it != end)
            stream << path << *(*it);
    }
        break;
    case Uniform::Type::Last:
        Q_UNREACHABLE();
        break;
    }
    return stream;
}

void CustomMaterial::writeQmlComponent(const QSSGSceneDesc::Material &material, QTextStream &stream)
{
    QSSGQmlUtilities::writeQmlComponent(material, stream);
}

QT_END_NAMESPACE
