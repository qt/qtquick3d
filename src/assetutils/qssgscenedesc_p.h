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

#ifndef QSSGSCENEDESCRIPTION_P_H
#define QSSGSCENEDESCRIPTION_P_H

#include <QtQuick3DAssetUtils/private/qtquick3dassetutilsglobal_p.h>
#include <QtQuick3DAssetUtils/private/qssgscenedesc_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>
#include <QtQuick3DRuntimeRender/private/qssgperframeallocator_p.h>
#include <QtQuick3DUtils/private/qssgmesh_p.h>

#include <QtCore/qlist.h>
#include <QtCore/qhash.h>
#include <QtCore/qvariant.h>
#include <QtQml/qqmllist.h>

// QtQuick3D
#include <QtQuick3D/private/qquick3dobject_p.h>
// Materials
#include <QtQuick3D/private/qquick3ddefaultmaterial_p.h>
#include <QtQuick3D/private/qquick3dprincipledmaterial_p.h>
#include <QtQuick3D/private/qquick3dmodel_p.h>
// cameras
#include <QtQuick3D/private/qquick3dorthographiccamera_p.h>
#include <QtQuick3D/private/qquick3dperspectivecamera_p.h>
#include <QtQuick3D/private/qquick3dcustomcamera_p.h>
// Lights
#include <QtQuick3D/private/qquick3ddirectionallight_p.h>
#include <QtQuick3D/private/qquick3dpointlight_p.h>
#include <QtQuick3D/private/qquick3dspotlight_p.h>
// Texture
#include <QtQuick3D/private/qquick3dtexture_p.h>
#include <QtQuick3D/private/qquick3dtexturedata_p.h>
//
#include <QtQuick3D/private/qquick3dskeleton_p.h>
#include <QtQuick3D/private/qquick3djoint_p.h>

#include <qmetatype.h>
#include <QtQuick3DUtils/private/qssginvasivelinkedlist_p.h>


QT_BEGIN_NAMESPACE

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

namespace QSSGSceneDesc
{

struct Node;

struct Scene
{
    using ResourceNodes = QVarLengthArray<Node *>;
    using MeshStorage = QVector<QSSGMesh::Mesh>;
    using Allocator = QSSGPerFrameAllocator;

    // Root node, usually an empty 'transform' node.
    Node *root = nullptr;
    ResourceNodes resources;
    Allocator allocator;
    MeshStorage meshStorage;

    template<typename T, typename... Args>
    Q_REQUIRED_RESULT inline T *create(Args&&... args)
    {
        return new (allocator.allocate(sizeof (T)))T(std::forward<Args>(args)...);
    }

    void reset();
};

struct Q_QUICK3DASSETUTILS_EXPORT PropertyCall
{
    virtual bool set(QQuick3DObject &, const void *) = 0;
    virtual bool get(const QQuick3DObject &, const void *[]) const = 0;
};

struct Value
{
    QMetaType mt;
    void *dptr;
};

struct BufferView {
    using type = QByteArray;
    QByteArrayView view;
};
struct UrlView : BufferView { using type = QUrl; };
struct StringView : BufferView { using type = QString; };

struct Property
{
    Value value;
    const char *name = nullptr;
    QSSGSceneDesc::PropertyCall *call = nullptr;
    Property *next = nullptr;
};

struct NodeList
{
    Node **head = nullptr;
    qsizetype count = -1;
};

struct Node
{
    // Node type
    enum class Type : quint8
    {
        Transform,
        Camera,
        Model,
        Texture,
        Material,
        Light,
        Mesh,
        Skeleton,
        Joint
    };

    // Runtime type type mapping between this type and the QtQuick3D type
    using RuntimeType = QSSGRenderGraphObject::Type;

    explicit Node(Node::Type type, Node::RuntimeType rt)
        : runtimeType(rt)
        , nodeType(type) {}

