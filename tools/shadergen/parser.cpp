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

#include "parser.h"

#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>

// Parsing
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>

#include <QtQuick3D/private/qquick3dobject_p.h>
#include <QtQuick3D/private/qquick3dviewport_p.h>
// Scene
#include <QtQuick3D/private/qquick3dsceneenvironment_p.h>
// Material(s)
#include <QtQuick3D/private/qquick3dprincipledmaterial_p.h>
// Lights
#include <QtQuick3D/private/qquick3darealight_p.h>
#include <QtQuick3D/private/qquick3dspotlight_p.h>
#include <QtQuick3D/private/qquick3dpointlight_p.h>
#include <QtQuick3D/private/qquick3ddirectionallight_p.h>

#include <QtQuick3DUtils/private/qssginvasivelinkedlist_p.h>

QT_BEGIN_NAMESPACE

template<typename T, T *T::*N = &T::next>
struct InvasiveListView : protected QSSGInvasiveSingleLinkedList<T, N>
{
    using QSSGInvasiveSingleLinkedList<T, N>::begin;
    using QSSGInvasiveSingleLinkedList<T, N>::end;
    using QSSGInvasiveSingleLinkedList<T, N>::m_head;

    explicit InvasiveListView(const T &obj) { m_head = &const_cast<T &>(obj); }
};

// name mapping.
namespace TypeInfo
{

enum Type
{
    View3D,
    SceneEnvironment,
    PrincipledMaterial,
    DirectionalLight,
    PointLight,
    AreaLight,
    SpotLight,
    Texture,
    Unknown
};

constexpr const char *typeStringTable[] {
    "View3D",
    "SceneEnvironment",
    "PrincipledMaterial",
    "DirectionalLight",
    "PointLight",
    "AreaLight",
    "SpotLight",
    "Texture",
};

template<typename T> constexpr Type getTypeId() { return Type::Unknown; }
template<> constexpr Type getTypeId<QQuick3DViewport>() { return Type::View3D; }
template<> constexpr Type getTypeId<QQuick3DSceneEnvironment>() { return Type::SceneEnvironment; }
template<> constexpr Type getTypeId<QQuick3DPrincipledMaterial>() { return Type::PrincipledMaterial; }
template<> constexpr Type getTypeId<QQuick3DDirectionalLight>() { return Type::DirectionalLight; }
template<> constexpr Type getTypeId<QQuick3DPointLight>() { return Type::PointLight; }
template<> constexpr Type getTypeId<QQuick3DAreaLight>() { return Type::AreaLight; }
template<> constexpr Type getTypeId<QQuick3DSpotLight>() { return Type::SpotLight; }
template<> constexpr Type getTypeId<QQuick3DTexture>() { return Type::Texture; }

}

using QmlTypeNames = QHash<QString, TypeInfo::Type>;
Q_GLOBAL_STATIC_WITH_ARGS(QmlTypeNames, s_typeMap, ({{TypeInfo::typeStringTable[TypeInfo::View3D], TypeInfo::View3D},
                                                     {TypeInfo::typeStringTable[TypeInfo::SceneEnvironment], TypeInfo::SceneEnvironment},
                                                     {TypeInfo::typeStringTable[TypeInfo::PrincipledMaterial], TypeInfo::PrincipledMaterial},
                                                     {TypeInfo::typeStringTable[TypeInfo::DirectionalLight], TypeInfo::DirectionalLight},
                                                     {TypeInfo::typeStringTable[TypeInfo::PointLight], TypeInfo::PointLight},
                                                     {TypeInfo::typeStringTable[TypeInfo::AreaLight], TypeInfo::AreaLight},
                                                     {TypeInfo::typeStringTable[TypeInfo::SpotLight], TypeInfo::SpotLight},
                                                     {TypeInfo::typeStringTable[TypeInfo::Texture], TypeInfo::Texture}
                                                    }))

struct Context
{
    enum class Type
    {
        Application,
        Component
    };

