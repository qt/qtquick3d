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
#include <QtQuick3DRuntimeRender/private/qssgrendershaderlibrarymanager_p.h>

#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE

struct QSSGRenderLayer;
struct QSSGCommand;
class QSSGRenderContextInterface;

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderEffect : public QSSGRenderGraphObject
{
    QSSGRenderEffect();
    ~QSSGRenderEffect();

    void finalizeShaders(const QSSGRenderLayer &layer, QSSGRenderContextInterface *renderContext);

    enum class Flags : quint8
    {
        Dirty = 0x1u
    };
    using FlagT = std::underlying_type_t<Flags>;

    struct TextureProperty
    {
        QSSGRenderImage *texImage = nullptr;
        QByteArray name;
        QSSGRenderShaderValue::Type shaderDataType;
        QSSGRenderTextureFilterOp minFilterType = QSSGRenderTextureFilterOp::Linear;
        QSSGRenderTextureFilterOp magFilterType = QSSGRenderTextureFilterOp::Linear;
        QSSGRenderTextureFilterOp mipFilterType = QSSGRenderTextureFilterOp::Linear;
        QSSGRenderTextureCoordOp horizontalClampType = QSSGRenderTextureCoordOp::ClampToEdge;
        QSSGRenderTextureCoordOp verticalClampType = QSSGRenderTextureCoordOp::ClampToEdge;
        QSSGRenderTextureCoordOp zClampType = QSSGRenderTextureCoordOp::ClampToEdge;
        QSSGRenderTextureTypeValue usageType;
    };

    QVector<TextureProperty> textureProperties;

    struct Property
    {
        Property() = default;
        Property(const QByteArray &name, const QByteArray &typeName, const QVariant &value, QSSGRenderShaderValue::Type shaderDataType, int pid = -1)
            : name(name), typeName(typeName), value(value), shaderDataType(shaderDataType), pid(pid)
        { }
        QByteArray name;
        QByteArray typeName;
        mutable QVariant value;
        QSSGRenderShaderValue::Type shaderDataType;
        int pid;
    };

    QVector<Property> properties;

    QSSGRenderEffect *m_nextEffect = nullptr;

    void markDirty();
    void clearDirty();
    [[nodiscard]] inline bool isDirty() const { return ((flags & FlagT(Flags::Dirty)) != 0); }

    struct Command {
        QSSGCommand *command;
        quint8 own : 1;
    };
    QVector<Command> commands;

    void resetCommands();

    const char *className = nullptr;
    FlagT flags = FlagT(Flags::Dirty);
    bool requiresDepthTexture = false;
    bool incompleteBuildTimeObject = false; // Used by the shadergen tool
    QSSGRenderTextureFormat::Format outputFormat = QSSGRenderTextureFormat::Unknown;

    struct ShaderPrepPassData
    {
        QByteArray shaderPathKeyPrefix; // to be completed in finalizeShaders
        QByteArray vertexShaderCode[2]; // without main(), to be completed in finalizeShaders
        QByteArray fragmentShaderCode[2]; // same here
        QSSGCustomShaderMetaData vertexMetaData[2];
        QSSGCustomShaderMetaData fragmentMetaData[2];
        int bindShaderCmdIndex = 0;
    };

    struct {
        bool valid = false;
        QVector<ShaderPrepPassData> passes;
    } shaderPrepData;

    QString debugObjectName;
};

QT_END_NAMESPACE

#endif
