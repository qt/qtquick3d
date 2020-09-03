/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2019 The Qt Company Ltd.
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

// All commands need at least two constructors.  One for when they are created that should
// setup all their member variables and one for when we are copying commands from an outside
// entity into the effect system.  We have to re-register strings in that case because we
// can't assume the outside entity was using the same string table we are...
struct QSSGCommand
{
    CommandType m_type;
    QSSGCommand(CommandType inType) : m_type(inType) {}
    QSSGCommand() : m_type(CommandType::Unknown) {}
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
    QSSGAllocateBuffer(const QSSGAllocateBuffer &inOther)
        : QSSGCommand(CommandType::AllocateBuffer)
        , m_name(inOther.m_name)
        , m_format(inOther.m_format)
        , m_filterOp(inOther.m_filterOp)
        , m_texCoordOp(inOther.m_texCoordOp)
        , m_sizeMultiplier(inOther.m_sizeMultiplier)
        , m_bufferFlags(inOther.m_bufferFlags)
    {
    }
    void addDebug(QDebug &stream) const {
        stream << "name:" <<  m_name << "format:" << m_format.toString() << "size multiplier:" << m_sizeMultiplier << "filter:" << toString(m_filterOp) << "tiling:" << toString(m_texCoordOp) << "sceneLifetime:" << m_bufferFlags.isSceneLifetime();
    }
};

struct QSSGBindTarget : public QSSGCommand
{
    QSSGRenderTextureFormat m_outputFormat;

    explicit QSSGBindTarget(QSSGRenderTextureFormat inFormat = QSSGRenderTextureFormat::RGBA8)
        : QSSGCommand(CommandType::BindTarget), m_outputFormat(inFormat)
    {
    }
    QSSGBindTarget(const QSSGBindTarget &inOther)
        : QSSGCommand(CommandType::BindTarget), m_outputFormat(inOther.m_outputFormat)
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
    QSSGBindBuffer(const QSSGBindBuffer &inOther)
        : QSSGCommand(CommandType::BindBuffer), m_bufferName(inOther.m_bufferName)
    {
    }
    void addDebug(QDebug &stream) const {
        stream << "name:" <<  m_bufferName;
    }
};

struct QSSGBindShader : public QSSGCommand
{
    QByteArray m_shaderPathKey; // something like "vertex_filename>fragment_filename"
    QSSGRenderEffect *m_effect;
    int m_passIndex;
    QSSGBindShader(const QByteArray &inShaderPathKey, QSSGRenderEffect *inEffect, int inPassIndex)
        : QSSGCommand(CommandType::BindShader),
          m_shaderPathKey(inShaderPathKey),
          m_effect(inEffect),
          m_passIndex(inPassIndex)
    {
    }
    QSSGBindShader() : QSSGCommand(CommandType::BindShader) {}
    QSSGBindShader(const QSSGBindShader &inOther)
        : QSSGCommand(CommandType::BindShader),
          m_shaderPathKey(inOther.m_shaderPathKey),
          m_effect(inOther.m_effect),
          m_passIndex(inOther.m_passIndex)
    {
    }
    void addDebug(QDebug &stream) const {
        stream << "path:" <<  m_shaderPathKey << "effect:" << m_effect << "pass index:" << m_passIndex;
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
    QSSGRenderShaderDataType m_valueType;
    // offset in the effect data section of value.
    quint32 m_valueOffset;
    QSSGApplyInstanceValue(const QByteArray &inName, QSSGRenderShaderDataType inValueType, quint32 inValueOffset)
        : QSSGCommand(CommandType::ApplyInstanceValue), m_propertyName(inName), m_valueType(inValueType), m_valueOffset(inValueOffset)
    {
    }
    // Default will attempt to apply all effect values to the currently bound shader
    QSSGApplyInstanceValue()
        : QSSGCommand(CommandType::ApplyInstanceValue), m_valueType(QSSGRenderShaderDataType::Unknown), m_valueOffset(0)
    {
    }
    QSSGApplyInstanceValue(const QSSGApplyInstanceValue &inOther)
        : QSSGCommand(CommandType::ApplyInstanceValue)
        , m_propertyName(inOther.m_propertyName)
        , m_valueType(inOther.m_valueType)
        , m_valueOffset(inOther.m_valueOffset)
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

    QSSGApplyValue(const QSSGApplyValue &inOther)
        : QSSGCommand(CommandType::ApplyValue)
        , m_propertyName(inOther.m_propertyName)
        , m_value(inOther.m_value)
    {
    }
    void addDebug(QDebug &stream) const {
        stream << "name:" <<  m_propertyName << "value:" << m_value;
    }
};

struct QSSGApplyBufferValue : public QSSGCommand
{
    QByteArray m_bufferName;
    QByteArray m_paramName;

    QSSGApplyBufferValue(const QByteArray &bufferName, const QByteArray &shaderParam)
        : QSSGCommand(CommandType::ApplyBufferValue), m_bufferName(bufferName), m_paramName(shaderParam)
    {
    }
    QSSGApplyBufferValue(const QSSGApplyBufferValue &inOther)
        : QSSGCommand(CommandType::ApplyBufferValue), m_bufferName(inOther.m_bufferName), m_paramName(inOther.m_paramName)
    {
    }
    void addDebug(QDebug &stream) const {
        stream << "name:" <<  m_bufferName << "parameter:" << m_paramName;
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