    struct Property {
        QObject *target = nullptr;
        QStringView name;
        TypeInfo::Type targetType = TypeInfo::Unknown;
    };

    template<typename T> using Vector = QVector<T>;
    using InterceptObjDefFunc = bool (*)(const QQmlJS::AST::UiObjectDefinition &, Context &, int &);
    using InterceptObjBinding = bool (*)(const QQmlJS::AST::UiObjectBinding &, Context &, int &);

    QQmlJS::Engine *engine = nullptr;
    QFileInfo currentFileInfo;

    MaterialParser::SceneData sceneData;
    Property property;
    QHash<QString, const QQuick3DViewport *> viewportComponents;
    QHash<QString, const QQuick3DSceneEnvironment *> sceneEnvComponents;
    QHash<QString, const QQuick3DPrincipledMaterial *> materialComponents;
    QHash<QString, const QQuick3DAbstractLight *> lightComponents;
    QHash<QString, const QQuick3DTexture *> textureComponents;
    InterceptObjDefFunc interceptODFunc = nullptr;
    InterceptObjBinding interceptOBFunc = nullptr;
    Type type = Type::Application;
    bool dbgprint = false;
};

static void cloneProperties(QObject &target, const QObject &source)
{
    Q_ASSERT(target.metaObject() == source.metaObject());
    const auto sourceMo = source.metaObject();
    auto targetMo = target.metaObject();
    const int propCount = sourceMo->propertyCount();
    for (int i = 0; i != propCount; ++i)
        targetMo->property(i).write(&target, sourceMo->property(i).read(&source));
}

template <typename T>
inline QVariant fromStringEnumHelper(const QStringView &ref, const QMetaProperty &property)
{
    bool ok = false;
    const int v = property.enumerator().keyToValue(ref.toLatin1(), &ok);
    return ok ? QVariant::fromValue(T(v)) : QVariant();
}

static QVariant fromString(const QStringView &ref, const Context::Property &p)
{
    if (!p.target)
        return QVariant();

    auto target = p.target;
    const auto &name = p.name;
    const auto &metaObject = target->metaObject();
    const int idx = metaObject->indexOfProperty(name.toLatin1());
    if (idx == -1)
        return  QVariant();

    const auto property = metaObject->property(idx);


    if (property.type() == QVariant::UserType) {
        const QMetaType metaType = property.metaType();
        switch (p.targetType) {
        case TypeInfo::PrincipledMaterial:
        {
            if (metaType.id() == qMetaTypeId<QQuick3DPrincipledMaterial::CullMode>())
                return fromStringEnumHelper<QQuick3DPrincipledMaterial::CullMode>(ref, property);
            if (metaType.id() == qMetaTypeId<QQuick3DPrincipledMaterial::TextureChannelMapping>())
                return fromStringEnumHelper<QQuick3DPrincipledMaterial::TextureChannelMapping>(ref, property);
            if (metaType.id() == qMetaTypeId<QQuick3DPrincipledMaterial::Lighting>())
                return fromStringEnumHelper<QQuick3DPrincipledMaterial::Lighting>(ref, property);
            if (metaType.id() == qMetaTypeId<QQuick3DPrincipledMaterial::BlendMode>())
                return fromStringEnumHelper<QQuick3DPrincipledMaterial::BlendMode>(ref, property);
            if (metaType.id() == qMetaTypeId<QQuick3DPrincipledMaterial::AlphaMode>())
                return fromStringEnumHelper<QQuick3DPrincipledMaterial::AlphaMode>(ref, property);
        }
            break;
        case TypeInfo::AreaLight:
        case TypeInfo::SpotLight:
        case TypeInfo::PointLight:
            if (metaType.id() == qMetaTypeId<QQuick3DAbstractLight::QSSGShadowMapQuality>())
                return fromStringEnumHelper<QQuick3DAbstractLight::QSSGShadowMapQuality>(ref, property);
            break;
        case TypeInfo::SceneEnvironment:
            Q_FALLTHROUGH();
        default:
            break;
        }
    } else {
        switch (property.type()) {
        case QVariant::Color:
            return QColor(ref); // Not really needed
        case QVariant::Url:
            return QUrl::fromLocalFile(ref.toString());
        default:
            break;
        }

        switch (property.metaType().id()) {
        case QMetaType::Float:
        {
            bool ok = false;
            const float ret = ref.toFloat(&ok);
            if (!ok) // TODO: The source data might be a ref. to the actual value, we don't handle that now.
                break;

            return ok ? QVariant::fromValue(ret) : QVariant();
        }
        default:
            break;
        }

    }

    printf("Unhandled type %d for property %s\n", property.type(), ref.toLatin1().constData());

    return QVariant();
}

