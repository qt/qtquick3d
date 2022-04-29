/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
******************************************************************************/

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

    void reset();

    using Flag = QSSGRenderNode::Flag;
    Q_DECLARE_FLAGS(Flags, Flag)

    QVector<QSSGCommand *> commands;

    Flags flags;
    const char *className = nullptr;
    bool requiresDepthTexture = false;
    bool requiresCompilation = true;
    bool incompleteBuildTimeObject = false; // Used by the shadergen tool
    QSSGRenderTextureFormat::Format outputFormat = QSSGRenderTextureFormat::Unknown;
};

QT_END_NAMESPACE

#endif