    Scene *scene = nullptr;
    QObject *obj = nullptr;
    Node *next = nullptr;
    using ChildList = QSSGInvasiveSingleLinkedList<Node, &Node::next>;
    using PropertyList = QSSGInvasiveSingleLinkedList<Property, &Property::next>;
    ChildList children;
    PropertyList properties;
    quint16 id = 0;
    RuntimeType runtimeType;
    Type nodeType;
};

static_assert(std::is_trivially_copy_constructible_v<Node>, "Nope");

struct Texture : Node
{
    Texture() : Node(Node::Type::Texture, RuntimeType::Image) {}
};

struct TextureData : Node
{
    enum class Flags : quint8
    {
        Compressed = 0x1
    };

    using Format = QQuick3DTextureData::Format;
    explicit TextureData(QByteArrayView dataref, QSize size, Format format, quint8 flags = 0)
        : Node(Node::Type::Texture, RuntimeType::TextureData)
        , data(dataref)
        , sz(size)
        , fmt(format)
        , flgs(flags)
    {}
    QByteArrayView data;
    QSize sz;
    Format fmt;
    quint8 flgs;
};

struct Material : Node
{
    explicit Material(Node::RuntimeType rt) : Node(Node::Type::Material, rt) {}
};

struct Mesh : Node
{
    explicit Mesh(QByteArrayView name, qsizetype index)
        : Node(Node::Type::Mesh, RuntimeType::Node)
        , name(name)
        , idx(index)
    {}
    QByteArrayView name;
    qsizetype idx; // idx to the mesh data in the mesh data storage (see Scene).
};

struct Model : Node
{
    Model() : Node(Node::Type::Model, Node::RuntimeType::Model) {}
};

struct Camera : Node
{
    explicit Camera(RuntimeType rt) : Node(Node::Type::Camera, rt) {}
};

struct Light : Node
{
    explicit Light(RuntimeType rt) : Node(Node::Type::Light, rt) {}
};

struct Skeleton : Node
{
    Skeleton() : Node(Node::Type::Skeleton, Node::RuntimeType::Skeleton) {}
};

struct Joint : Node
{
    Joint() : Node(Node::Type::Joint, Node::RuntimeType::Joint) {}
};


// Add a child node to parent node.
Q_QUICK3DASSETUTILS_EXPORT void addNode(Node &parent, Node &node);
// Add node to the scene, if a node is already set the new node will
// become a child of the root node.
Q_QUICK3DASSETUTILS_EXPORT void addNode(Scene &scene, Node &node);


template <typename T> struct TypeMap {};
template <> struct TypeMap<QSSGSceneDesc::Node> { using type = QQuick3DNode; };
template <> struct TypeMap<QSSGSceneDesc::Texture> { using type = QQuick3DTexture; };
template <> struct TypeMap<QSSGSceneDesc::TextureData> { using type = QQuick3DTextureData; };
template <> struct TypeMap<QSSGSceneDesc::Material> { using type = QQuick3DMaterial; };
template <> struct TypeMap<QSSGSceneDesc::Mesh> { using type = QQuick3DNode; };
template <> struct TypeMap<QSSGSceneDesc::Model> { using type = QQuick3DModel; };
template <> struct TypeMap<QSSGSceneDesc::Camera> { using type = QQuick3DCamera; };
template <> struct TypeMap<QSSGSceneDesc::Light> { using type = QQuick3DAbstractLight; };

template <> struct TypeMap<QQuick3DMaterial> { using type = QSSGSceneDesc::Material; };


template <typename T>
struct FuncType
{
    enum { value = 0 };
};

template <typename R, typename C, typename... A>
struct FuncType<R (C::*)(A...)>
{
    enum { value = sizeof... (A) == 1 };
    using Ret = R;
    using Class = C;
    // For now we only care about single argument functions
    using Arg0 = typename std::tuple_element_t<0, std::tuple<A...>>;
    using Arg0Base = typename std::remove_cv_t<typename std::remove_reference_t<typename std::remove_pointer_t<Arg0>>>;
};

template <typename T, typename C>
struct FuncType<QQmlListProperty<T> (C::*)()>
{
    enum { value = 2 };
    using Ret = QQmlListProperty<T>;
    using Class = C;
};

template <typename Ret, typename Class, typename Arg>
struct PropertySetter : PropertyCall
{
    using Setter = Ret (Class::*)(Arg);
    constexpr explicit PropertySetter(Setter fn) : call(fn) {}
    Setter call = nullptr;
    bool get(const QQuick3DObject &, const void *[]) const override { return false; }
    bool set(QQuick3DObject &that, const void *value) override
    {
        if (value) {
            if constexpr (std::is_pointer_v<typename FuncType<Setter>::Arg0>)
                (qobject_cast<Class *>(&that)->*call)(reinterpret_cast<typename FuncType<Setter>::Arg0>(const_cast<void *>(value)));
            else
                (qobject_cast<Class *>(&that)->*call)(*reinterpret_cast<typename FuncType<Setter>::Arg0Base *>(const_cast<void *>(value)));
            return true;
        }

        return false;
    }
};

template <typename Class, typename T, template <typename> typename List>
struct PropertyList : PropertyCall
{
    using ListType = List<T>;
    using ListFunc = ListType (Class::*)();
    constexpr explicit PropertyList(ListFunc fn) : listfn(fn) {}
    ListFunc listfn = nullptr;

