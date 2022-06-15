// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgrtutilities_p.h"

#include "qssgqmlutilities_p.h"
#include "qssgscenedesc_p.h"

#include <QtCore/qurl.h>
#include <QtCore/qbuffer.h>

#include <QtGui/qimage.h>
#include <QtGui/qimagereader.h>
#include <QtGui/qimagewriter.h>
#include <QtGui/qquaternion.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>

QT_BEGIN_NAMESPACE

static void setProperties(QQuick3DObject &obj, const QSSGSceneDesc::Node &node)
{
    using namespace QSSGSceneDesc;
    const auto &properties = node.properties;
    auto it = properties.begin();
    const auto end = properties.end();
    for (; it != end; ++it) {
        const auto &v = *it;
        if (v.value.mt.id() == qMetaTypeId<Node *>()) {
            if (const auto *node = reinterpret_cast<Node *>(v.value.dptr)) {
                Q_ASSERT(node->obj);
                v.call->set(obj, v.name, node->obj);
            }
        } else if (v.value.mt == QMetaType::fromType<Mesh>()) { // Special handling for mesh nodes.
            // Mesh nodes does not have an equivalent in the QtQuick3D scene, but is registered
            // as a source property in the intermediate scene we therefore need to convert it to
            // be a usable source url now.
            if (const auto meshNode = reinterpret_cast<const Mesh *>(v.value.dptr)) {
                const auto url = QUrl(QSSGBufferManager::runtimeMeshSourceName(node.scene->id, meshNode->idx));
                v.call->set(obj, v.name, &url);
            }
        } else if (v.value.mt == QMetaType::fromType<BufferView>()) {
            if (const auto buffer = reinterpret_cast<const BufferView *>(v.value.dptr)) {
                const QByteArray qbuffer = buffer->view.toByteArray();
                v.call->set(obj, v.name, &qbuffer);
            }
        } else if (v.value.mt == QMetaType::fromType<UrlView>()) {
            if (const auto url = reinterpret_cast<const UrlView *>(v.value.dptr)) {
                const QUrl qurl = QUrl::fromUserInput(QString::fromUtf8(url->view));
                v.call->set(obj, v.name, &qurl);
            }
        } else if (v.value.mt == QMetaType::fromType<StringView>()) {
            if (const auto string = reinterpret_cast<const StringView *>(v.value.dptr)) {
                const QString qstring(QString::fromUtf8(string->view));
                v.call->set(obj, v.name, &qstring);
            }
        } else if (v.value.mt.id() == qMetaTypeId<QSSGSceneDesc::Flag>()) {
            if (const auto flag = reinterpret_cast<const QSSGSceneDesc::Flag *>(v.value.dptr)) {
                const int qflag(flag->value);
                v.call->set(obj, v.name, &qflag);
            }
        } else {
            v.call->set(obj, v.name, v.value.dptr);
        }
    }
}

template<typename GraphObjectType, typename NodeType>
GraphObjectType *createRuntimeObject(NodeType &node, QQuick3DObject &parent)
{
    GraphObjectType *obj = qobject_cast<GraphObjectType *>(node.obj);
    if (!obj) {
        node.obj = qobject_cast<QQuick3DObject *>(obj = new GraphObjectType);
        obj->setParent(&parent);
        obj->setParentItem(&parent);
    }
    Q_ASSERT(obj == node.obj);

    return obj;
}