static QString getQmlFileExtension() { return QStringLiteral("qml"); }


struct Visitors
{
    static void visit(const QQmlJS::AST::UiProgram &program, Context &ctx, int &ret)
    {
        using namespace QQmlJS::AST;
        const bool readHeaders = false;
        if (readHeaders && program.headers) { // No real need for us to look at the includes
            using HeaderItem = UiHeaderItemList;
            using Headers = InvasiveListView<HeaderItem>;

            Headers headers(*program.headers);
            for (const auto &header : headers)
                printf("Type: %d\n", header.kind);
        }
        if (program.members)
            visit(*program.members, ctx, ret);
    }
    static void visit(const QQmlJS::AST::UiObjectInitializer &objInitializer, Context &ctx, int &ret)
    {
        using namespace QQmlJS::AST;
        if (objInitializer.members)
            visit(*objInitializer.members, ctx, ret);
    }

    static void visit(const QQmlJS::AST::UiObjectDefinition &def, Context &ctx, int &ret)
    {
        using namespace QQmlJS::AST;
        if (ctx.dbgprint)
            printf("Object -> %s\n", def.qualifiedTypeNameId->name.toLocal8Bit().constData());
        if (!(ctx.interceptODFunc && ctx.interceptODFunc(def, ctx, ret))) {
            if (def.initializer)
                visit(*static_cast<UiObjectInitializer *>(def.initializer), ctx, ret);
        }
    }

    static void visit(const QQmlJS::AST::UiArrayMemberList &memberList, Context &ctx, int &ret)
    {
        using namespace QQmlJS::AST;
        using ArrayMemberItem = UiArrayMemberList;
        using ArrayMembers = InvasiveListView<ArrayMemberItem>;

        ArrayMembers arrayMembers(memberList);
        for (auto &object : arrayMembers) {
            if (object.member->kind == Node::Kind_UiObjectDefinition) {
                const auto &def = *static_cast<UiObjectDefinition *>(object.member);
                visit(def, ctx, ret);
            }
        }
    }

    static void visit(const QQmlJS::AST::UiScriptBinding &binding, Context &ctx, int &ret)
    {
        using namespace QQmlJS::AST;
        if (ctx.dbgprint)
            printf("property -> %s: ", binding.qualifiedId->name.toLocal8Bit().constData());

        const auto oldName = ctx.property.name; // reentrancy
        ctx.property.name = binding.qualifiedId->name;

        if (binding.statement) {
            if (binding.statement->kind == Node::Kind_ExpressionStatement) {
                const auto &expressionStatement = static_cast<const ExpressionStatement &>(*binding.statement);
                visit(expressionStatement, ctx, ret);
            }
        }
        ctx.property.name = oldName;
    }

    static void visit(const QQmlJS::AST::UiArrayBinding &arrayBinding, Context &ctx, int &ret)
    {
        using namespace QQmlJS::AST;
        if (ctx.dbgprint)
            printf("property -> %s: [\n", arrayBinding.qualifiedId->name.toLocal8Bit().constData());

        if (arrayBinding.members)
            visit(*arrayBinding.members, ctx, ret);

        if (ctx.dbgprint)
            printf("]\n");
    }

