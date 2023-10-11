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
#include <QtQuick3DUtils/private/qquick3dprofiler_p.h>
#include <QtCore/QString>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderGraphObject
{
    Q_DISABLE_COPY_MOVE(QSSGRenderGraphObject)
public:
    // Types should be setup on construction. Change the type
    // at your own risk as the type is used for RTTI purposes.
    // See QSSGRenderGraphObject, QQuick3DObject and QSSceneDesc (asset useage).

    enum BaseType : quint32 {
        // Internal types
        Node = 0x1000,
        Light = 0x2000,
        Camera = 0x4000,
        Renderable = 0x8000,
        Resource = 0x10000,
        Material = 0x20000,
        Texture = 0x40000,
        Extension = 0x80000,
        User =   0x80000000
    };

    enum class Type : quint32 {
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
        RenderExtension = BaseType::Extension, // Extension
        // User types E.g.: (User | Node) + 1)
    };
    using TypeT = std::underlying_type_t<Type>;

    enum class Flags : quint32 {
        HasGraphicsResources = 0x1
    };
    using FlagT = std::underlying_type_t<Flags>;

    [[nodiscard]] static inline constexpr bool isNodeType(Type type) noexcept
    {
        return (TypeT(type) & BaseType::Node);
    }

    [[nodiscard]] static inline constexpr bool isLight(Type type) noexcept
    {
        return (TypeT(type) & BaseType::Light);
    }

    [[nodiscard]] static inline constexpr bool isCamera(Type type) noexcept
    {
        return (TypeT(type) & BaseType::Camera);
    }

    [[nodiscard]] static inline constexpr bool isMaterial(Type type) noexcept
    {
        return (TypeT(type) & BaseType::Material);
    }

    [[nodiscard]] static inline constexpr bool isTexture(Type type) noexcept
    {
        return (TypeT(type) & BaseType::Texture);
    }

    [[nodiscard]] static inline constexpr bool isRenderable(Type type) noexcept
    {
        return (TypeT(type) & BaseType::Renderable);
    }

    [[nodiscard]] static inline constexpr bool isResource(Type type) noexcept
    {
        return (TypeT(type) & BaseType::Resource);
    }

    [[nodiscard]] static inline constexpr bool isExtension(Type type) noexcept
    {
        return (TypeT(type) & BaseType::Extension);
    }

    // Note: This could have been a non-static member, as we never create or use
    // user types we do the built-in types; In any case just follow the existing pattern.
    [[nodiscard]] static inline constexpr bool isUserType(Type type) noexcept
    {
        return (TypeT(type) & BaseType::User);
    }

    // Objects tagged with HasGraphicsResources get special handling and will be queued for release
    // on the render thread after the frame has been submitted. See cleanupNodes() in the scene manager.
    [[nodiscard]] inline bool hasGraphicsResources() const noexcept
    {
        return ((flags & FlagT(Flags::HasGraphicsResources)) != 0);
    }

    const Type type;
    FlagT flags { 0 };
    Q_QUICK3D_PROFILE_ID

    explicit QSSGRenderGraphObject(QSSGRenderGraphObject::Type inType);
    explicit QSSGRenderGraphObject(QSSGRenderGraphObject::Type inType, FlagT inFlags) : type(inType), flags(inFlags) {}
    virtual ~QSSGRenderGraphObject();

#ifndef QT_NO_DEBUG_STREAM
    friend Q_QUICK3DRUNTIMERENDER_EXPORT QDebug operator<<(QDebug stream, QSSGRenderGraphObject::Type type);
#endif
};

#ifndef QT_NO_DEBUG_STREAM
Q_QUICK3DRUNTIMERENDER_EXPORT QDebug operator<<(QDebug, QSSGRenderGraphObject::Type type);
#endif

QT_END_NAMESPACE

#endif
