// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DSHADERUTILS_H
#define QQUICK3DSHADERUTILS_H

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

#include <QtQuick3D/qtquick3dglobal.h>
#include <QtQuick3D/private/qquick3dobject_p.h>
#include <QtQuick3D/private/qquick3dtexture_p.h>
#include <QtQuick3D/private/qquick3dmaterial_p.h>

#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendercommands_p.h>

QT_BEGIN_NAMESPACE

class QQuick3DShaderUtilsShader;
class QQmlContext;

namespace QSSGShaderUtils {

using MetaTypeList = QList<QMetaType::Type>;
using ResolveFunction = bool (*)(const QUrl &url, const QQmlContext *context, QByteArray &shaderData, QByteArray &shaderPathKey);
Q_QUICK3D_EXPORT void setResolveFunction(ResolveFunction fn);
Q_QUICK3D_EXPORT QByteArray resolveShader(const QUrl &fileUrl, const QQmlContext *context, QByteArray &shaderPathKey);
Q_QUICK3D_EXPORT MetaTypeList supportedMetatypes();


template<QMetaType::Type>
struct ShaderType
{
};

Q_QUICK3D_EXPORT QByteArray uniformTypeName(QMetaType type);
Q_QUICK3D_EXPORT QByteArray uniformTypeName(QSSGRenderShaderValue::Type type);
Q_QUICK3D_EXPORT QSSGRenderShaderValue::Type uniformType(QMetaType type);
}

class Q_QUICK3D_EXPORT QQuick3DShaderUtilsTextureInput : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuick3DTexture *texture READ texture WRITE setTexture NOTIFY textureChanged)
    Q_PROPERTY(bool enabled MEMBER enabled NOTIFY enabledChanged)

    QML_NAMED_ELEMENT(TextureInput)

public:
    explicit QQuick3DShaderUtilsTextureInput(QObject *p = nullptr);
    virtual ~QQuick3DShaderUtilsTextureInput();
    QQuick3DTexture *m_texture = nullptr;
    bool enabled = true;
    QByteArray name;
    QQuick3DTexture *texture() const
    {
        return m_texture;
    }

public Q_SLOTS:
    void setTexture(QQuick3DTexture *texture);

Q_SIGNALS:
    void textureChanged();
    void enabledChanged();
};

class Q_QUICK3D_EXPORT QQuick3DShaderUtilsBuffer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TextureFormat format READ format WRITE setFormat)
    Q_PROPERTY(TextureFilterOperation textureFilterOperation READ textureFilterOperation WRITE setTextureFilterOperation)
    Q_PROPERTY(TextureCoordOperation textureCoordOperation READ textureCoordOperation WRITE setTextureCoordOperation)
    Q_PROPERTY(float sizeMultiplier MEMBER sizeMultiplier)
    Q_PROPERTY(AllocateBufferFlagValues bufferFlags READ bufferFlags WRITE setBufferFlags)
    Q_PROPERTY(QByteArray name MEMBER name)

    QML_NAMED_ELEMENT(Buffer)

public:
    QQuick3DShaderUtilsBuffer() = default;
    ~QQuick3DShaderUtilsBuffer() override = default;

    enum class TextureFilterOperation // must match QSSGRenderTextureFilterOp
    {
        Unknown = 0,
        Nearest,
        Linear
    };
    Q_ENUM(TextureFilterOperation)

    enum class TextureCoordOperation // must match QSSGRenderTextureCoordOp
    {
        Unknown = 0,
        ClampToEdge,
        MirroredRepeat,
        Repeat
    };
    Q_ENUM(TextureCoordOperation)

    enum class AllocateBufferFlagValues
    {
        None = 0,
        SceneLifetime = 1
    };
    Q_ENUM(AllocateBufferFlagValues)

    enum class TextureFormat {
        Unknown = 0,
        RGBA8,
        RGBA16F,
        RGBA32F,
        R8,
        R16,
        R16F,
        R32F
    };
    Q_ENUM(TextureFormat)

    QSSGAllocateBuffer command {};
    TextureFilterOperation textureFilterOperation() const { return TextureFilterOperation(command.m_filterOp); }
    void setTextureFilterOperation(TextureFilterOperation op) { command.m_filterOp = QSSGRenderTextureFilterOp(op); }

    TextureCoordOperation textureCoordOperation() const { return TextureCoordOperation(command.m_texCoordOp); }
    void setTextureCoordOperation(TextureCoordOperation texCoordOp) { command.m_texCoordOp = QSSGRenderTextureCoordOp(texCoordOp); }
    float &sizeMultiplier = command.m_sizeMultiplier;
    QSSGCommand *getCommand() { return &command; }

    TextureFormat format() const;
    void setFormat(TextureFormat format);

    AllocateBufferFlagValues bufferFlags() const { return AllocateBufferFlagValues(int(command.m_bufferFlags)); }
    void setBufferFlags(AllocateBufferFlagValues flag) { command.m_bufferFlags = quint32(flag);}

    QByteArray &name = command.m_name;

    static QSSGRenderTextureFormat::Format mapTextureFormat(QQuick3DShaderUtilsBuffer::TextureFormat fmt);
    static QQuick3DShaderUtilsBuffer::TextureFormat mapRenderTextureFormat(QSSGRenderTextureFormat::Format fmt);
};

class Q_QUICK3D_EXPORT QQuick3DShaderUtilsRenderCommand : public QObject
{
    Q_OBJECT

