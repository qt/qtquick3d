// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSG_RENDER_GRAPH_OBJECT_H
#define QSSG_RENDER_GRAPH_OBJECT_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>
#include <QtCore/QString>

#define QSSG_DEBUG_ID 0

QT_BEGIN_NAMESPACE

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderGraphObject
{
    // Types should be setup on construction. Change the type
    // at your own risk as the type is used for RTTI purposes.
    // See QSSGRenderGraphObject, QQuick3DObject and QSSceneDesc (asset useage).

    enum BaseType : quint16
    {
        Node = 0x10,
        Light = 0x20,
        Camera = 0x40,
        Renderable = 0x80,
        Resource = 0x100,
        Material = 0x200,
        Texture = 0x400
    };

    enum class Type : quint16 {
        Unknown = 0,
        // Nodes
        Node = BaseType::Node,
        Layer, // Node
        Joint, // Node
        Skeleton, // Node (A resource to the model node)
        ImportScene, // Node
        ReflectionProbe,
        // Light nodes
        DirectionalLight = BaseType::Light | BaseType::Node,
        PointLight,
        SpotLight,
        // Camera nodes
        OrthographicCamera = BaseType::Camera | BaseType::Node,
        PerspectiveCamera,
        CustomFrustumCamera, // Perspective camera with user specified frustum bounds.
        CustomCamera,
        // Renderable nodes
        Model = BaseType::Renderable | BaseType::Node, // Renderable Node
        Item2D, // Renderable Node
        Particles, // Renderable Node
        // Resources
        SceneEnvironment = BaseType::Resource, // Resource
        Effect, // Resource
        Geometry, // Resource
        TextureData, // Resource
        MorphTarget, // Resource
        ModelInstance, // Resource
        ModelBlendParticle, // Resource
        ResourceLoader, // Resource [meta]
        // Materials
        DefaultMaterial = BaseType::Material | BaseType::Resource, // Resource
        PrincipledMaterial, // Resource
        CustomMaterial, // Resource
        SpecularGlossyMaterial, //Resource
        Skin, // Resource
        // Textures
        Image2D = BaseType::Texture | BaseType::Resource, // Resource
        ImageCube, // Resource
    };

    Q_REQUIRED_RESULT static inline constexpr bool isNodeType(Type type) Q_DECL_NOTHROW
    {
        return (quint16(type) & BaseType::Node);
    }

    Q_REQUIRED_RESULT static inline constexpr bool isLight(Type type) Q_DECL_NOTHROW
    {
        return (quint16(type) & BaseType::Light);
    }

    Q_REQUIRED_RESULT static inline constexpr bool isCamera(Type type) Q_DECL_NOTHROW
    {
        return (quint16(type) & BaseType::Camera);
    }

    Q_REQUIRED_RESULT static inline constexpr bool isMaterial(Type type) Q_DECL_NOTHROW
    {
        return (quint16(type) & BaseType::Material);
    }

    Q_REQUIRED_RESULT static inline constexpr bool isTexture(Type type) Q_DECL_NOTHROW
    {
        return (quint16(type) & BaseType::Texture);
    }

    Q_REQUIRED_RESULT static inline constexpr bool isRenderable(Type type) Q_DECL_NOTHROW
    {
        return (quint16(type) & BaseType::Renderable);
    }

    Q_REQUIRED_RESULT static inline constexpr bool isResource(Type type) Q_DECL_NOTHROW
    {
        return (quint16(type) & BaseType::Resource);
    }

    // These require special handling, see cleanupNodes() in the scene manager.
    Q_REQUIRED_RESULT static inline constexpr bool hasGraphicsResources(Type type) Q_DECL_NOTHROW
    {
        return ((type == Type::Model)
                || (isTexture(type))
                || (type == Type::Geometry)
                || (type == Type::TextureData)
                || (type == Type::ResourceLoader));
    }

    QAtomicInt ref;
    // Id's help debugging the object and are optionally set
#if QSSG_DEBUG_ID
    QByteArray id;
#endif
    // Type is used for RTTI purposes down the road.
    Type type;

    explicit QSSGRenderGraphObject(QSSGRenderGraphObject::Type inType) : type(inType) {}
    virtual ~QSSGRenderGraphObject();

    Q_DISABLE_COPY_MOVE(QSSGRenderGraphObject)
};

QT_END_NAMESPACE

#endif