    static_assert(std::is_same_v<ListType, QQmlListProperty<T>>, "Expected QQmlListProperty!");

    bool get(const QQuick3DObject &, const void *[]) const override { return false; }
    bool set(QQuick3DObject &that, const void *value) override
    {
        if (value) {
            ListType list = (qobject_cast<Class *>(&that)->*listfn)();
            const auto &nodeList = *reinterpret_cast<const QSSGSceneDesc::NodeList *>(value);
            auto head = reinterpret_cast<typename TypeMap<T>::type **>(nodeList.head);
            for (int i = 0, end = nodeList.count; i != end; ++i)
                list.append(&list, qobject_cast<T *>((*(head + i))->obj));
            return true;
        }

        return false;
    }
};

template <typename T, typename Class>
struct PropertyGetter : PropertyCall
{
    using Getter = T (Class::*)() const;
    constexpr explicit PropertyGetter(Getter fn) : call(fn) {}
    Getter call = nullptr;
    bool get(const QQuick3DObject &, const void *[]) const override { return false; }
    bool set(QQuick3DObject &, const void *) override { return false; }
};

template <typename T, typename Class>
struct PropertyMember : PropertyCall
{
    using Getter = T (Class::*);
    constexpr explicit PropertyMember(Getter fn) : call(fn) {}
    Getter call = nullptr;
    bool get(const QQuick3DObject &, const void *[]) const override { return false; }
    bool set(QQuick3DObject &that, const void *value) override {
        if (value) {
            (qobject_cast<Class *>(&that)->*call) = (*reinterpret_cast<const T *>(value));
            return true;
        }

        return false;
    }
};

// Sets a property on a node, the property is a name map to struct containing a pointer to the value and a function
// to set the value on an runtime object (QtQuick3DObject). The type is verified at compile-time, so we can assume
// the value is of the right type when setting it at run-time.
template<typename Setter, typename Value, typename std::enable_if_t<std::is_same_v<typename FuncType<Setter>::Arg0Base, Value>, bool> = false>
static void setProperty(QSSGSceneDesc::Node &node, const char *name, Setter setter, const Value &value)
{
    Q_ASSERT(node.scene);
    static_assert((std::is_copy_constructible_v<Value> && std::is_trivially_destructible_v<Value>),
                  "Value needs to be copy constructable and trivially destructible!");
    Property *prop = node.scene->create<Property>();
    prop->name = name;
    prop->call = node.scene->create<decltype(PropertySetter(setter))>(setter);
    prop->value.mt = QMetaType::fromType<Value>();
    prop->value.dptr = node.scene->create<Value>(value);
    node.properties.push_back(*prop);
}

// Calling this will omit any type checking, so make sure the type is handled correctly
// when it gets used later!
template<typename Setter>
static void setProperty(QSSGSceneDesc::Node &node, const char *name, Setter setter, QSSGSceneDesc::Value &&value)
{
    Q_ASSERT(node.scene);
    Property *prop = node.scene->create<Property>();
    prop->name = name;
    prop->call = node.scene->create<decltype(PropertySetter(setter))>(setter);
    prop->value = value;
    node.properties.push_back(*prop);
}

template<typename Setter, typename ViewValue, typename std::enable_if_t<std::is_base_of_v<QSSGSceneDesc::BufferView, ViewValue>, bool> = false>
static void setProperty(QSSGSceneDesc::Node &node, const char *name, Setter setter, ViewValue view)
{
    Q_ASSERT(node.scene);
    static_assert(std::is_same_v<typename ViewValue::type, typename FuncType<Setter>::Arg0Base>, "Type cannot be mapped to slot argument");
    Property *prop = node.scene->create<Property>();
    prop->name = name;
    prop->call = node.scene->create<decltype(PropertySetter(setter))>(setter);
    prop->value.mt = QMetaType::fromType<ViewValue>();
    prop->value.dptr = node.scene->create<ViewValue>(std::move(view));
    node.properties.push_back(*prop);
}

template<typename Setter, typename Value, typename std::enable_if_t<std::is_same_v<typename FuncType<Setter>::Arg0, typename TypeMap<Value>::type *>, bool> = true>
static void setProperty(QSSGSceneDesc::Node &node, const char *name, Setter setter, Value *value)
{
    Q_ASSERT(node.scene);
    auto &scene = node.scene;
    Property *prop = scene->create<Property>();
    prop->name = name;
    prop->call = scene->create<decltype(PropertySetter(setter))>(setter);
    prop->value.mt = QMetaType::fromType<Node *>(); // Always 'Node', the Node class itself contains more fine grain type information.
    prop->value.dptr = value;
    node.properties.push_back(*prop);
}

// Overloaded function for setting a type to a property that's a QQmlListProperty.
template<typename Setter, typename NodeT, typename std::enable_if_t<std::is_same_v<typename FuncType<Setter>::Ret, QQmlListProperty<typename TypeMap<std::remove_pointer_t<NodeT>>::type>>, bool> = true>
static void setProperty(QSSGSceneDesc::Node &node, const char *name, Setter setter, const QVarLengthArray<NodeT> &list)
{
    Q_ASSERT(node.scene);
    if (!list.isEmpty()) {
        auto &scene = node.scene;
        NodeList *l = scene->create<NodeList>();
        {
            const auto size = sizeof(Node *) * list.count();
            l->head = reinterpret_cast<Node **>(scene->allocator.allocate(size));
            memcpy(l->head, list.data(), size);
            l->count = list.count();
        }

        Property *prop = scene->create<Property>();
        prop->name = name;
        prop->call = scene->create<decltype(PropertyList(setter))>(setter);
        prop->value.mt = QMetaType::fromType<QSSGSceneDesc::NodeList *>();
        prop->value.dptr = l;
        node.properties.push_back(*prop);
    }
}

} // QSSGSceneDesc

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QSSGSceneDesc::Node)
Q_DECLARE_METATYPE(QSSGSceneDesc::Texture)
Q_DECLARE_METATYPE(QSSGSceneDesc::Material)
Q_DECLARE_METATYPE(QSSGSceneDesc::Mesh)
Q_DECLARE_METATYPE(QSSGSceneDesc::Model)
Q_DECLARE_METATYPE(QSSGSceneDesc::Camera)
Q_DECLARE_METATYPE(QSSGSceneDesc::Light)
Q_DECLARE_METATYPE(QSSGSceneDesc::Skeleton)
Q_DECLARE_METATYPE(QSSGSceneDesc::Joint)
Q_DECLARE_METATYPE(QSSGSceneDesc::NodeList);

Q_DECLARE_METATYPE(QSSGSceneDesc::BufferView)
Q_DECLARE_METATYPE(QSSGSceneDesc::UrlView)
Q_DECLARE_METATYPE(QSSGSceneDesc::StringView)

#endif // QSSGSCENEDESCRIPTION_P_H
