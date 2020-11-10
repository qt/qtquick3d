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
#include <QtCore/qstringview.h>

#include <QtQml/qqmllist.h>

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
#include <QtQuick3D/private/qquick3ddefaultmaterial_p.h>
#include <QtQuick3D/private/qquick3dcustommaterial_p.h>
// Lights
#include <QtQuick3D/private/qquick3dspotlight_p.h>
#include <QtQuick3D/private/qquick3dpointlight_p.h>
#include <QtQuick3D/private/qquick3ddirectionallight_p.h>

#include <QtQuick3D/private/qquick3dshaderutils_p.h>

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

constexpr const char *typeStringTable[] {
    "View3D",
    "SceneEnvironment",
    "PrincipledMaterial",
    "DefaultMaterial",
    "CustomMaterial",
    "DirectionalLight",
    "PointLight",
    "SpotLight",
    "Texture",
    "TextureInput",
    "Model"
};

template<typename T> constexpr QmlType getTypeId() { return QmlType::Unknown; }
template<> constexpr QmlType getTypeId<QQuick3DViewport>() { return QmlType::View3D; }
template<> constexpr QmlType getTypeId<QQuick3DSceneEnvironment>() { return QmlType::SceneEnvironment; }
template<> constexpr QmlType getTypeId<QQuick3DPrincipledMaterial>() { return QmlType::PrincipledMaterial; }
template<> constexpr QmlType getTypeId<QQuick3DDefaultMaterial>() { return QmlType::DefaultMaterial; }
template<> constexpr QmlType getTypeId<QQuick3DCustomMaterial>() { return QmlType::CustomMaterial; }
template<> constexpr QmlType getTypeId<QQuick3DDirectionalLight>() { return QmlType::DirectionalLight; }
template<> constexpr QmlType getTypeId<QQuick3DPointLight>() { return QmlType::PointLight; }
template<> constexpr QmlType getTypeId<QQuick3DSpotLight>() { return QmlType::SpotLight; }
template<> constexpr QmlType getTypeId<QQuick3DTexture>() { return QmlType::Texture; }
template<> constexpr QmlType getTypeId<QQuick3DShaderUtilsTextureInput>() { return QmlType::TextureInput; }
template<> constexpr QmlType getTypeId<QQuick3DModel>() { return QmlType::Model; }

}