    static void visit(const QQmlJS::AST::UiObjectBinding &objectBinding, Context &ctx, int &ret)
    {
        using namespace QQmlJS::AST;

        if (ctx.dbgprint)
            printf("Property -> %s: %s {\n", objectBinding.qualifiedId->name.toLocal8Bit().constData(), objectBinding.qualifiedTypeNameId->name.toLocal8Bit().constData());

        if (objectBinding.initializer) {
            if (!(ctx.interceptOBFunc && ctx.interceptOBFunc(objectBinding, ctx, ret)))
                visit(*objectBinding.initializer, ctx, ret);
        }

        if (ctx.dbgprint)
            printf("}\n");
    }

    static void visit(const QQmlJS::AST::ExpressionStatement &exprStatement, Context &ctx, int &ret)
    {
        using namespace QQmlJS::AST;
        if (exprStatement.expression) {
            if (exprStatement.expression->kind == Node::Kind_IdentifierExpression) {
                const auto &identExpression = static_cast<const IdentifierExpression &>(*exprStatement.expression);
                visit(identExpression, ctx, ret);
            } else if (exprStatement.expression->kind == Node::Kind_StringLiteral) {
                const auto &stringLiteral = static_cast<const StringLiteral &>(*exprStatement.expression);
                visit(stringLiteral, ctx, ret);
            } else if (exprStatement.expression->kind == Node::Kind_NumericLiteral) {
                const auto &numericLiteral = static_cast<const NumericLiteral &>(*exprStatement.expression);
                visit(numericLiteral, ctx, ret);
            } else if (exprStatement.expression->kind == Node::Kind_FieldMemberExpression) {
                const auto &fieldMemberExpression = static_cast<const FieldMemberExpression &>(*exprStatement.expression);
                visit(fieldMemberExpression, ctx, ret);
            } else if (exprStatement.expression->kind == Node::Kind_TrueLiteral || exprStatement.expression->kind == Node::Kind_FalseLiteral) {
                const bool v = (exprStatement.expression->kind == Node::Kind_TrueLiteral);
                if (ctx.dbgprint)
                    printf("%s\n", v ? "true" : "false");
                if (ctx.property.target) {
                    auto target = ctx.property.target;
                    const auto &name = ctx.property.name;
                    if (target->metaObject()->indexOfProperty(name.toLatin1()) != -1)
                        target->setProperty(name.toLatin1(), QVariant::fromValue(v));
                }
            } else {
                if (ctx.dbgprint)
                    printf("<expression>\n");
            }
        }
    }

    static void visit(const QQmlJS::AST::IdentifierExpression &idExpr, Context &ctx, int &ret)
    {
        Q_UNUSED(ret);
        if (ctx.dbgprint)
            printf("%s\n", idExpr.name.toLocal8Bit().constData());
    }

    static void visit(const QQmlJS::AST::StringLiteral &stringLiteral, Context &ctx, int &ret)
    {
        Q_UNUSED(ret);
        if (ctx.dbgprint)
            printf("\"%s\"\n", stringLiteral.value.toLocal8Bit().constData());

        if (ctx.property.target) {
            const auto &name = ctx.property.name;
            const auto v = fromString(stringLiteral.value, ctx.property);
            if (v.isValid())
                ctx.property.target->setProperty(name.toLatin1(), v);
        }
    }

    static void visit(const QQmlJS::AST::NumericLiteral &numericLiteral, Context &ctx, int &ret)
    {
        Q_UNUSED(ret);
        if (ctx.dbgprint)
            printf("%f\n", numericLiteral.value);

        if (ctx.property.target) {
            auto target = ctx.property.target;
            const auto &name = ctx.property.name;
            if (ctx.property.target->metaObject()->indexOfProperty(name.toLatin1()) != -1)
                target->setProperty(name.toLatin1(), QVariant::fromValue(numericLiteral.value));
        }
    }

