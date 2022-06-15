// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSG_RENDER_EFFECT_H
#define QSSG_RENDER_EFFECT_H

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

#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendernode_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>

#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE

struct QSSGRenderLayer;
struct QSSGCommand;

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderEffect : public QSSGRenderGraphObject
{
    QSSGRenderEffect();
    ~QSSGRenderEffect();

    enum class Flags : quint8
    {
        Active = 0x1u,
        Dirty = 0x2u
    };
    using FlagT = std::underlying_type_t<Flags>;

    struct TextureProperty
    {
        QSSGRenderImage *texImage = nullptr;
        QByteArray name;
        QSSGRenderShaderDataType shaderDataType;
        QSSGRenderTextureFilterOp minFilterType = QSSGRenderTextureFilterOp::Linear;
        QSSGRenderTextureFilterOp magFilterType = QSSGRenderTextureFilterOp::Linear;
        QSSGRenderTextureFilterOp mipFilterType = QSSGRenderTextureFilterOp::Linear;
        QSSGRenderTextureCoordOp horizontalClampType = QSSGRenderTextureCoordOp::ClampToEdge;
        QSSGRenderTextureCoordOp verticalClampType = QSSGRenderTextureCoordOp::ClampToEdge;
        QSSGRenderTextureTypeValue usageType;
    };

    QVector<TextureProperty> textureProperties;

    struct Property
    {
        Property() = default;
        Property(const QByteArray &name, const QByteArray &typeName, const QVariant &value, QSSGRenderShaderDataType shaderDataType, int pid = -1)
            : name(name), typeName(typeName), value(value), shaderDataType(shaderDataType), pid(pid)
        { }
        QByteArray name;
        QByteArray typeName;
        mutable QVariant value;
        QSSGRenderShaderDataType shaderDataType;
        int pid;
    };

    QVector<Property> properties;

    QSSGRenderLayer *m_layer = nullptr;
    QSSGRenderEffect *m_nextEffect = nullptr;

    void initialize();

    // If our active flag value changes, then we ask the effect manager
    // to reset our context.
    void setActive(bool inActive);
    [[nodiscard]] inline bool isActive() const { return ((flags & FlagT(Flags::Active)) != 0); }

    void markDirty();
    void clearDirty();
    [[nodiscard]] inline bool isDirty() const { return ((flags & FlagT(Flags::Dirty)) != 0); }

    QVector<QSSGCommand *> commands;

    const char *className = nullptr;
    FlagT flags { FlagT(Flags::Active) | FlagT(Flags::Dirty) };
    bool requiresDepthTexture = false;
    bool incompleteBuildTimeObject = false; // Used by the shadergen tool
    QSSGRenderTextureFormat::Format outputFormat = QSSGRenderTextureFormat::Unknown;
};

QT_END_NAMESPACE

#endif
