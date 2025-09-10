// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSG_RENDER_COMMANDS_H
#define QSSG_RENDER_COMMANDS_H

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

#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>

#include <QDebug>
#include <QVariant>

QT_BEGIN_NAMESPACE

struct QSSGRenderEffect;

enum class CommandType
{
    Unknown = 0,
    AllocateBuffer,
    BindTarget,
    BindBuffer,
    BindShader,
    ApplyInstanceValue,
    ApplyBufferValue,
    Render,
    ApplyValue,
};

struct QSSGCommand
{
    CommandType m_type;
    QSSGCommand(CommandType inType) : m_type(inType) {}
    const char *typeAsString() const;
    QString debugString() const;
    void addDebug(QDebug &stream) const;
};

enum class AllocateBufferFlagValues
{
    None = 0,
    SceneLifetime = 1,
};

struct QSSGAllocateBufferFlags : public QFlags<AllocateBufferFlagValues>
{
    QSSGAllocateBufferFlags(quint32 inValues) : QFlags(inValues) {}
    QSSGAllocateBufferFlags() {}
    void setSceneLifetime(bool inValue) { setFlag(AllocateBufferFlagValues::SceneLifetime, inValue); }
    // If isSceneLifetime is unset the buffer is assumed to be frame lifetime and will be
    // released after this render operation.
    bool isSceneLifetime() const { return this->operator&(AllocateBufferFlagValues::SceneLifetime); }
};

struct QSSGAllocateBuffer : public QSSGCommand
{
    QByteArray m_name;
    QSSGRenderTextureFormat m_format = QSSGRenderTextureFormat::RGBA8;
    QSSGRenderTextureFilterOp m_filterOp = QSSGRenderTextureFilterOp::Linear;
    QSSGRenderTextureCoordOp m_texCoordOp = QSSGRenderTextureCoordOp::ClampToEdge;
    float m_sizeMultiplier = 1.0f;
    QSSGAllocateBufferFlags m_bufferFlags;
    QSSGAllocateBuffer() : QSSGCommand(CommandType::AllocateBuffer) {}
    QSSGAllocateBuffer(const QByteArray &inName,
                         QSSGRenderTextureFormat inFormat,
                         QSSGRenderTextureFilterOp inFilterOp,
                         QSSGRenderTextureCoordOp inCoordOp,
                         float inMultiplier,
                         QSSGAllocateBufferFlags inFlags)
        : QSSGCommand(CommandType::AllocateBuffer)
        , m_name(inName)
        , m_format(inFormat)
        , m_filterOp(inFilterOp)
        , m_texCoordOp(inCoordOp)
        , m_sizeMultiplier(inMultiplier)
        , m_bufferFlags(inFlags)
    {
    }
    void addDebug(QDebug &stream) const {
        stream << "name:" <<  m_name << "format:" << m_format.toString() << "size multiplier:" << m_sizeMultiplier << "filter:" << QSSGBaseTypeHelpers::toString(m_filterOp) << "tiling:" << QSSGBaseTypeHelpers::toString(m_texCoordOp) << "sceneLifetime:" << m_bufferFlags.isSceneLifetime();
    }
};

struct QSSGBindTarget : public QSSGCommand
{
    QSSGRenderTextureFormat m_outputFormat;

    explicit QSSGBindTarget(QSSGRenderTextureFormat inFormat = QSSGRenderTextureFormat::RGBA8)
        : QSSGCommand(CommandType::BindTarget), m_outputFormat(inFormat)
    {
    }
    void addDebug(QDebug &stream) const {
        stream << "format" <<  m_outputFormat.toString();
    }
};

struct QSSGBindBuffer : public QSSGCommand
{
    QByteArray m_bufferName;
    QSSGBindBuffer(const QByteArray &inBufName)
        : QSSGCommand(CommandType::BindBuffer), m_bufferName(inBufName)
    {
    }
    void addDebug(QDebug &stream) const {
        stream << "name:" <<  m_bufferName;
    }
};

struct QSSGBindShader : public QSSGCommand
{
    QByteArray m_shaderPathKey; // something like "prefix>vertex_filename>fragment_filename:source_sha:y_up_in_fbo[:tonemapping]"
    QSSGBindShader(const QByteArray &inShaderPathKey)
        : QSSGCommand(CommandType::BindShader),
          m_shaderPathKey(inShaderPathKey)
    {
    }
    QSSGBindShader() : QSSGCommand(CommandType::BindShader) {}
    void addDebug(QDebug &stream) const {
        stream << "key:" <<  m_shaderPathKey << "effect:";
    }
};

// The value sits immediately after the 'this' object
// in memory.
// If propertyName is not valid then we attempt to apply all of the effect property values
// to the shader, ignoring ones that don't match up.
struct QSSGApplyInstanceValue : public QSSGCommand
{
    // Name of value to apply in shader
    QByteArray m_propertyName;
    // type of value
    QSSGRenderShaderValue::Type m_valueType;
    // offset in the effect data section of value.
    quint32 m_valueOffset;
    QSSGApplyInstanceValue(const QByteArray &inName, QSSGRenderShaderValue::Type inValueType, quint32 inValueOffset)
        : QSSGCommand(CommandType::ApplyInstanceValue), m_propertyName(inName), m_valueType(inValueType), m_valueOffset(inValueOffset)
    {
    }
    // Default will attempt to apply all effect values to the currently bound shader
    QSSGApplyInstanceValue()
        : QSSGCommand(CommandType::ApplyInstanceValue), m_valueType(QSSGRenderShaderValue::Unknown), m_valueOffset(0)
    {
    }
    void addDebug(QDebug &stream) const {
        stream << "name:" <<  m_propertyName << "type:" << int(m_valueType) << "offset:" << m_valueOffset ;
    }
};

struct QSSGApplyValue : public QSSGCommand
{
    QByteArray m_propertyName;
    QVariant m_value;
    explicit QSSGApplyValue(const QByteArray &inName)
        : QSSGCommand(CommandType::ApplyValue), m_propertyName(inName)
    {
    }
    // Default will attempt to apply all effect values to the currently bound shader
    QSSGApplyValue() : QSSGCommand(CommandType::ApplyValue) {}
    void addDebug(QDebug &stream) const {
        stream << "name:" <<  m_propertyName << "value:" << m_value;
    }
};

struct QSSGApplyBufferValue : public QSSGCommand
{
    QByteArray m_bufferName;
    QByteArray m_samplerName;

    QSSGApplyBufferValue(const QByteArray &bufferName, const QByteArray &shaderSampler)
        : QSSGCommand(CommandType::ApplyBufferValue), m_bufferName(bufferName), m_samplerName(shaderSampler)
    {
    }
    void addDebug(QDebug &stream) const {
        stream << "name:" <<  m_bufferName << "sampler:" << m_samplerName;
    }
};

struct QSSGRender : public QSSGCommand
{
    explicit QSSGRender() : QSSGCommand(CommandType::Render) { }

    QSSGRender(const QSSGRender &)
        : QSSGCommand(CommandType::Render)
    {
    }
    void addDebug(QDebug &stream) const {
        stream << "(no parameters)";
    }
};

QT_END_NAMESPACE

#endif