    static void visit(const QQmlJS::AST::FieldMemberExpression &fieldMemberExpression, Context &ctx, int &ret)
    {
        Q_UNUSED(ret);
        if (ctx.dbgprint)
            printf("%s\n", fieldMemberExpression.name.toLocal8Bit().constData());

        if (ctx.property.target) {
            const auto &name = ctx.property.name;
            const auto v = fromString(fieldMemberExpression.name, ctx.property);
            if (v.isValid())
                ctx.property.target->setProperty(name.toLatin1(), v);
        }
    }

    static void visit(const QQmlJS::AST::UiObjectMemberList &memberList, Context &ctx, int &ret)
    {
        using namespace QQmlJS::AST;
        using ObjectMemberItem = UiObjectMemberList;
        using ObjectMembers = InvasiveListView<ObjectMemberItem>;

        ObjectMembers objectMembers(memberList);
        for (const auto &member : objectMembers) {
            if (member.member) {
                if (member.member->kind == Node::Kind_UiScriptBinding) {
                    const auto &scriptBinding = static_cast<const UiScriptBinding &>(*member.member);
                    visit(scriptBinding, ctx, ret);
                } else if (member.member->kind == Node::Kind_UiArrayBinding) {
                    const auto &arrayBinding = static_cast<const UiArrayBinding &>(*member.member);
                    visit(arrayBinding, ctx, ret);
                } else if (member.member->kind == Node::Kind_UiObjectDefinition) {
                    const auto &objectDef = static_cast<const UiObjectDefinition &>(*member.member);
                    visit(objectDef, ctx, ret);
                } else if (member.member->kind == Node::Kind_UiObjectBinding) {
                    const auto &objBinding = static_cast<const UiObjectBinding &>(*member.member);
                    visit(objBinding, ctx, ret);
                } else {
                    if (ctx.dbgprint)
                        printf("<member %d>\n", member.member->kind);
                }
            }
        }
    }

private:
    Visitors() = delete;
    Q_DISABLE_COPY(Visitors);
};

template <typename O, typename T>
T *buildType(const O &obj, Context &ctx, int &ret, const T *base = nullptr)
{
    // swap -> reentrancy
    Context::Property property;
    qSwap(property, ctx.property);
    Q_ASSERT(ctx.property.target == nullptr);

    T *instance = nullptr;

    if (ctx.dbgprint)
        printf("Building %s!\n", TypeInfo::typeStringTable[TypeInfo::getTypeId<T>()]);

    if (obj.initializer && obj.initializer->members) {
        instance = new T;
        if (base)
            cloneProperties(*instance, *base);

        if (obj.initializer) {
            ctx.property.target = instance;
            ctx.property.targetType = TypeInfo::getTypeId<T>();
            Visitors::visit(*obj.initializer, ctx, ret);
        }
    }

    // swap back
    qSwap(property, ctx.property);

    return instance;
}

static QQuick3DAbstractLight *buildLight(const QQmlJS::AST::UiObjectDefinition &def,
                                         Context &ctx,
                                         int &ret,
                                         TypeInfo::Type lightType,
                                         const QQuick3DAbstractLight *base = nullptr)
{
    switch (lightType) {
    case TypeInfo::DirectionalLight:
        return buildType(def, ctx, ret, qobject_cast<const QQuick3DDirectionalLight *>(base));
    case TypeInfo::PointLight:
        return buildType(def, ctx, ret, qobject_cast<const QQuick3DPointLight *>(base));
    case TypeInfo::AreaLight:
        return buildType(def, ctx, ret, qobject_cast<const QQuick3DAreaLight *>(base));
    case TypeInfo::SpotLight:
        return buildType(def, ctx, ret, qobject_cast<const QQuick3DSpotLight *>(base));
    default:
        break;
    }
    return nullptr;
}

