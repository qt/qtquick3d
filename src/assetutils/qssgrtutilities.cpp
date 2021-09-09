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
                v.call->set(obj, node->obj);
            }
        } else if (v.value.mt == QMetaType::fromType<Mesh>()) { // Special handling for mesh nodes.
            // Mesh nodes does not have an equivalent in the QtQuick3D scene, but is registered
            // as a source property in the intermediate scene we therefore need to convert it to
            // be a usable source url now.
            if (const auto meshNode = reinterpret_cast<const Mesh *>(v.value.dptr)) {
                const auto url = QUrl(QSSGBufferManager::runtimeMeshSourceName(node.scene->id, meshNode->idx));
                v.call->set(obj, &url);
            }
        } else if (v.value.mt == QMetaType::fromType<BufferView>()) {
            if (const auto buffer = reinterpret_cast<const BufferView *>(v.value.dptr)) {
                const QByteArray qbuffer = buffer->view.toByteArray();
                v.call->set(obj, &qbuffer);
            }
        } else if (v.value.mt == QMetaType::fromType<UrlView>()) {
            if (const auto url = reinterpret_cast<const UrlView *>(v.value.dptr)) {
                const QUrl qurl = QUrl::fromUserInput(QString::fromUtf8(url->view));
                v.call->set(obj, &qurl);
            }
        } else if (v.value.mt == QMetaType::fromType<StringView>()) {
            if (const auto string = reinterpret_cast<const StringView *>(v.value.dptr)) {
                const QString qstring(QString::fromUtf8(string->view));
                v.call->set(obj, &qstring);
            }
        } else {
            v.call->set(obj, v.value.dptr);
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

static void createGraphObject(QSSGSceneDesc::Node &node, QQuick3DObject &parent, bool traverse = true)
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
        else
            obj = createRuntimeObject<QQuick3DTexture>(static_cast<Texture &>(node), parent);
        break;
    case Node::Type::Material:
    {
        if (node.runtimeType == Node::RuntimeType::PrincipledMaterial)
            obj = createRuntimeObject<QQuick3DPrincipledMaterial>(static_cast<Material &>(node), parent);
        else if (node.runtimeType == Node::RuntimeType::DefaultMaterial)
            obj = createRuntimeObject<QQuick3DDefaultMaterial>(static_cast<Material &>(node), parent);
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
        setProperties(*obj, node);

        for (auto &chld : node.children)
            createGraphObject(chld, *obj);
    }
}

QQuick3DNode *QSSGRuntimeUtils::createScene(QQuick3DNode &parent, const QSSGSceneDesc::Scene &scene)
{
    Q_ASSERT(scene.root);
    Q_ASSERT(QQuick3DObjectPrivate::get(&parent)->sceneManager);

    QSSGBufferManager::registerMeshData(scene.id, scene.meshStorage);

    auto root = scene.root;
    for (const auto &resource : scene.resources)
        createGraphObject(*resource, parent, false);

    createGraphObject(*root, parent);

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