using QmlTypeNames = QHash<QString, TypeInfo::QmlType>;
Q_GLOBAL_STATIC_WITH_ARGS(QmlTypeNames, s_typeMap, ({{TypeInfo::typeStringTable[TypeInfo::View3D], TypeInfo::View3D},
                                                     {TypeInfo::typeStringTable[TypeInfo::SceneEnvironment], TypeInfo::SceneEnvironment},
                                                     {TypeInfo::typeStringTable[TypeInfo::PrincipledMaterial], TypeInfo::PrincipledMaterial},
                                                     {TypeInfo::typeStringTable[TypeInfo::DefaultMaterial], TypeInfo::DefaultMaterial},
                                                     {TypeInfo::typeStringTable[TypeInfo::CustomMaterial], TypeInfo::CustomMaterial},
                                                     {TypeInfo::typeStringTable[TypeInfo::DirectionalLight], TypeInfo::DirectionalLight},
                                                     {TypeInfo::typeStringTable[TypeInfo::PointLight], TypeInfo::PointLight},
                                                     {TypeInfo::typeStringTable[TypeInfo::SpotLight], TypeInfo::SpotLight},
                                                     {TypeInfo::typeStringTable[TypeInfo::Texture], TypeInfo::Texture},
                                                     {TypeInfo::typeStringTable[TypeInfo::TextureInput], TypeInfo::TextureInput},
                                                     {TypeInfo::typeStringTable[TypeInfo::Model], TypeInfo::Model}
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
        TypeInfo::QmlType targetType = TypeInfo::Unknown;
        TypeInfo::QmlType propertyType = TypeInfo::Unknown;
        TypeInfo::BuiltinType builtinType = TypeInfo::InvalidBuiltin;
        bool isMember = false;
    };

    template<typename T> using Vector = QVector<T>;
    using InterceptObjDefFunc = bool (*)(const QQmlJS::AST::UiObjectDefinition &, Context &, int &);
    using InterceptObjBinding = bool (*)(const QQmlJS::AST::UiObjectBinding &, Context &, int &);
    using InterceptPublicMember = bool (*)(const QQmlJS::AST::UiPublicMember &, Context &, int &);
    using InterceptCallExpression = bool (*)(const QQmlJS::AST::CallExpression &, Context &, int &);

    QQmlJS::Engine *engine = nullptr;
    QDir workingDir; // aka source directory
    QFileInfo currentFileInfo;

    MaterialParser::SceneData sceneData;
    Property property;

    struct Component
    {
        QObject *ptr = nullptr;
        TypeInfo::QmlType type = TypeInfo::QmlType::Unknown;
    };

    QHash<QStringView, QObject *> identifierMap;
    QHash<QString, Component> components;
    InterceptObjDefFunc interceptODFunc = nullptr;
    InterceptObjBinding interceptOBFunc = nullptr;
    InterceptPublicMember interceptPMFunc = nullptr;
    InterceptCallExpression interceptCallExpr = nullptr;
    Type type = Type::Application;
    bool dbgprint = false;
};

Q_DECLARE_TYPEINFO(Context::Component, Q_PRIMITIVE_TYPE);

namespace BuiltinHelpers {

using ArgumentListView = InvasiveListView<QQmlJS::AST::ArgumentList>;

static constexpr quint8 componentCount(const QVector2D &) { return 2; }
static constexpr quint8 componentCount(const QVector3D &) { return 3; }
static constexpr quint8 componentCount(const QVector4D &) { return 4; }

static double expressionValue(const QQmlJS::AST::ExpressionNode &expr) {
    using namespace QQmlJS::AST;

    if (expr.kind == Node::Kind_NumericLiteral) {
        return static_cast<const NumericLiteral &>(expr).value;
    } else if (expr.kind == Node::Kind_UnaryMinusExpression) {
        const auto &minusExpr = static_cast<const UnaryMinusExpression &>(expr);
        if (minusExpr.expression && minusExpr.expression->kind == Node::Kind_NumericLiteral)
            return static_cast<const NumericLiteral &>(*minusExpr.expression).value * -1.0;
    } else if (expr.kind == Node::Kind_UnaryPlusExpression) {
        const auto &plusExpr = static_cast<const UnaryPlusExpression &>(expr);
        if (plusExpr.expression && plusExpr.expression->kind == Node::Kind_NumericLiteral)
            return static_cast<const NumericLiteral &>(*plusExpr.expression).value;
    }

    return 0.0;
}

template <typename T>
static inline bool setProperty(const Context::Property &property, const T &v)
{
    return property.target->setProperty(property.name.toLatin1(), QVariant::fromValue(v));
}

template<typename Vec>
static Vec toVec(const ArgumentListView &list, bool *ok = nullptr)
{
    using namespace QQmlJS::AST;
    int i = 0;
    Vec vec;
    for (const auto &listItem : list) {
        if (listItem.expression)
            vec[i++] = expressionValue(*listItem.expression);
    }

    if (ok)
        *ok = (i == componentCount(vec));

    return vec;
}

static QPointF toPoint(const ArgumentListView &list, bool *ok = nullptr)
{
    using namespace QQmlJS::AST;
    int i = 0;
    qreal args[2];
    for (const auto &listItem : list) {
        if (listItem.expression)
            args[i++] = expressionValue(*listItem.expression);
    }

    if (ok)
        *ok = (i == 2);

    return QPointF(args[0], args[1]);
}

static QSizeF toSize(const ArgumentListView &list, bool *ok = nullptr)
{
    using namespace QQmlJS::AST;
    int i = 0;
    qreal args[2];
    for (const auto &listItem : list) {
        if (listItem.expression && listItem.expression->kind == Node::Kind_NumericLiteral)
            args[i++] = expressionValue(*listItem.expression);
    }

    if (ok)
        *ok = (i == 2);

    return QSizeF(args[0], args[1]);
}

static QRectF toRect(const ArgumentListView &list, bool *ok = nullptr)
{
    using namespace QQmlJS::AST;
    int i = 0;
    qreal args[4];
    for (const auto &listItem : list) {
        if (listItem.expression)
            args[i++] = expressionValue(*listItem.expression);
    }

    if (ok)
        *ok = (i == 4);

    return QRectF(args[0], args[1], args[2], args[3]);
}

static QMatrix4x4 toMat44(const ArgumentListView &list, bool *ok = nullptr)
{
    using namespace QQmlJS::AST;
    int i = 0;
    float args[16];
    for (const auto &listItem : list) {
        if (listItem.expression)
            args[i++] = expressionValue(*listItem.expression);
    }

    if (ok)
        *ok = (i == 16);

    return QMatrix4x4(args);
}

static QQuaternion toQuaternion(const ArgumentListView &list, bool *ok = nullptr)
{
    using namespace QQmlJS::AST;
    int i = 0;
    float args[4];
    for (const auto &listItem : list) {
        if (listItem.expression && listItem.expression->kind == Node::Kind_NumericLiteral)
            args[i++] = expressionValue(*listItem.expression);
    }

    if (ok)
        *ok = (i == 4);

    return QQuaternion(args[0], args[1], args[2], args[3]);
}

// String variants
// Note: Unlike the call variants we assume arguments are correct (can be converted),
// as they should have failed earlier, during parsing, if they were not.
template <typename Vec>
static Vec toVec(const QStringView &ref)
{
    const auto args = ref.split(u',');
    Vec vec;
    int i = 0;
    bool ok = false;
    if (args.size() == componentCount(vec)) {
        for (const auto &arg : args) {
            vec[++i] = arg.toDouble(&ok);
            Q_ASSERT(ok);
        }
    }

    return vec;
}

static QPointF toPoint(const QStringView &ref)
{
    const auto args = ref.split(u",");
    if (args.size() == 2) {
        bool ok = false;
        const auto arg0 = args.at(0).toDouble(&ok);
        Q_ASSERT(ok);
        const auto arg1 = args.at(1).toDouble(&ok);
        Q_ASSERT(ok);
        return QPointF(arg0, arg1);
    }
    return QPointF();
}

static QSizeF toSize(const QStringView &ref)
{
    const auto args = ref.split(u'x');
    if (args.size() == 2) {
        bool ok = false;
        const auto arg0 = args.at(0).toDouble(&ok);
        Q_ASSERT(ok);
        const auto arg1 = args.at(1).toDouble(&ok);
        Q_ASSERT(ok);
        return QSizeF(arg0, arg1);
    }
    return QSizeF();
}

static QRectF toRect(const QStringView &ref)
{
    auto args = ref.split(u",");
    if (args.size() == 3) {
        bool ok = false;
        const auto arg0 = args.at(0).toDouble(&ok);
        Q_ASSERT(ok);
        const auto arg1 = args.at(1).toDouble(&ok);
        Q_ASSERT(ok);
        args = args.at(2).split(u'x');
        if (args.size() == 2) {
            const auto arg2 = args.at(0).toDouble(&ok);
            Q_ASSERT(ok);
            const auto arg3 = args.at(1).toDouble(&ok);
            Q_ASSERT(ok);
            return QRectF(arg0, arg1, arg2, arg3);
        }
    }
    return QRectF();
}

static QQuaternion toQuaternion(const QStringView &ref)
{
    const auto args = ref.split(u',');
    if (args.size() == 4) {
        bool ok = false;
        const auto arg0 = args.at(0).toDouble(&ok);
        Q_ASSERT(ok);
        const auto arg1 = args.at(1).toDouble(&ok);
        Q_ASSERT(ok);
        const auto arg2 = args.at(2).toDouble(&ok);
        Q_ASSERT(ok);
        const auto arg3 = args.at(3).toDouble(&ok);
        Q_ASSERT(ok);
        return QQuaternion(arg0, arg1, arg2, arg3);
    }
    return QQuaternion();
}

} // BuiltinHelpers

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

static QVariant fromString(const QStringView &ref, const Context &ctx)
{
    const auto &p = ctx.property;
    if (!p.target)
        return QVariant();

    static const auto toBuiltinType = [](int type, const QStringView &ref, const QDir &workingDir) {
        using namespace BuiltinHelpers;
        bool ok = false;
        switch (type) {
        case TypeInfo::Var:
            break;
        case TypeInfo::Int:
        {
            const auto v = ref.toInt(&ok);
            return (ok ? QVariant::fromValue(v) : QVariant());
        }
            break;
        case TypeInfo::Bool:
        {
            const auto v = ref.toInt(&ok);
            return (ok ? QVariant::fromValue(bool(v)) : QVariant());
        }
        case TypeInfo::Real:
        {
            const auto v = ref.toDouble(&ok);
            return (ok ? QVariant::fromValue(qreal(v)) : QVariant());
        }
            break;
        case TypeInfo::String:
            return QVariant::fromValue(ref);
        case TypeInfo::Url:
        {
            if (ref.startsWith(u':') || ref.startsWith(QDir::separator()))
                return QVariant::fromValue(QUrl::fromLocalFile(ref.toString()));
            else if (ref.startsWith(u'#'))
                return QVariant::fromValue(QUrl(ref.toString()));
            else
                return QVariant::fromValue(QUrl::fromUserInput(ref.toString(), workingDir.canonicalPath()));
        }
        case TypeInfo::Color:
            return QVariant::fromValue(QColor(ref));
        case TypeInfo::Time:
            return QVariant::fromValue(QTime::fromString(ref.toString()));
        case TypeInfo::Date:
            return QVariant::fromValue(QDate::fromString(ref.toString()));
        case TypeInfo::DateTime:
            return QVariant::fromValue(QDateTime::fromString(ref.toString()));
        case TypeInfo::Rect:
            return QVariant::fromValue(toRect(ref));
        case TypeInfo::Point:
            return QVariant::fromValue(toPoint(ref));
        case TypeInfo::Size:
            return QVariant::fromValue(toSize(ref));
        case TypeInfo::Vector2D:
            return QVariant::fromValue(toVec<QVector2D>(ref));
        case TypeInfo::Vector3D:
            return QVariant::fromValue(toVec<QVector3D>(ref));
        case TypeInfo::Vector4D:
            return QVariant::fromValue(toVec<QVector4D>(ref));
        case TypeInfo::Quaternion:
            return QVariant::fromValue(toQuaternion(ref));
        case TypeInfo::Font:
            break;
        case TypeInfo::Matrix4x4:
            break;
        case TypeInfo::InvalidBuiltin:
            break;
        }

        return QVariant();
    };

    if (p.propertyType == TypeInfo::Builtin) { // Built in Qt types int, vector3d etc
        return toBuiltinType(p.builtinType, ref, ctx.workingDir);
    } else { // hard mode, detect the property type
        // We only care about the types that are relevant for us
        if (p.targetType != TypeInfo::Unknown) {
            Q_ASSERT(p.target);
            Q_ASSERT(!p.name.isEmpty());
            const int idx = p.target->metaObject()->indexOfProperty(p.name.toLatin1().constData());
            const auto property = p.target->metaObject()->property(idx);
            if (property.metaType().id() >= QMetaType::User) {
                const QMetaType metaType = property.metaType();
                switch (p.targetType) {
                case TypeInfo::DefaultMaterial:
                    Q_FALLTHROUGH();
                case TypeInfo::PrincipledMaterial:
                {
                    // Common for both materials
                    if (metaType.id() == qMetaTypeId<QQuick3DMaterial::CullMode>())
                        return fromStringEnumHelper<QQuick3DMaterial::CullMode>(ref, property);
                    if (metaType.id() == qMetaTypeId<QQuick3DMaterial::TextureChannelMapping>())
                        return fromStringEnumHelper<QQuick3DMaterial::TextureChannelMapping>(ref, property);

                    if (p.targetType == TypeInfo::PrincipledMaterial) {
                        if (metaType.id() == qMetaTypeId<QQuick3DPrincipledMaterial::Lighting>())
                            return fromStringEnumHelper<QQuick3DPrincipledMaterial::Lighting>(ref, property);
                        if (metaType.id() == qMetaTypeId<QQuick3DPrincipledMaterial::BlendMode>())
                            return fromStringEnumHelper<QQuick3DPrincipledMaterial::BlendMode>(ref, property);
                        if (metaType.id() == qMetaTypeId<QQuick3DPrincipledMaterial::AlphaMode>())
                            return fromStringEnumHelper<QQuick3DPrincipledMaterial::AlphaMode>(ref, property);
                    } else if (p.targetType == TypeInfo::DefaultMaterial) {
                        if (metaType.id() == qMetaTypeId<QQuick3DDefaultMaterial::Lighting>())
                            return fromStringEnumHelper<QQuick3DDefaultMaterial::Lighting>(ref, property);
                        if (metaType.id() == qMetaTypeId<QQuick3DDefaultMaterial::BlendMode>())
                            return fromStringEnumHelper<QQuick3DDefaultMaterial::BlendMode>(ref, property);
                        if (metaType.id() == qMetaTypeId<QQuick3DDefaultMaterial::SpecularModel>())
                            return fromStringEnumHelper<QQuick3DDefaultMaterial::SpecularModel>(ref, property);
                    }
                }
                    break;
                case TypeInfo::CustomMaterial:
                    if (metaType.id() == qMetaTypeId<QQuick3DCustomMaterial::ShadingMode>())
                        return fromStringEnumHelper<QQuick3DCustomMaterial::ShadingMode>(ref, property);
                    if (metaType.id() == qMetaTypeId<QQuick3DCustomMaterial::BlendMode>())
                        return fromStringEnumHelper<QQuick3DCustomMaterial::BlendMode>(ref, property);
                    break;
                case TypeInfo::SpotLight:
                    Q_FALLTHROUGH();
                case TypeInfo::PointLight:
                    if (metaType.id() == qMetaTypeId<QQuick3DAbstractLight::QSSGShadowMapQuality>())
                        return fromStringEnumHelper<QQuick3DAbstractLight::QSSGShadowMapQuality>(ref, property);
                    break;
                case TypeInfo::SceneEnvironment:
                    if (metaType.id() == qMetaTypeId<QQuick3DSceneEnvironment::QQuick3DEnvironmentBackgroundTypes>())
                        return fromStringEnumHelper<QQuick3DSceneEnvironment::QQuick3DEnvironmentBackgroundTypes>(ref, property);
                    if (metaType.id() == qMetaTypeId<QQuick3DSceneEnvironment::QQuick3DEnvironmentAAModeValues>())
                        return fromStringEnumHelper<QQuick3DSceneEnvironment::QQuick3DEnvironmentAAModeValues>(ref, property);
                    if (metaType.id() == qMetaTypeId<QQuick3DSceneEnvironment::QQuick3DEnvironmentAAQualityValues>())
                        return fromStringEnumHelper<QQuick3DSceneEnvironment::QQuick3DEnvironmentAAQualityValues>(ref, property);
                    if (metaType.id() == qMetaTypeId<QQuick3DSceneEnvironment::QQuick3DEnvironmentTonemapModes>())
                        return fromStringEnumHelper<QQuick3DSceneEnvironment::QQuick3DEnvironmentTonemapModes>(ref, property);
                    Q_FALLTHROUGH();
                default:
                    break;
                }
            } else { // Qt type
                return toBuiltinType(property.metaType().id(), ref, ctx.workingDir);
            }
        }

        printf("Unhandled type for property %s\n", ref.toLatin1().constData());
    }

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
            printf("Object definition -> %s\n", def.qualifiedTypeNameId->name.toLocal8Bit().constData());
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
            printf("Script binding -> %s ", binding.qualifiedId->name.toLocal8Bit().constData());

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
            printf("Array binding(s) -> %s: [\n", arrayBinding.qualifiedId->name.toLocal8Bit().constData());

        const auto oldName = ctx.property.name; // reentrancy
        ctx.property.name = arrayBinding.qualifiedId->name;

        if (arrayBinding.members)
            visit(*arrayBinding.members, ctx, ret);

        if (ctx.dbgprint)
            printf("]\n");

        ctx.property.name = oldName;
    }

    static void visit(const QQmlJS::AST::UiObjectBinding &objectBinding, Context &ctx, int &ret)
    {
        using namespace QQmlJS::AST;

        if (ctx.dbgprint)
            printf("Object binding -> %s: %s {\n", objectBinding.qualifiedId->name.toLocal8Bit().constData(), objectBinding.qualifiedTypeNameId->name.toLocal8Bit().constData());

        if (objectBinding.initializer) {
            if (!(ctx.interceptOBFunc && ctx.interceptOBFunc(objectBinding, ctx, ret)))
                visit(*objectBinding.initializer, ctx, ret);
        }

        if (ctx.dbgprint)
            printf("}\n");
    }

    static void visit(const QQmlJS::AST::UiPublicMember &member, Context &ctx, int &ret)
    {
        using namespace QQmlJS::AST;

        if (ctx.dbgprint)
            printf("%s member -> %s ", (member.type == UiPublicMember::Signal ? "Signal" : "Property"), member.name.toLocal8Bit().constData());

        auto name = ctx.property.name;
        const bool isMemberCpy = ctx.property.isMember;
        ctx.property.name = member.name;
        ctx.property.isMember = true;
        const auto propCpy = ctx.property.propertyType;
        const auto builtinTypeCpy = ctx.property.builtinType;

        if (!(ctx.interceptPMFunc && ctx.interceptPMFunc(member, ctx, ret))) {
            if (member.statement) {
                const auto &statement = member.statement;
                if (statement->kind == Node::Kind_ExpressionStatement)
                    visit(static_cast<const ExpressionStatement &>(*statement), ctx, ret);
                else if (ctx.dbgprint)
                    printf("Unhandled statement (%d)\n", statement->kind);
            } else if (member.binding) {
                const auto &binding = member.binding;
                if (binding->kind == Node::Kind_UiObjectBinding)
                    visit(static_cast<const UiObjectBinding &>(*binding), ctx, ret);
                else if (ctx.dbgprint)
                    printf("Unhandled binding (%d)\n", binding->kind);
            }
        }

        qSwap(ctx.property.name, name);
        ctx.property.propertyType = propCpy;
        ctx.property.builtinType = builtinTypeCpy;
        ctx.property.isMember = isMemberCpy;
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
                    printf("-> TrueLiteral: %s\n", v ? "true" : "false");
                if (ctx.property.target) {
                    auto target = ctx.property.target;
                    const auto &name = ctx.property.name;
                    target->setProperty(name.toLatin1(), QVariant::fromValue(v));
                }
            } else if (exprStatement.expression->kind == Node::Kind_ArrayPattern) {
                const auto &arrayPattern = static_cast<const ArrayPattern &>(*exprStatement.expression);
                visit(arrayPattern, ctx, ret);
            } else if (exprStatement.expression->kind == Node::Kind_CallExpression) {
                const auto &callExpression = static_cast<const CallExpression &>(*exprStatement.expression);
                visit(callExpression, ctx, ret);
            } else if (exprStatement.expression->kind == Node::Kind_UnaryMinusExpression) {
                const auto &unaryMinusExpr = static_cast<const UnaryMinusExpression &>(*exprStatement.expression);
                if (unaryMinusExpr.expression && unaryMinusExpr.expression->kind == Node::Kind_NumericLiteral) {
                    auto &numericLiteral = static_cast<NumericLiteral &>(*unaryMinusExpr.expression);
                    const auto value = numericLiteral.value;
                    numericLiteral.value *= -1;
                    visit(numericLiteral, ctx, ret);
                    numericLiteral.value = value;
                }
            } else if (exprStatement.expression->kind == Node::Kind_UnaryPlusExpression) {
                const auto &unaryPlusExpr = static_cast<const UnaryPlusExpression &>(*exprStatement.expression);
                if (unaryPlusExpr.expression)
                    visit(static_cast<const NumericLiteral &>(*unaryPlusExpr.expression), ctx, ret);
            } else {
                if (ctx.dbgprint)
                    printf("<expression: %d>\n", exprStatement.expression->kind);
            }
        }
    }

    static void visit(const QQmlJS::AST::IdentifierExpression &idExpr, Context &ctx, int &ret)
    {
        Q_UNUSED(ret);
        if (ctx.dbgprint)
            printf("-> Identifier: %s\n", idExpr.name.toLocal8Bit().constData());

        if (ctx.property.target) {
            const auto foundIt = ctx.identifierMap.constFind(idExpr.name);
            const auto end = ctx.identifierMap.constEnd();
            if (foundIt != end) {
                if (ctx.property.targetType == TypeInfo::Model) {
                    if (QQuick3DMaterial *mat = qobject_cast<QQuick3DMaterial *>(*foundIt)) {
                        auto materials = qobject_cast<QQuick3DModel *>(ctx.property.target)->materials();
                        materials.append(&materials, mat);
                        // Remove the material from the "free" list (materials that aren't use anywhere).
                        auto &freeMaterials = ctx.sceneData.materials;
                        if (const auto idx = freeMaterials.indexOf({ qobject_cast<QQuick3DPrincipledMaterial *>(mat), TypeInfo::PrincipledMaterial }))
                            freeMaterials.removeAt(idx);
                        if (ctx.dbgprint)
                            printf("Appending material to %s\n", ctx.property.name.toLatin1().constData());
                    }
                }
            } else {
                ctx.identifierMap.insert(idExpr.name, ctx.property.target);
            }
        }
    }

    static void visit(const QQmlJS::AST::StringLiteral &stringLiteral, Context &ctx, int &ret)
    {
        Q_UNUSED(ret);
        if (ctx.dbgprint)
            printf("-> StringLiteral: \"%s\"\n", stringLiteral.value.toLocal8Bit().constData());

        if (ctx.property.target) {
            const auto &name = ctx.property.name;
            const auto v = fromString(stringLiteral.value, ctx);
            if (v.isValid()) {
                const bool b = ctx.property.target->setProperty(name.toLatin1(), v);
                if (b && ctx.dbgprint)
                    printf("Property %s updated!\n", name.toLatin1().constData());
            }
        }
    }

    static void visit(const QQmlJS::AST::NumericLiteral &numericLiteral, Context &ctx, int &ret)
    {
        Q_UNUSED(ret);
        if (ctx.dbgprint)
            printf("-> NumericLiteral: %f\n", numericLiteral.value);

        if (ctx.property.target) {
            auto target = ctx.property.target;
            const auto &name = ctx.property.name;
            target->setProperty(name.toLatin1(), QVariant::fromValue(numericLiteral.value));
        }
    }

    static void visit(const QQmlJS::AST::FieldMemberExpression &fieldMemberExpression, Context &ctx, int &ret)
    {
        using namespace QQmlJS::AST;

        Q_UNUSED(ret);
        if (ctx.dbgprint)
            printf("-> FieldMemberExpression: %s\n", fieldMemberExpression.name.toLocal8Bit().constData());

        if (ctx.property.target) {
            const auto &name = ctx.property.name;
            const auto v = fromString(fieldMemberExpression.name, ctx);
            if (v.isValid())
                ctx.property.target->setProperty(name.toLatin1(), v);
        }
    }

    static void visit(const QQmlJS::AST::ArrayPattern &arrayPattern, Context &ctx, int &ret)
    {
        Q_UNUSED(ret);
        if (ctx.dbgprint)
            printf("-> [ ");

        using namespace QQmlJS::AST;
        using PatternElementItem = PatternElementList;
        using PatternElementListView = InvasiveListView<PatternElementItem>;

        PatternElementListView elements(*arrayPattern.elements);
        for (auto &element : elements) {
            auto patternElement = element.element;
            if (patternElement->type == PatternElement::Literal) {
                if (patternElement->initializer && patternElement->initializer->kind == Node::Kind_IdentifierExpression) {
                    const auto &identExpression = static_cast<const IdentifierExpression &>(*patternElement->initializer);
                    visit(identExpression, ctx, ret);
                }
            } else if (ctx.dbgprint) {
                printf("Unahandled(%d), ", patternElement->type);
            }
        }

        if (ctx.dbgprint)
            printf(" ]\n");

    }

    static void visit(const QQmlJS::AST::CallExpression &callExpression, Context &ctx, int &ret)
    {
        using namespace QQmlJS::AST;

        Q_UNUSED(ret);
        Q_UNUSED(callExpression);
        if (ctx.dbgprint)
            printf("-> Call(%d)\n", callExpression.base->kind);

        (ctx.interceptCallExpr && ctx.interceptCallExpr(callExpression, ctx, ret));
    }

    static void visit(const QQmlJS::AST::UiObjectMember &member, Context &ctx, int &ret)
    {
        using namespace QQmlJS::AST;

        if (member.kind == Node::Kind_UiObjectBinding)
            visit(static_cast<const UiObjectBinding &>(member), ctx, ret);
        else if (ctx.dbgprint)
            printf("Unhandled member (%d)\n", member.kind);
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
                } else if (member.member->kind == Node::Kind_UiPublicMember) {
                    const auto &pubMember = static_cast<const UiPublicMember &>(*member.member);
                    visit(pubMember, ctx, ret);
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

    if (obj.initializer) {
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
                                         TypeInfo::QmlType lightType,
                                         const QQuick3DAbstractLight *base = nullptr)
{
    switch (lightType) {
    case TypeInfo::DirectionalLight:
        return buildType(def, ctx, ret, qobject_cast<const QQuick3DDirectionalLight *>(base));
    case TypeInfo::PointLight:
        return buildType(def, ctx, ret, qobject_cast<const QQuick3DPointLight *>(base));
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
            auto &components = ctx.components;
            const auto compIt = components.constFind(typeName);
            const QQuick3DSceneEnvironment *base = (compIt != components.cend()) ? qobject_cast<QQuick3DSceneEnvironment *>(compIt->ptr) : nullptr;
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
            auto &components = ctx.components;
            const auto compIt = components.constFind(typeName);
            const QQuick3DTexture *base = (compIt != components.cend()) ? qobject_cast<QQuick3DTexture *>(compIt->ptr) : nullptr;
            if (QQuick3DTexture *tex = buildType(objectBinding, ctx, ret, base)) {
                if (ctx.property.target) {
                    if (ctx.dbgprint)
                        printf("Updating property %s on %s\n", propName.toLatin1().constData(), TypeInfo::typeStringTable[ctx.property.targetType]);
                    const auto &target = ctx.property.target;
                    if (ctx.property.isMember || target->metaObject()->indexOfProperty(propName.toLatin1().constData()) != -1)
                        target->setProperty(propName.toLatin1().constData(), QVariant::fromValue(tex));
                }
                ctx.sceneData.textures.append(tex);
            }
            handled = true;
            break;
        }
        case TypeInfo::TextureInput:
        {
            auto &components = ctx.components;
            const auto compIt = components.constFind(typeName);
            const QQuick3DShaderUtilsTextureInput *base = (compIt != components.cend()) ? qobject_cast<QQuick3DShaderUtilsTextureInput *>(compIt->ptr) : nullptr;
            if (QQuick3DShaderUtilsTextureInput *texInput = buildType(objectBinding, ctx, ret, base)) {
                if (ctx.property.target) {
                    if (ctx.dbgprint)
                        printf("Updating property %s on %s\n", propName.toLatin1().constData(), TypeInfo::typeStringTable[ctx.property.targetType]);
                    const auto &target = ctx.property.target;
                    if (ctx.property.isMember || target->metaObject()->indexOfProperty(propName.toLatin1().constData()) != -1)
                        target->setProperty(propName.toLatin1().constData(), QVariant::fromValue(texInput));
                }
                ctx.sceneData.textures.append(texInput->texture());
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
            const auto componentName = QStringView(fileName).left(fileName.length() - 4);
            s_typeMap->insert(componentName.toString(), typeIt.value());
        } else if (ctx.dbgprint) {
            printf("Resolving: %s -> \'%s\'\n", qPrintable(typeIt.key()), TypeInfo::typeStringTable[type]);
        }

        switch (type) {
        case TypeInfo::View3D:
        {
            auto &components = ctx.components;
            const auto compIt = components.constFind(typeName);
            const QQuick3DViewport *base = (compIt != components.cend()) ? qobject_cast<QQuick3DViewport *>(compIt->ptr) : nullptr;
            if (QQuick3DViewport *viewport = buildType(def, ctx, ret, base)) {
                // If this is a component we'll store it for lookups later.
                if (!base) {
                    const auto &fileName = ctx.currentFileInfo.fileName();
                    const auto componentName = QStringView(fileName).left(fileName.length() - 4);
                    components.insert(componentName.toString(), { viewport, type });
                }
                // Only one viewport supported atm (see SceneEnvironment case as well).
                if (!ctx.sceneData.viewport)
                    ctx.sceneData.viewport = viewport;
            }
            break;
        }
        case TypeInfo::SceneEnvironment:
        {
            auto &components = ctx.components;
            const auto compIt = components.constFind(typeName);
            const QQuick3DSceneEnvironment *base = (compIt != components.cend()) ? qobject_cast<QQuick3DSceneEnvironment *>(compIt->ptr) : nullptr;
            if (QQuick3DSceneEnvironment *sceneEnv = buildType(def, ctx, ret, base)) {
                // If this is a component we'll store it for lookups later.
                if (!base) {
                    const auto &fileName = ctx.currentFileInfo.fileName();
                    const auto componentName = QStringView(fileName).left(fileName.length() - 4);
                    components.insert(componentName.toString(), { sceneEnv, type });
                }

                if (ctx.sceneData.viewport)
                    ctx.sceneData.viewport->setEnvironment(sceneEnv);
            }
            break;
        }
        case TypeInfo::PrincipledMaterial:
        {
            auto &components = ctx.components;
            const auto compIt = components.constFind(typeName);
            const QQuick3DPrincipledMaterial *base = (compIt != components.cend()) ? qobject_cast<QQuick3DPrincipledMaterial *>(compIt->ptr) : nullptr;
            if (QQuick3DPrincipledMaterial *mat = buildType(def, ctx, ret, base)) {
                // If this is a component we'll store it for lookups later.
                if (!base) {
                    const auto &fileName = ctx.currentFileInfo.fileName();
                    const auto componentName = QStringView(fileName).left(fileName.length() - 4);
                    components.insert(componentName.toString(), { mat, type });
                }

                bool handled = false;
                if (ctx.property.target) {
                    if (ctx.property.targetType == TypeInfo::Model) {
                        auto materials = qobject_cast<QQuick3DModel *>(ctx.property.target)->materials();
                        materials.append(&materials, mat);
                        handled = true;
                        if (ctx.dbgprint)
                            printf("Appending material to %s\n", ctx.property.name.toLatin1().constData());
                    }
                }

                if (!handled)
                    ctx.sceneData.materials.push_back({ mat, TypeInfo::PrincipledMaterial });
            }
            break;
        }
        case TypeInfo::DefaultMaterial:
        {
            auto &components = ctx.components;
            const auto compIt = components.constFind(typeName);
            const QQuick3DDefaultMaterial *base = (compIt != components.cend()) ? qobject_cast<QQuick3DDefaultMaterial *>(compIt->ptr) : nullptr;
            if (QQuick3DDefaultMaterial *mat = buildType(def, ctx, ret, base)) {
                // If this is a component we'll store it for lookups later.
                if (!base) {
                    const auto &fileName = ctx.currentFileInfo.fileName();
                    const auto componentName = QStringView(fileName).left(fileName.length() - 4);
                    components.insert(componentName.toString(), { mat, type });
                }

                bool handled = false;
                if (ctx.property.target) {
                    if (ctx.property.targetType == TypeInfo::Model) {
                        auto materials = qobject_cast<QQuick3DModel *>(ctx.property.target)->materials();
                        materials.append(&materials, mat);
                        handled = true;
                        if (ctx.dbgprint)
                            printf("Appending material to %s\n", ctx.property.name.toLatin1().constData());
                    }
                }

                if (!handled)
                    ctx.sceneData.materials.push_back({ mat, TypeInfo::DefaultMaterial });
            }
            break;
        }
        case TypeInfo::CustomMaterial:
        {
            auto &components = ctx.components;
            const auto compIt = components.constFind(typeName);
            const QQuick3DCustomMaterial *base = (compIt != components.cend()) ? qobject_cast<QQuick3DCustomMaterial *>(compIt->ptr) : nullptr;
            if (QQuick3DCustomMaterial *mat = buildType(def, ctx, ret, base)) {
                // If this is a component we'll store it for lookups later.
                if (!base) {
                    const auto &fileName = ctx.currentFileInfo.fileName();
                    const auto componentName = fileName.left(fileName.length() - 4);
                    components.insert(componentName, { mat, type });
                }

                bool handled = false;
                if (ctx.property.target) {
                    if (ctx.property.targetType == TypeInfo::Model) {
                        auto materials = qobject_cast<QQuick3DModel *>(ctx.property.target)->materials();
                        materials.append(&materials, mat);
                        handled = true;
                        if (ctx.dbgprint)
                            printf("Appending material to %s\n", ctx.property.name.toLatin1().constData());
                    }
                }

                if (!handled)
                    ctx.sceneData.materials.push_back({ mat, TypeInfo::CustomMaterial });
            }
            break;
        }
        case TypeInfo::DirectionalLight:
            Q_FALLTHROUGH();
        case TypeInfo::PointLight:
            Q_FALLTHROUGH();
        case TypeInfo::SpotLight:
        {
            auto &components = ctx.components;
            const auto compIt = components.constFind(typeName);
            const QQuick3DAbstractLight *base = (compIt != components.cend()) ? qobject_cast<QQuick3DAbstractLight *>(compIt->ptr) : nullptr;
            if (QQuick3DAbstractLight *light = buildLight(def, ctx, ret, type, base)) {
                // If this is a component we'll store it for lookups later.
                if (!base) {
                    const auto &fileName = ctx.currentFileInfo.fileName();
                    const auto componentName = QStringView(fileName).left(fileName.length() - 4);
                    components.insert(componentName.toString(), { light, type });
                }
                ctx.sceneData.lights.push_back({light, type});

            }
            break;
        }
        case TypeInfo::Texture:
        {
            auto &components = ctx.components;
            const auto compIt = components.constFind(typeName);
            const QQuick3DTexture *base = (compIt != components.cend()) ? qobject_cast<QQuick3DTexture *>(compIt->ptr) : nullptr;
            if (QQuick3DTexture *tex = buildType(def, ctx, ret, base)) {
                // If this is a component we'll store it for lookups later.
                if (!base) {
                    const auto &fileName = ctx.currentFileInfo.fileName();
                    const auto componentName = QStringView(fileName).left(fileName.length() - 4);
                    components.insert(componentName.toString(), { tex, type });
                }
                ctx.sceneData.textures.push_back(tex);

            }
            break;
        }
        case TypeInfo::Model:
        {
            auto &components = ctx.components;
            const auto compIt = components.constFind(typeName);
            const auto *base = (compIt != components.cend()) ? qobject_cast<QQuick3DModel *>(compIt->ptr) : nullptr;
            if (auto *model = buildType(def, ctx, ret, base)) {
                // If this is a component we'll store it for lookups later.
                if (!base) {
                    const auto &fileName = ctx.currentFileInfo.fileName();
                    const auto componentName = QStringView(fileName).left(fileName.length() - 4);
                    components.insert(componentName.toString(), { model, type });
                }
                ctx.sceneData.models.push_back(model);
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

static bool interceptPublicMember(const QQmlJS::AST::UiPublicMember &member, Context &ctx, int &ret)
{
    Q_UNUSED(ret);
    using namespace QQmlJS::AST;

    if (ctx.dbgprint)
        printf("Intercepted public member!\n");

    if (member.statement && member.statement->kind == Node::Kind_ExpressionStatement) {
        if (ctx.property.targetType == TypeInfo::CustomMaterial && member.memberType) {
            // For custom materials we have properties that are user provided, so we'll
            // need to add these to the objects properties (we add these here to be able
            // to piggyback on the existing type matching code) - TODO: This needs some
            // better solution later.
            if (member.memberType->name == u"real") {
                ctx.property.builtinType = TypeInfo::Real;
            } else if (member.memberType->name == u"bool") {
                ctx.property.builtinType = TypeInfo::Bool;
            } else if (member.memberType->name == u"int") {
                ctx.property.builtinType = TypeInfo::Int;
            } else if (member.memberType->name == u"size") {
                ctx.property.builtinType = TypeInfo::Size;
            } else if (member.memberType->name == u"rect") {
                ctx.property.builtinType = TypeInfo::Rect;
            } else if (member.memberType->name == u"point") {
                ctx.property.builtinType = TypeInfo::Point;
            } else if (member.memberType->name == u"color") {
                ctx.property.builtinType = TypeInfo::Color;
            } else if (member.memberType->name.startsWith(u"vector")) {
                if (member.memberType->name.endsWith(u"2d")) {
                    ctx.property.builtinType = TypeInfo::Vector2D;
                } else if (member.memberType->name.endsWith(u"3d")) {
                    ctx.property.builtinType = TypeInfo::Vector3D;
                } else if (member.memberType->name.endsWith(u"4d")) {
                    ctx.property.builtinType = TypeInfo::Vector4D;
                }
            } else if (member.memberType->name == u"matrix4x4") {
                ctx.property.builtinType = TypeInfo::Matrix4x4;;
            } else if (member.memberType->name == u"quaternion") {
                ctx.property.builtinType = TypeInfo::Quaternion;
            } else if (member.memberType->name == u"var") {
                ctx.property.builtinType = TypeInfo::Var;
            }

            ctx.property.propertyType = (ctx.property.builtinType != TypeInfo::InvalidBuiltin)
                    ? TypeInfo::Builtin
                    : TypeInfo::Unknown;
        }
    }

    return false;
}

static bool interceptCallExpression(const QQmlJS::AST::CallExpression &callExpression, Context &ctx, int &ret)
{
    Q_UNUSED(ret);
    using namespace QQmlJS::AST;
    using namespace BuiltinHelpers;

    if (ctx.dbgprint)
        printf("Intercepted call expression!\n");

    const bool ok = (ctx.property.target && !ctx.property.name.isEmpty());
    if (callExpression.base && ok) {
        if (callExpression.base->kind == Node::Kind_FieldMemberExpression) {
            const auto &fieldMemberExpression = static_cast<const FieldMemberExpression &>(*callExpression.base);
            if (fieldMemberExpression.base) {
                if (fieldMemberExpression.base->kind == Node::Kind_IdentifierExpression) {
                    const auto &identExpr = static_cast<const IdentifierExpression &>(*fieldMemberExpression.base);
                    if (identExpr.name == u"Qt") {
                        bool ok = false;
                        QVariant v;
                        if (fieldMemberExpression.name == u"point") {
                            const auto point = toPoint(ArgumentListView(*callExpression.arguments), &ok);
                            if (ctx.dbgprint)
                                printf("Qt.point(%f, %f)\n", point.x(), point.y());
                            setProperty(ctx.property, point);
                        } else if (fieldMemberExpression.name == u"size") {
                            const auto size = toSize(ArgumentListView(*callExpression.arguments), &ok);
                            if (ctx.dbgprint)
                                printf("Qt.size(%f, %f)\n", size.width(), size.height());
                            setProperty(ctx.property, size);
                        } else if (fieldMemberExpression.name == u"rect") {
                            const auto rect = toRect(ArgumentListView(*callExpression.arguments), &ok);
                            if (ctx.dbgprint)
                                printf("Qt.rect(%f, %f, %f, %f)\n", rect.x(), rect.y(), rect.width(), rect.height());
                            setProperty(ctx.property, rect);
                        } else if (fieldMemberExpression.name.startsWith(u"vector")) {
                            if (fieldMemberExpression.name.endsWith(u"2d")) {
                                const auto vec2 = toVec<QVector2D>(ArgumentListView(*callExpression.arguments), &ok);
                                if (ctx.dbgprint)
                                    printf("Qt.vector2d(%f, %f)\n", vec2.x(), vec2.y());
                                setProperty(ctx.property, vec2);
                            } else if (fieldMemberExpression.name.endsWith(u"3d")) {
                                const auto vec3 = toVec<QVector3D>(ArgumentListView(*callExpression.arguments), &ok);
                                if (ctx.dbgprint)
                                    printf("Qt.vector3d(%f, %f, %f)\n", vec3.x(), vec3.y(), vec3.z());
                                setProperty(ctx.property, vec3);
                            } else if (fieldMemberExpression.name.endsWith(u"4d")) {
                                const auto vec4 = toVec<QVector4D>(ArgumentListView(*callExpression.arguments), &ok);
                                if (ctx.dbgprint)
                                    printf("Qt.vector4d(%f, %f, %f, %f)\n", vec4.x(), vec4.y(), vec4.z(), vec4.w());
                                setProperty(ctx.property, vec4);
                            }
                        } else if (fieldMemberExpression.name == u"matrix4x4") {
                            const auto mat44 = toMat44(ArgumentListView(*callExpression.arguments), &ok);
                            if (ctx.dbgprint)
                                printf("Qt.matrix4x4(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)\n",
                                       mat44(0, 0), mat44(0, 1), mat44(0, 2), mat44(0, 3),
                                       mat44(1, 0), mat44(1, 1), mat44(1, 2), mat44(1, 3),
                                       mat44(2, 0), mat44(2, 1), mat44(2, 2), mat44(2, 3),
                                       mat44(3, 0), mat44(3, 1), mat44(3, 2), mat44(3, 3));
                            setProperty(ctx.property, mat44);
                        } else if (fieldMemberExpression.name == u"quaternion") {
                            const auto quat = toQuaternion(ArgumentListView(*callExpression.arguments), &ok);
                            if (ctx.dbgprint)
                                printf("Qt.quaternion(%f, %f, %f, %f)\n", quat.scalar(), quat.x(), quat.y(), quat.z());
                            setProperty(ctx.property, quat);
                        } else if (fieldMemberExpression.name == u"rgba") {
                            const auto vec4 = toVec<QVector4D>(ArgumentListView(*callExpression.arguments), &ok);
                            if (ok) {
                                QColor color = QColor::fromRgbF(vec4.x(), vec4.y(), vec4.z(), vec4.w());
                                if (ctx.dbgprint)
                                    printf("Qt.rgba(%f, %f, %f, %f)\n", color.redF(), color.greenF(), color.blueF(), color.alphaF());
                                setProperty(ctx.property, color);
                            }
                        }
                        if (ok && v.isValid() && ctx.property.target)
                            ctx.property.target->setProperty(ctx.property.name.toLatin1().constData(), v);
                    }
                }
            }
        }
    }

    return false;
}

static int parseQmlData(const QByteArray &code, Context &ctx)
{
    Q_ASSERT(ctx.engine && ctx.engine->lexer());
    ctx.identifierMap.clear(); // not visible outside the scope of this "code"
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

int MaterialParser::parseQmlData(const QByteArray &code, const QString &fileName, MaterialParser::SceneData &sceneData)
{
    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);

    Context ctx;
    ctx.engine = &engine;
    ctx.interceptODFunc = &interceptObjectDef;
    ctx.interceptOBFunc = &interceptObjectBinding;
    ctx.interceptPMFunc = &interceptPublicMember;
    ctx.interceptCallExpr = &interceptCallExpression;
    ctx.currentFileInfo = QFileInfo(fileName);
    ctx.type = Context::Type::Component;

    const int ret = ::parseQmlData(code, ctx);
    sceneData = std::move(ctx.sceneData);

    return ret;
}

int MaterialParser::parseQmlFiles(const QVector<QString> &filePaths, const QDir &sourceDir, SceneData &sceneData, bool verboseOutput)
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
    ctx.interceptPMFunc = &interceptPublicMember;
    ctx.interceptCallExpr = &interceptCallExpression;
    ctx.workingDir = sourceDir;

    QVector<QString> deferredOther;
    QVector<QString> deferredComponets;

    const QString sourcePath = sourceDir.canonicalPath() + QDir::separator();

    const bool isMultifile = filePaths.size() != 1;

    // Go through and find the material components first
    for (const auto &v : filePaths) {
        QFileInfo &currentFileInfo = ctx.currentFileInfo;
        if (!QFileInfo(v).isAbsolute())
            currentFileInfo.setFile(sourcePath + v);
        else
            currentFileInfo.setFile(v);
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
                    QVarLengthArray<TypeInfo::QmlType, 3> componentTypes { TypeInfo::PrincipledMaterial,
                                                                           TypeInfo::CustomMaterial,
                                                                           TypeInfo::DefaultMaterial};
                    for (const auto compType : qAsConst(componentTypes)) {
                        if ((idx = section.indexOf(TypeInfo::typeStringTable[compType])) != -1)
                            break;
                    }
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
            qWarning("The file %s is either not a file or has the wrong extension!", qPrintable(v));
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