static bool interceptObjectBinding(const QQmlJS::AST::UiObjectBinding &objectBinding, Context &ctx, int &ret)
{
    if (ctx.dbgprint)
        printf("Intercepted object binding!\n");

    bool handled = false;

    const auto &typeName = objectBinding.qualifiedTypeNameId->name.toString();
    const auto &propName = objectBinding.qualifiedId->name;

    const auto typeIt = s_typeMap->constFind(typeName);
    if (typeIt != s_typeMap->constEnd()) {
        const auto type = *typeIt;
        if (ctx.dbgprint)
            printf("Resolving: %s -> \'%s\'\n", qPrintable(typeIt.key()), TypeInfo::typeStringTable[type]);

        switch (type) {
        case TypeInfo::SceneEnvironment:
        {
            auto &components = ctx.sceneEnvComponents;
            const auto compIt = components.constFind(typeName);
            const QQuick3DSceneEnvironment *base = (compIt != components.cend()) ? *compIt : nullptr;
            if (QQuick3DSceneEnvironment *env = buildType(objectBinding, ctx, ret, base)) {
                if (ctx.property.target) {
                    if (ctx.dbgprint)
                        printf("Updating property %s on %s\n", propName.toLatin1().constData(), TypeInfo::typeStringTable[ctx.property.targetType]);
                    const auto &target = ctx.property.target;
                    const int idx = target->metaObject()->indexOfProperty(propName.toLatin1().constData());
                    if (idx != -1)
                        target->setProperty(propName.toLatin1().constData(), QVariant::fromValue(env));
                    else
                        qWarning("Property %s not found on %s", propName.toLatin1().constData(), TypeInfo::typeStringTable[ctx.property.targetType]);
                }
            }
            handled = true;
            break;
        }
        case TypeInfo::Texture:
        {
            auto &components = ctx.textureComponents;
            const auto compIt = components.constFind(typeName);
            const QQuick3DTexture *base = (compIt != components.cend()) ? *compIt : nullptr;
            if (QQuick3DTexture *tex = buildType(objectBinding, ctx, ret, base)) {
                if (ctx.property.target) {
                    if (ctx.dbgprint)
                        printf("Updating property %s on %s\n", propName.toLatin1().constData(), TypeInfo::typeStringTable[ctx.property.targetType]);
                    const auto &target = ctx.property.target;
                    const int idx = target->metaObject()->indexOfProperty(propName.toLatin1().constData());
                    if (idx != -1)
                        target->setProperty(propName.toLatin1().constData(), QVariant::fromValue(tex));
                }
                ctx.sceneData.textures.append(tex);
            }
            handled = true;
            break;
        }
        default:
            printf("Unhandled type\n");
            break;
        }
    }

    return handled;
}