template<>
QQuick3DTextureData *createRuntimeObject<QQuick3DTextureData>(QSSGSceneDesc::TextureData &node, QQuick3DObject &parent)
{
    QQuick3DTextureData *obj = qobject_cast<QQuick3DTextureData *>(node.obj);
    if (!obj) {
        node.obj = qobject_cast<QQuick3DObject *>(obj = new QQuick3DTextureData);
        obj->setParent(&parent);
        obj->setParentItem(&parent);

        const auto &texData = node.data;
        const bool isCompressed = ((node.flgs & quint8(QSSGSceneDesc::TextureData::Flags::Compressed)) != 0);

        if (!texData.isEmpty()) {
            QImage image;
            if (isCompressed) {
                QByteArray data = texData.toByteArray();
                QBuffer readBuffer(&data);
                QImageReader imageReader(&readBuffer);
                image = imageReader.read();
                if (image.isNull())
                    qWarning() << imageReader.errorString();
            } else {
                const auto &size = node.sz;
                image = QImage(reinterpret_cast<const uchar *>(texData.data()), size.width(), size.height(), QImage::Format::Format_RGBA8888);
            }

            if (!image.isNull()) {
                const QPixelFormat pixFormat = image.pixelFormat();
                QImage::Format targetFormat = QImage::Format_RGBA8888_Premultiplied;
                QQuick3DTextureData::Format textureFormat = QQuick3DTextureData::Format::RGBA8;
                if (image.colorCount()) { // a palleted image
                    targetFormat = QImage::Format_RGBA8888;
                } else if (pixFormat.channelCount() == 1) {
                    targetFormat = QImage::Format_Grayscale8;
                    textureFormat = QQuick3DTextureData::Format::R8;
                } else if (pixFormat.alphaUsage() == QPixelFormat::IgnoresAlpha) {
                    targetFormat = QImage::Format_RGBX8888;
                } else if (pixFormat.premultiplied() == QPixelFormat::NotPremultiplied) {
                    targetFormat = QImage::Format_RGBA8888;
                }

                image.convertTo(targetFormat); // convert to a format mappable to QRhiTexture::Format
                image.mirror(); // Flip vertically to the conventional Y-up orientation

                const auto bytes = image.sizeInBytes();
                obj->setSize(image.size());
                obj->setFormat(textureFormat);
                obj->setTextureData(QByteArray(reinterpret_cast<const char *>(image.constBits()), bytes));
            }
        }
    }

    return obj;
}