    QML_NAMED_ELEMENT(Command)

public:
    QQuick3DShaderUtilsRenderCommand() = default;
    ~QQuick3DShaderUtilsRenderCommand() override = default;
    virtual QSSGCommand *getCommand() { Q_ASSERT(0); return nullptr; }
    virtual int bufferCount() const { return 0; }
    virtual QQuick3DShaderUtilsBuffer *bufferAt(int idx) const { Q_UNUSED(idx); return nullptr; }
};

class Q_QUICK3D_EXPORT QQuick3DShaderUtilsBufferInput : public QQuick3DShaderUtilsRenderCommand
{
    Q_OBJECT
    Q_PROPERTY(QQuick3DShaderUtilsBuffer *buffer READ buffer WRITE setBuffer)
    Q_PROPERTY(QByteArray sampler MEMBER sampler)

    QML_NAMED_ELEMENT(BufferInput)

public:
    QQuick3DShaderUtilsBufferInput() = default;
    ~QQuick3DShaderUtilsBufferInput() override = default;
    QSSGApplyBufferValue command { QByteArray(), QByteArray() };
    QByteArray &sampler = command.m_samplerName;
    QSSGCommand *getCommand() override { return &command; }

    int bufferCount() const override { return (m_buffer != nullptr) ? 1 : 0; }
    QQuick3DShaderUtilsBuffer *bufferAt(int idx) const override
    {
        Q_ASSERT(idx < 1 && idx >= 0);
        return (m_buffer && idx == 0) ? m_buffer : nullptr;
    }

    QQuick3DShaderUtilsBuffer *buffer() const { return m_buffer; }
    void setBuffer(QQuick3DShaderUtilsBuffer *buffer) {
        if (m_buffer == buffer)
            return;

        if (buffer) {
            Q_ASSERT(!buffer->name.isEmpty());
            command.m_bufferName = buffer->name;
        }
        m_buffer = buffer;
    }

    QQuick3DShaderUtilsBuffer *m_buffer = nullptr;

};

class Q_QUICK3D_EXPORT QQuick3DShaderUtilsApplyValue : public QQuick3DShaderUtilsRenderCommand
{
    Q_OBJECT
    Q_PROPERTY(QByteArray target MEMBER target)
    Q_PROPERTY(QVariant value MEMBER value)

    QML_NAMED_ELEMENT(SetUniformValue)

public:
    QQuick3DShaderUtilsApplyValue() = default;
    ~QQuick3DShaderUtilsApplyValue() override = default;
    QSSGCommand *getCommand() override { return &command; }
    QSSGApplyValue command { };
    QVariant &value = command.m_value;
    QByteArray &target = command.m_propertyName;
};

class Q_QUICK3D_EXPORT QQuick3DShaderUtilsRenderPass : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> commands READ commands)
    Q_PROPERTY(QQuick3DShaderUtilsBuffer *output MEMBER outputBuffer)
    Q_PROPERTY(QQmlListProperty<QQuick3DShaderUtilsShader> shaders READ shaders)

    QML_NAMED_ELEMENT(Pass)

public:
    QQuick3DShaderUtilsRenderPass() = default;
    ~QQuick3DShaderUtilsRenderPass() override = default;

    static void qmlAppendCommand(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list, QQuick3DShaderUtilsRenderCommand *command);
    static QQuick3DShaderUtilsRenderCommand *qmlCommandAt(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list, qsizetype index);
    static qsizetype qmlCommandCount(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list);
    static void qmlCommandClear(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list);

    static void qmlAppendShader(QQmlListProperty<QQuick3DShaderUtilsShader> *list, QQuick3DShaderUtilsShader *shader);
    static QQuick3DShaderUtilsShader *qmlShaderAt(QQmlListProperty<QQuick3DShaderUtilsShader> *list, qsizetype index);
    static qsizetype qmlShaderCount(QQmlListProperty<QQuick3DShaderUtilsShader> *list);
    static void qmlShaderClear(QQmlListProperty<QQuick3DShaderUtilsShader> *list);

    QQmlListProperty<QQuick3DShaderUtilsRenderCommand> commands();
    QVector<QQuick3DShaderUtilsRenderCommand *> m_commands;
    QQuick3DShaderUtilsBuffer *outputBuffer = nullptr;
    QQmlListProperty<QQuick3DShaderUtilsShader> shaders();
    QVarLengthArray<QQuick3DShaderUtilsShader *, 2> m_shaders;

Q_SIGNALS:
    void changed();
};

class Q_QUICK3D_EXPORT QQuick3DShaderUtilsShader : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl shader MEMBER shader NOTIFY shaderChanged)
    Q_PROPERTY(Stage stage MEMBER stage NOTIFY stageChanged)

    QML_NAMED_ELEMENT(Shader)

public:
    QQuick3DShaderUtilsShader() = default;
    virtual ~QQuick3DShaderUtilsShader() = default;
    enum class Stage : quint8
    {
        Vertex = 0,
        Fragment = 1
    };
    Q_ENUM(Stage)

    QUrl shader;
    Stage stage = Stage::Fragment;

Q_SIGNALS:
    void shaderChanged();
    void stageChanged();
};

QT_END_NAMESPACE

Q_DECLARE_OPAQUE_POINTER(QQuick3DShaderUtilsTextureInput)

#endif // QQUICK3DSHADERUTILS_H