static bool interceptObjectDef(const QQmlJS::AST::UiObjectDefinition &def, Context &ctx, int &ret)
{
    if (ctx.dbgprint)
        printf("Intercepted object definition!\n");

    const auto &typeName = def.qualifiedTypeNameId->name.toString();

    // known type?
    const auto typeIt = s_typeMap->constFind(typeName);
    if (typeIt != s_typeMap->constEnd()) {
        const auto type = *typeIt;
        // If this is a component of a known type register the component name
        if (ctx.type == Context::Type::Component) {
            const auto &fileName = ctx.currentFileInfo.fileName();
            const auto componentName = fileName.leftRef(fileName.length() - 4);
            s_typeMap->insert(componentName.toString(), typeIt.value());
        } else if (ctx.dbgprint) {
            printf("Resolving: %s -> \'%s\'\n", qPrintable(typeIt.key()), TypeInfo::typeStringTable[type]);
        }

        switch (type) {
        case TypeInfo::View3D:
        {
            auto &components = ctx.viewportComponents;
            const auto compIt = components.constFind(typeName);
            const QQuick3DViewport *base = (compIt != components.cend()) ? *compIt : nullptr;
            if (QQuick3DViewport *viewport = buildType(def, ctx, ret, base)) {
                // If this is a component we'll store it for look-ups later.
                if (!base) {
                    const auto &fileName = ctx.currentFileInfo.fileName();
                    const auto componentName = fileName.leftRef(fileName.length() - 4);
                    components.insert(componentName.toString(), viewport);
                }
                // Only one viewport supported atm (see SceneEnvironment case as well).
                if (!ctx.sceneData.viewport)
                    ctx.sceneData.viewport = viewport;
            }
            break;
        }
        case TypeInfo::SceneEnvironment:
        {
            auto &components = ctx.sceneEnvComponents;
            const auto compIt = components.constFind(typeName);
            const QQuick3DSceneEnvironment *base = (compIt != components.cend()) ? *compIt : nullptr;
            if (QQuick3DSceneEnvironment *sceneEnv = buildType(def, ctx, ret, base)) {
                // If this is a component we'll store it for look-ups later.
                if (!base) {
                    const auto &fileName = ctx.currentFileInfo.fileName();
                    const auto componentName = fileName.leftRef(fileName.length() - 4);
                    components.insert(componentName.toString(), sceneEnv);
                }

                if (ctx.sceneData.viewport)
                    ctx.sceneData.viewport->setEnvironment(sceneEnv);
            }
            break;
        }
        case TypeInfo::PrincipledMaterial:
        {
            auto &components = ctx.materialComponents;
            const auto compIt = components.constFind(typeName);
            const QQuick3DPrincipledMaterial *base = (compIt != components.cend()) ? *compIt : nullptr;
            if (QQuick3DPrincipledMaterial *mat = buildType(def, ctx, ret, base)) {
                // If this is a component we'll store it for look-ups later.
                if (!base) {
                    const auto &fileName = ctx.currentFileInfo.fileName();
                    const auto componentName = fileName.leftRef(fileName.length() - 4);
                    components.insert(componentName.toString(), mat);
                }
                ctx.sceneData.materials.push_back(mat);
            }
            break;
        }
        case TypeInfo::DirectionalLight:
            Q_FALLTHROUGH();
        case TypeInfo::PointLight:
            Q_FALLTHROUGH();
        case TypeInfo::AreaLight:
            Q_FALLTHROUGH();
        case TypeInfo::SpotLight:
        {
            auto &components = ctx.lightComponents;
            const auto compIt = components.constFind(typeName);
            const QQuick3DAbstractLight *base = (compIt != components.cend()) ? *compIt : nullptr;
            if (QQuick3DAbstractLight *light = buildLight(def, ctx, ret, type, base)) {
                // If this is a component we'll store it for look-ups later.
                if (!base) {
                    const auto &fileName = ctx.currentFileInfo.fileName();
                    const auto componentName = fileName.leftRef(fileName.length() - 4);
                    components.insert(componentName.toString(), light);
                }
                if (type == TypeInfo::DirectionalLight)
                    ctx.sceneData.directionalLights.push_back(qobject_cast<QQuick3DDirectionalLight *>(light));
                else if (type == TypeInfo::PointLight)
                    ctx.sceneData.pointLights.push_back(qobject_cast<QQuick3DPointLight *>(light));
                else if (type == TypeInfo::AreaLight)
                    ctx.sceneData.areaLights.push_back(qobject_cast<QQuick3DAreaLight *>(light));
                else if (type == TypeInfo::SpotLight)
                    ctx.sceneData.spotLights.push_back(qobject_cast<QQuick3DSpotLight *>(light));
            }
            break;
        }
        case TypeInfo::Texture:
        {
            auto &components = ctx.textureComponents;
            const auto compIt = components.constFind(typeName);
            const QQuick3DTexture *base = (compIt != components.cend()) ? *compIt : nullptr;
            if (QQuick3DTexture *tex = buildType(def, ctx, ret, base)) {
                // If this is a component we'll store it for look-ups later.
                if (!base) {
                    const auto &fileName = ctx.currentFileInfo.fileName();
                    const auto componentName = fileName.leftRef(fileName.length() - 4);
                    components.insert(componentName.toString(), tex);
                }
                ctx.sceneData.textures.push_back(tex);
            }
            break;
        }
        default:
            Q_UNREACHABLE();
            break;
        }
        return true;
    }

    return false;
}