void QSSGRuntimeUtils::createGraphObject(QSSGSceneDesc::Node &node,
                                         QList<QSSGSceneDesc::Node *> &deferredNodes,
                                         QQuick3DObject &parent, bool traverse)
{
    using namespace QSSGSceneDesc;

    QQuick3DObject *obj = nullptr;
    switch (node.nodeType) {
    case Node::Type::Skeleton:
        // NOTE: The skeleton is special as it's a resource and a node, the parent is
        // hierarchical parent is therefore important here.
        if (!node.obj) {// 1st Phase : 'create Resources'
            obj = createRuntimeObject<QQuick3DSkeleton>(static_cast<Skeleton &>(node), parent);
        } else { // 2nd Phase : setParent for the Node hierarchy.
            obj = qobject_cast<QQuick3DSkeleton *>(node.obj);
            obj->setParent(&parent);
            obj->setParentItem(&parent);
        }
        break;
    case Node::Type::Joint:
        obj = createRuntimeObject<QQuick3DJoint>(static_cast<Joint &>(node), parent);
        break;
    case Node::Type::Skin:
        obj = createRuntimeObject<QQuick3DSkin>(static_cast<Skin &>(node), parent);
        // We will update it's properties later because a property, joints,
        // is required for other nodes to be completed before
        deferredNodes.push_back(&node);
        break;
    case Node::Type::MorphTarget:
        obj = createRuntimeObject<QQuick3DMorphTarget>(static_cast<MorphTarget &>(node), parent);
        break;
    case Node::Type::Light:
    {
        auto &light = static_cast<Light &>(node);
        if (light.runtimeType == Node::RuntimeType::DirectionalLight)
            obj = createRuntimeObject<QQuick3DDirectionalLight>(light, parent);
        else if (light.runtimeType == Node::RuntimeType::PointLight)
            obj = createRuntimeObject<QQuick3DPointLight>(light, parent);
        else if (light.runtimeType == Node::RuntimeType::SpotLight)
            obj = createRuntimeObject<QQuick3DSpotLight>(light, parent);
        else
            Q_UNREACHABLE();
    }
        break;
    case Node::Type::Transform:
        obj = createRuntimeObject<QQuick3DNode>(node, parent);
        break;
    case Node::Type::Camera:
    {
        auto &camera = static_cast<Camera &>(node);
        if (camera.runtimeType == Node::RuntimeType::OrthographicCamera)
            obj = createRuntimeObject<QQuick3DOrthographicCamera>(camera, parent);
        else if (camera.runtimeType == Node::RuntimeType::PerspectiveCamera)
            obj = createRuntimeObject<QQuick3DPerspectiveCamera>(camera, parent);
        else if (camera.runtimeType == Node::RuntimeType::CustomCamera)
            obj = createRuntimeObject<QQuick3DCustomCamera>(camera, parent);
        else
            Q_UNREACHABLE();
    }
        break;
    case Node::Type::Model:
        obj = createRuntimeObject<QQuick3DModel>(static_cast<Model &>(node), parent);
        break;
    case Node::Type::Texture:
        if (node.runtimeType == Node::RuntimeType::TextureData)
            obj = createRuntimeObject<QQuick3DTextureData>(static_cast<TextureData &>(node), parent);
        else if (node.runtimeType == Node::RuntimeType::Image2D)
            obj = createRuntimeObject<QQuick3DTexture>(static_cast<Texture &>(node), parent);
        else if (node.runtimeType == Node::RuntimeType::ImageCube)
            obj = createRuntimeObject<QQuick3DCubeMapTexture>(static_cast<Texture &>(node), parent);
        else
            Q_UNREACHABLE();
        break;
    case Node::Type::Material:
    {
        if (node.runtimeType == Node::RuntimeType::PrincipledMaterial)
            obj = createRuntimeObject<QQuick3DPrincipledMaterial>(static_cast<Material &>(node), parent);
        else if (node.runtimeType == Node::RuntimeType::DefaultMaterial)
            obj = createRuntimeObject<QQuick3DDefaultMaterial>(static_cast<Material &>(node), parent);
        else if (node.runtimeType == Node::RuntimeType::CustomMaterial)
            obj = createRuntimeObject<QQuick3DCustomMaterial>(static_cast<Material &>(node), parent);
        else if (node.runtimeType == Node::RuntimeType::SpecularGlossyMaterial)
            obj = createRuntimeObject<QQuick3DSpecularGlossyMaterial>(static_cast<Material &>(node), parent);
        else
            Q_UNREACHABLE();
    }
        break;
    case Node::Type::Mesh:
        // There's no runtime object for this type, but we need to register the mesh with the
        // buffer manager (this will happen once the mesh property is processed on the model).
        break;
    }

    if (obj && traverse) {
        if (node.nodeType != Node::Type::Skin)
            setProperties(*obj, node);

        for (auto &chld : node.children)
            createGraphObject(chld, deferredNodes, *obj);
    }
}

QQuick3DNode *QSSGRuntimeUtils::createScene(QQuick3DNode &parent, const QSSGSceneDesc::Scene &scene)
{
    Q_ASSERT(scene.root);
    Q_ASSERT(QQuick3DObjectPrivate::get(&parent)->sceneManager);

    QSSGBufferManager::registerMeshData(scene.id, scene.meshStorage);

    QList<QSSGSceneDesc::Node *> deferredNodes;
    auto root = scene.root;
    for (const auto &resource : scene.resources)
        createGraphObject(*resource, deferredNodes, parent, false);

    createGraphObject(*root, deferredNodes, parent);

    // Some resources such as Skin have properties related with the node
    // heirarchy. They will be deferred to be set
    for (const auto &deferred: deferredNodes)
        setProperties(static_cast<QQuick3DObject &>(*deferred->obj), *deferred);

    // Usually it makes sense to only enable 1 timeline at a time
    // so for now we just enable the first one.
    bool isFirstAnimation = true;
    for (const auto &anim: scene.animations) {
        QSSGQmlUtilities::createTimelineAnimation(*anim, root->obj, isFirstAnimation);
        if (isFirstAnimation)
            isFirstAnimation = false;
    }

    return qobject_cast<QQuick3DNode *>(scene.root->obj);
}

QT_END_NAMESPACE