static int parseQmlData(const QByteArray &code, Context &ctx)
{
    Q_ASSERT(ctx.engine && ctx.engine->lexer());
    if (ctx.dbgprint)
        printf("Parsing %s\n", qPrintable(ctx.currentFileInfo.filePath()));
    int ret = 0;
    ctx.engine->lexer()->setCode(QString::fromUtf8(code), 1, true);
    QQmlJS::Parser parser(ctx.engine);
    const bool ok = parser.parse();
    if (ok) {
        const auto program = parser.ast();
        if (program)
            Visitors::visit(*program, ctx, ret);
    } else {
        ret = -1;
        qWarning("Parsing failed due to %s in %s:%d%d", qPrintable(parser.errorMessage()), qPrintable(ctx.currentFileInfo.fileName()), parser.errorLineNumber(), parser.errorColumnNumber());
    }

    return ret;
}

int MaterialParser::parseQmlFiles(const QVector<QStringRef> &filePaths, const QDir &sourceDir, SceneData &sceneData, bool verboseOutput)
{
    int ret = 0;

    if (filePaths.isEmpty()) {
        qWarning("No input files");
        return ret;
    }

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);

    Context ctx;
    ctx.dbgprint = verboseOutput;
    ctx.engine = &engine;
    ctx.interceptODFunc = &interceptObjectDef;
    ctx.interceptOBFunc = &interceptObjectBinding;

    QVector<QString> deferredOther;
    QVector<QString> deferredComponets;

    const QString sourcePath = sourceDir.canonicalPath() + QDir::separator();

    const bool isMultifile = filePaths.size() != 1;

    // Go through and find the material components first
    for (const auto &v : filePaths) {
        QFileInfo &currentFileInfo = ctx.currentFileInfo;
        currentFileInfo.setFile(sourcePath + v);
        const bool maybeComponent = currentFileInfo.fileName().at(0).isUpper();
        if (currentFileInfo.isFile() && currentFileInfo.suffix() == getQmlFileExtension()) {
            const QString filePath = currentFileInfo.canonicalFilePath();
            if (isMultifile && maybeComponent) {
                QFile f(filePath);
                if (!f.open(QFile::ReadOnly)) {
                    qWarning("Could not open file %s for reading!", qPrintable(filePath));
                    return -1;
                }

                const QByteArray code = f.readAll();
                int idx = code.indexOf('{');
                if (idx != -1) {
                    const QByteArray section = code.mid(0, idx);
                    idx = section.indexOf(TypeInfo::typeStringTable[TypeInfo::PrincipledMaterial]);
                    if (idx != -1) {
                        ctx.type = Context::Type::Component;
                        ret = parseQmlData(code, ctx);
                        if (ret != 0)
                            break;
                    } else {
                        deferredComponets.push_back(filePath);
                    }
                } else {
                    qWarning("No items found in %s\n", qPrintable(filePath));
                }
            } else {
                deferredOther.push_back(filePath);
            }
        } else {
            qWarning("The file %s is either not a file or has the wrong extension!", qPrintable(v.toString()));
        }
    }

    const auto parsePaths = [&ctx, &ret](const QVector<QString> &paths, Context::Type type) {
        ctx.type = type;
        for (const auto &path : paths) {
            QFileInfo &currentFileInfo = ctx.currentFileInfo;
            currentFileInfo.setFile(path);
            if (currentFileInfo.isFile() && currentFileInfo.suffix() == getQmlFileExtension()) {
                const QString filePath = currentFileInfo.canonicalFilePath();
                QFile f(filePath);
                if (!f.open(QFile::ReadOnly)) {
                    qWarning("Could not open file %s for reading!", qPrintable(filePath));
                    ret =  -1;
                    return;
                }

                const QByteArray code = f.readAll();
                ret = parseQmlData(code, ctx);
                if (ret != 0)
                    break;
            }
        }
    };

    // Other components
    parsePaths(deferredComponets, Context::Type::Component);

    // Now parse the rest
    parsePaths(deferredOther, Context::Type::Application);

    sceneData = std::move(ctx.sceneData);

    return ret;
}

QT_END_NAMESPACE
