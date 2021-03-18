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

#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>
#include <QtQuick3DRender/private/qssgrenderimagetexture_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtGui/qcolor.h>

#include <limits>

QT_BEGIN_NAMESPACE

template<typename TDataType>
struct ShaderConstantApplier
{
    bool force_compile_error;
};

template<>
struct ShaderConstantApplier<qint32>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       const qint32 &inValue,
                       qint32 &oldValue)
    {
        if (!(inValue == oldValue)) {
            program->backend()->setConstantValue(program->handle(), location, type, count, &inValue);
            oldValue = inValue;
        }
    }
};

template<>
struct ShaderConstantApplier<qint32_2>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       const qint32_2 &inValue,
                       qint32_2 &oldValue)
    {
        if (!(inValue == oldValue)) {
            program->backend()->setConstantValue(program->handle(), location, type, count, &inValue.x);
            oldValue = inValue;
        }
    }
};

template<>
struct ShaderConstantApplier<qint32_3>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       const qint32_3 &inValue,
                       qint32_3 &oldValue)
    {
        if (!(inValue == oldValue)) {
            program->backend()->setConstantValue(program->handle(), location, type, count, &inValue.x);
            oldValue = inValue;
        }
    }
};

template<>
struct ShaderConstantApplier<qint32_4>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       const qint32_4 &inValue,
                       qint32_4 &oldValue)
    {
        if (!(inValue == oldValue)) {
            program->backend()->setConstantValue(program->handle(), location, type, count, &inValue.x);
            oldValue = inValue;
        }
    }
};

template<>
struct ShaderConstantApplier<bool>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       const bool inValue,
                       bool &oldValue)
    {
        if (!(inValue == oldValue)) {
            program->backend()->setConstantValue(program->handle(), location, type, count, &inValue);
            oldValue = inValue;
        }
    }
};

template<>
struct ShaderConstantApplier<bool_2>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       const bool_2 &inValue,
                       bool_2 &oldValue)
    {
        if (!(inValue == oldValue)) {
            program->backend()->setConstantValue(program->handle(), location, type, count, &inValue);
            oldValue = inValue;
        }
    }
};

template<>
struct ShaderConstantApplier<bool_3>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       const bool_3 &inValue,
                       bool_3 &oldValue)
    {
        if (!(inValue == oldValue)) {
            program->backend()->setConstantValue(program->handle(), location, type, count, &inValue);
            oldValue = inValue;
        }
    }
};

template<>
struct ShaderConstantApplier<bool_4>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       const bool_4 &inValue,
                       bool_4 &oldValue)
    {
        if (!(inValue == oldValue)) {
            program->backend()->setConstantValue(program->handle(), location, type, count, &inValue);
            oldValue = inValue;
        }
    }
};

template<>
struct ShaderConstantApplier<float>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       const float &inValue,
                       float &oldValue)
    {
        if (count > 1 || !(inValue == oldValue)) {
            program->backend()->setConstantValue(program->handle(), location, type, count, &inValue);
            oldValue = inValue;
        }
    }
};

template<>
struct ShaderConstantApplier<QVector2D>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       const QVector2D &inValue,
                       QVector2D &oldValue)
    {
        if (!(inValue == oldValue)) {
            program->backend()->setConstantValue(program->handle(), location, type, count, &inValue);
            oldValue = inValue;
        }
    }
};

template<>
struct ShaderConstantApplier<QVector3D>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       const QVector3D &inValue,
                       QVector3D &oldValue)
    {
        if (!(inValue == oldValue)) {
            program->backend()->setConstantValue(program->handle(), location, type, count, &inValue);
            oldValue = inValue;
        }
    }
};

template<>
struct ShaderConstantApplier<QVector4D>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       const QVector4D &inValue,
                       QVector4D &oldValue)
    {
        if (!(inValue == oldValue)) {
            program->backend()->setConstantValue(program->handle(), location, type, count, &inValue);
            oldValue = inValue;
        }
    }
};

template<>
struct ShaderConstantApplier<quint32>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       const quint32 &inValue,
                       quint32 &oldValue)
    {
        if (!(inValue == oldValue)) {
            program->backend()->setConstantValue(program->handle(), location, type, count, &inValue);
            oldValue = inValue;
        }
    }
};

template<>
struct ShaderConstantApplier<quint32_2>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       const quint32_2 &inValue,
                       quint32_2 &oldValue)
    {
        if (!(inValue == oldValue)) {
            program->backend()->setConstantValue(program->handle(), location, type, count, &inValue.x);
            oldValue = inValue;
        }
    }
};

template<>
struct ShaderConstantApplier<quint32_3>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       const quint32_3 &inValue,
                       quint32_3 &oldValue)
    {
        if (!(inValue == oldValue)) {
            program->backend()->setConstantValue(program->handle(), location, type, count, &inValue.x);
            oldValue = inValue;
        }
    }
};

template<>
struct ShaderConstantApplier<quint32_4>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       const quint32_4 &inValue,
                       quint32_4 &oldValue)
    {
        if (!(inValue == oldValue)) {
            program->backend()->setConstantValue(program->handle(), location, type, count, &inValue.x);
            oldValue = inValue;
        }
    }
};

template<>
struct ShaderConstantApplier<QMatrix3x3>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       const QMatrix3x3 inValue,
                       QMatrix3x3 &,
                       bool inTranspose)
    {
        program->backend()->setConstantValue(program->handle(), location, type, count, inValue.constData(), inTranspose);
    }
};

template<>
struct ShaderConstantApplier<QMatrix4x4>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       const QMatrix4x4 inValue,
                       QMatrix4x4 &,
                       bool inTranspose)
    {
        program->backend()->setConstantValue(program->handle(), location, type, count, inValue.constData(), inTranspose);
    }

    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       QSSGDataView<QMatrix4x4> inValue,
                       QMatrix4x4 &,
                       bool inTranspose)
    {
        program->backend()->setConstantValue(program->handle(),
                                  location,
                                  type,
                                  count,
                                  reinterpret_cast<const GLfloat *>(inValue.begin()),
                                  inTranspose);
    }
};

template<>
struct ShaderConstantApplier<QSSGRenderTexture2D *>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       QSSGRenderTexture2D *inValue,
                       quint32 &oldValue)
    {
        if (inValue) {
            QSSGRenderTexture2D *texObj = reinterpret_cast<QSSGRenderTexture2D *>(inValue);
            texObj->bind();
            quint32 texUnit = texObj->textureUnit();
            if (texUnit != oldValue) {
                program->backend()->setConstantValue(program->handle(), location, type, count, &texUnit);
                oldValue = texUnit;
            }
        }
    }
};

template<>
struct ShaderConstantApplier<QSSGRenderTexture2D **>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       QSSGRenderTexture2D **inValue,
                       QVector<quint32> &oldValue)
    {
        Q_UNUSED(type)
        if (inValue) {
            bool update = false;
            for (int i = 0; i < count; i++) {
                QSSGRenderTexture2D *texObj = reinterpret_cast<QSSGRenderTexture2D *>(inValue[i]);
                quint32 texUnit = std::numeric_limits<quint32>::max();
                if (texObj) {
                    texObj->bind();
                    texUnit = texObj->textureUnit();
                }
                if (texUnit != oldValue[i]) {
                    update = true;
                    oldValue[i] = texUnit;
                }
            }
            if (update)
                program->backend()->setConstantValue(program->handle(),
                                          location,
                                          QSSGRenderShaderDataType::Texture2D,
                                          count,
                                          oldValue.data());
        }
    }
};

template<>
struct ShaderConstantApplier<QSSGRenderTextureCube *>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       QSSGRenderTextureCube *inValue,
                       quint32 &oldValue)
    {
        if (inValue) {
            QSSGRenderTextureCube *texObj = reinterpret_cast<QSSGRenderTextureCube *>(inValue);
            texObj->bind();
            quint32 texUnit = texObj->textureUnit();
            if (texUnit != oldValue) {
                program->backend()->setConstantValue(program->handle(), location, type, count, &texUnit);
                oldValue = texUnit;
            }
        }
    }
};

template<>
struct ShaderConstantApplier<QSSGRenderTextureCube **>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       QSSGRenderTextureCube **inValue,
                       QVector<quint32> &oldValue)
    {
        Q_UNUSED(type)
        if (inValue) {
            bool update = false;
            for (int i = 0; i < count; i++) {
                QSSGRenderTextureCube *texObj = reinterpret_cast<QSSGRenderTextureCube *>(inValue[i]);
                quint32 texUnit = std::numeric_limits<quint32>::max();
                if (texObj) {
                    texObj->bind();
                    texUnit = texObj->textureUnit();
                }
                if (texUnit != oldValue[i]) {
                    update = true;
                    oldValue[i] = texUnit;
                }
            }
            if (update)
                program->backend()->setConstantValue(program->handle(),
                                          location,
                                          QSSGRenderShaderDataType::TextureCube,
                                          count,
                                          oldValue.data());
        }
    }
};

template<>
struct ShaderConstantApplier<QSSGRenderImage2D *>
{
    void applyConstant(const QSSGRenderShaderProgram *program,
                       qint32 location,
                       qint32 count,
                       QSSGRenderShaderDataType type,
                       QSSGRenderImage2D *image,
                       quint32 &oldValue,
                       qint32 binding)
    {
        if (image) {
            image->bind(binding);
            quint32 texUnit = image->textureUnit();
            if (texUnit != oldValue) {
                // on ES we need a explicit binding value
                Q_ASSERT(program->backend()->getRenderContextType() != QSSGRenderContextType::GLES3PLUS || binding != -1);
                // this is not allowed on ES 3+ for image types
                if (program->backend()->getRenderContextType() != QSSGRenderContextType::GLES3PLUS)
                    program->backend()->setConstantValue(program->handle(), location, type, count, &texUnit);

                oldValue = texUnit;
            }
        }
    }
};

QSSGRenderShaderProgram::QSSGRenderShaderProgram(const QSSGRef<QSSGRenderContext> &context, const char *programName, bool separableProgram)
    : m_context(context)
    , m_backend(context->backend())
    , m_programName(programName)
    , m_handle(nullptr)
    , m_programType(ProgramType::Graphics)
{
    m_handle = m_backend->createShaderProgram(separableProgram);

    Q_UNUSED(m_programName)
    Q_ASSERT(m_handle);
}

QSSGRenderShaderProgram::~QSSGRenderShaderProgram()
{
    m_context->shaderDestroyed(this);

    if (m_handle)
        m_backend->releaseShaderProgram(m_handle);

    m_handle = nullptr;
}

template<typename TShaderObject>
void QSSGRenderShaderProgram::attach(TShaderObject *pShader)
{
    m_backend->attachShader(m_handle, pShader);
}

template<typename TShaderObject>
void QSSGRenderShaderProgram::detach(TShaderObject *pShader)
{
    m_backend->detachShader(m_handle, pShader);
}

static QSSGRef<QSSGRenderShaderConstantBase> shaderConstantFactory(const QByteArray &inName,
                                                                       qint32 uniLoc,
                                                                       qint32 elementCount,
                                                                       QSSGRenderShaderDataType inConstantType,
                                                                       qint32 binding)
{
    switch (inConstantType) {
    case QSSGRenderShaderDataType::Integer:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<qint32>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::IntegerVec2:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<qint32_2>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::IntegerVec3:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<qint32_3>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::IntegerVec4:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<qint32_4>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::Boolean:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<bool>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::BooleanVec2:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<bool_2>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::BooleanVec3:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<bool_3>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::BooleanVec4:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<bool_4>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::Float:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<float>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::Vec2:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<QVector2D>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::Vec3:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<QVector3D>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::Vec4:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<QVector4D>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::UnsignedInteger:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<quint32>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::UnsignedIntegerVec2:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<quint32_2>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::UnsignedIntegerVec3:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<quint32_3>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::UnsignedIntegerVec4:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<quint32_4>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::Matrix3x3:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<QMatrix3x3>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::Matrix4x4:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<QMatrix4x4>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::Texture2D:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<QSSGRenderTexture2D *>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::Texture2DHandle:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<QSSGRenderTexture2D **>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::TextureCube:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<QSSGRenderTextureCube *>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::TextureCubeHandle:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<QSSGRenderTextureCube **>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::Image2D:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<QSSGRenderImage2D *>(inName, uniLoc, elementCount, inConstantType, binding));
    case QSSGRenderShaderDataType::DataBuffer:
        return QSSGRef<QSSGRenderShaderConstantBase>(
                new QSSGRenderShaderConstant<QSSGRenderDataBuffer *>(inName, uniLoc, elementCount, inConstantType, binding));
    default:
        break;
    }
    Q_ASSERT(false);
    return nullptr;
}

template<typename TShaderBufferType, typename TBufferDataType>
static QSSGRef<QSSGRenderShaderBufferBase> shaderBufferFactory(const QSSGRef<QSSGRenderContext> &context,
                                                                   const QByteArray &inName,
                                                                   qint32 cbLoc,
                                                                   qint32 cbBinding,
                                                                   qint32 cbSize,
                                                                   qint32 cbCount,
                                                                   const QSSGRef<TBufferDataType> &pBuffer)
{
    return QSSGRef<QSSGRenderShaderBufferBase>(new TShaderBufferType(context, inName, cbLoc, cbBinding, cbSize, cbCount, pBuffer));
}

void QSSGRenderShaderProgram::getShaderParameters()
{
    char nameBuf[512];
    qint32 location, elementCount, binding;
    QSSGRenderShaderDataType type;

    qint32 constantCount = m_backend->getConstantCount(m_handle);

    for (int idx = 0; idx != constantCount; ++idx) {
        location = m_backend->getConstantInfoByID(m_handle, idx, 512, &elementCount, &type, &binding, nameBuf);

        // sampler arrays have different type
        if (type == QSSGRenderShaderDataType::Texture2D && elementCount > 1) {
            type = QSSGRenderShaderDataType::Texture2DHandle;
        } else if (type == QSSGRenderShaderDataType::TextureCube && elementCount > 1) {
            type = QSSGRenderShaderDataType::TextureCubeHandle;
        }
        if (location != -1)
            m_constants.insert(nameBuf, shaderConstantFactory(nameBuf, location, elementCount, type, binding));
    }

    // next query constant buffers info
    qint32 length, bufferSize, paramCount;
    qint32 constantBufferCount = m_backend->getConstantBufferCount(m_handle);
    for (int idx = 0; idx != constantBufferCount; ++idx) {
        location = m_backend->getConstantBufferInfoByID(m_handle, idx, 512, &paramCount, &bufferSize, &length, nameBuf);

        if (location != -1) {
            // find constant buffer in our DB
            const QSSGRef<QSSGRenderConstantBuffer> &cb = m_context->getConstantBuffer(nameBuf);
            if (cb) {
                cb->setupBuffer(this, location, bufferSize, paramCount);
            }

            m_shaderBuffers.insert(nameBuf,
                                   shaderBufferFactory<QSSGRenderShaderConstantBuffer,
                                                       QSSGRenderConstantBuffer>(m_context, nameBuf, location, -1, bufferSize, paramCount, cb));
        }
    }

    // next query storage buffers
    qint32 storageBufferCount = m_backend->getStorageBufferCount(m_handle);
    for (int idx = 0; idx != storageBufferCount; ++idx) {
        location = m_backend->getStorageBufferInfoByID(m_handle, idx, 512, &paramCount, &bufferSize, &length, nameBuf);

        if (location != -1) {
            // find constant buffer in our DB
            const QSSGRef<QSSGRenderStorageBuffer> &sb = m_context->getStorageBuffer(nameBuf);
            m_shaderBuffers.insert(nameBuf,
                                   shaderBufferFactory<QSSGRenderShaderStorageBuffer,
                                                       QSSGRenderStorageBuffer>(m_context, nameBuf, location, -1, bufferSize, paramCount, sb));
        }
    }
}

bool QSSGRenderShaderProgram::link()
{
    bool success = m_backend->linkProgram(m_handle, m_errorMessage);
    if (success)
        getShaderParameters();
    return success;
}

bool QSSGRenderShaderProgram::link(quint32 format, const QByteArray &binary)
{
    bool success = m_backend->linkProgram(m_handle, m_errorMessage, format, binary);
    if (success)
        getShaderParameters();
    return success;
}

QByteArray QSSGRenderShaderProgram::errorMessage()
{
    return m_errorMessage;
}

QSSGRef<QSSGRenderShaderConstantBase> QSSGRenderShaderProgram::shaderConstant(const QByteArray &constantName) const
{
    const auto foundIt = m_constants.constFind(constantName);
    return (foundIt != m_constants.cend()) ? foundIt.value() : nullptr;
}

QSSGRef<QSSGRenderShaderBufferBase> QSSGRenderShaderProgram::shaderBuffer(const QByteArray &bufferName) const
{
    const auto foundIt = m_shaderBuffers.constFind(bufferName);
    return (foundIt != m_shaderBuffers.cend()) ? foundIt.value() : nullptr;
}

const QSSGRef<QSSGRenderContext> &QSSGRenderShaderProgram::renderContext()
{
    return m_context;
}

template<typename TDataType>
void setConstantValueOfType(const QSSGRenderShaderProgram *program,
                            QSSGRenderShaderConstantBase *inConstantBase,
                            const TDataType &inValue,
                            const qint32 inCount)
{
    if (inConstantBase == nullptr) {
        Q_ASSERT(false);
        return;
    }

    Q_ASSERT(inConstantBase->m_elementCount >= inCount);

    if (inConstantBase->getShaderConstantType() == QSSGDataTypeToShaderDataTypeMap<TDataType>::getType()) {
        QSSGRenderShaderConstant<TDataType> *inConstant = static_cast<QSSGRenderShaderConstant<TDataType> *>(inConstantBase);
        ShaderConstantApplier<TDataType>().applyConstant(program,
                                                         inConstant->m_location,
                                                         inCount,
                                                         inConstant->m_type,
                                                         inValue,
                                                         inConstant->m_value);
    } else {
        Q_ASSERT(false);
    }
}

template<typename TDataType>
void setSamplerConstantValueOfType(const QSSGRenderShaderProgram *program,
                                   QSSGRenderShaderConstantBase *inConstantBase,
                                   const TDataType &inValue,
                                   const qint32 inCount)
{
    if (inConstantBase == nullptr) {
        Q_ASSERT(false);
        return;
    }

    Q_ASSERT(inConstantBase->m_elementCount >= inCount);

    if (inConstantBase->getShaderConstantType() == QSSGDataTypeToShaderDataTypeMap<TDataType>::getType()) {
        QSSGRenderShaderConstant<TDataType> *inConstant = static_cast<QSSGRenderShaderConstant<TDataType> *>(inConstantBase);
        ShaderConstantApplier<TDataType>().applyConstant(program,
                                                         inConstant->m_location,
                                                         inCount,
                                                         inConstant->m_type,
                                                         inValue,
                                                         inConstant->m_value,
                                                         inConstant->m_binding);
    } else {
        Q_ASSERT(false);
    }
}

template<typename TDataType>
void setMatrixConstantValueOfType(const QSSGRenderShaderProgram *program,
                                  QSSGRenderShaderConstantBase *inConstantBase,
                                  const TDataType &inValue,
                                  const qint32 inCount,
                                  bool inTranspose)
{
    if (inConstantBase == nullptr) {
        Q_ASSERT(false);
        return;
    }

    Q_ASSERT(inConstantBase->m_elementCount >= inCount);

    if (inConstantBase->getShaderConstantType() == QSSGDataTypeToShaderDataTypeMap<TDataType>::getType()) {
        QSSGRenderShaderConstant<TDataType> *inConstant = static_cast<QSSGRenderShaderConstant<TDataType> *>(inConstantBase);
        ShaderConstantApplier<TDataType>().applyConstant(program,
                                                         inConstant->m_location,
                                                         inCount,
                                                         inConstant->m_type,
                                                         inValue,
                                                         inConstant->m_value,
                                                         inTranspose);
    } else {
        Q_ASSERT(false);
    }
}

template<typename TDataType>
void setMatrixConstantValueOfType(const QSSGRenderShaderProgram *program,
                                  QSSGRenderShaderConstantBase *inConstantBase,
                                  const QSSGDataView<TDataType> inValue,
                                  const qint32 /*inCount*/,
                                  bool inTranspose)
{
    if (inConstantBase == nullptr) {
        Q_ASSERT(false);
        return;
    }

    Q_ASSERT(inConstantBase->m_elementCount >= (qint32)inValue.size());

    if (inConstantBase->getShaderConstantType() == QSSGDataTypeToShaderDataTypeMap<TDataType>::getType()) {
        QSSGRenderShaderConstant<TDataType> *inConstant = static_cast<QSSGRenderShaderConstant<TDataType> *>(inConstantBase);
        ShaderConstantApplier<TDataType>().applyConstant(program,
                                                         inConstant->m_location,
                                                         inValue.size(),
                                                         inConstant->m_type,
                                                         inValue,
                                                         inConstant->m_value,
                                                         inTranspose);
    } else {
        Q_ASSERT(false);
    }
}

void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, qint32 inValue, const qint32 inCount)
{
    setConstantValueOfType(this, inConstant, inValue, inCount);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, const qint32_2 &inValue, const qint32 inCount)
{
    setConstantValueOfType(this, inConstant, inValue, inCount);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, const qint32_3 &inValue, const qint32 inCount)
{
    setConstantValueOfType(this, inConstant, inValue, inCount);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, const qint32_4 &inValue, const qint32 inCount)
{
    setConstantValueOfType(this, inConstant, inValue, inCount);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, bool inValue, const qint32 inCount)
{
    setConstantValueOfType(this, inConstant, inValue, inCount);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, const bool_2 &inValue, const qint32 inCount)
{
    setConstantValueOfType(this, inConstant, inValue, inCount);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, const bool_3 &inValue, const qint32 inCount)
{
    setConstantValueOfType(this, inConstant, inValue, inCount);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, const bool_4 &inValue, const qint32 inCount)
{
    setConstantValueOfType(this, inConstant, inValue, inCount);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, const float &inValue, const qint32 inCount)
{
    setConstantValueOfType(this, inConstant, inValue, inCount);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, const QVector2D &inValue, const qint32 inCount)
{
    setConstantValueOfType(this, inConstant, inValue, inCount);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, const QVector3D &inValue, const qint32 inCount)
{
    setConstantValueOfType(this, inConstant, inValue, inCount);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, const QVector4D &inValue, const qint32 inCount)
{
    setConstantValueOfType(this, inConstant, inValue, inCount);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, const QColor &inValue, const qint32 inCount)
{
    QVector4D value(float(inValue.redF()), float(inValue.greenF()), float(inValue.blueF()), float(inValue.alphaF()));
    setConstantValueOfType(this, inConstant, value, inCount);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, const quint32 &inValue, const qint32 inCount)
{
    setConstantValueOfType(this, inConstant, inValue, inCount);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, const quint32_2 &inValue, const qint32 inCount)
{
    setConstantValueOfType(this, inConstant, inValue, inCount);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, const quint32_3 &inValue, const qint32 inCount)
{
    setConstantValueOfType(this, inConstant, inValue, inCount);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, const quint32_4 &inValue, const qint32 inCount)
{
    setConstantValueOfType(this, inConstant, inValue, inCount);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant,
                                                 const QMatrix3x3 &inValue,
                                                 const qint32 inCount,
                                                 bool inTranspose)
{
    setMatrixConstantValueOfType(this, inConstant, inValue, inCount, inTranspose);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant,
                                                 const QMatrix4x4 &inValue,
                                                 const qint32 inCount,
                                                 bool inTranspose)
{
    setMatrixConstantValueOfType(this, inConstant, inValue, inCount, inTranspose);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant,
                                                 const QSSGDataView<QMatrix4x4> inValue,
                                                 const qint32 inCount)
{
    setMatrixConstantValueOfType(this, inConstant, inValue, inCount, false);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, QSSGRenderTexture2D *inValue, const qint32 inCount)
{
    Q_UNUSED(inCount)
    setConstantValueOfType(this, inConstant, inValue, 1);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, QSSGRenderTexture2D **inValue, const qint32 inCount)
{
    setConstantValueOfType(this, inConstant, inValue, inCount);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, QSSGRenderTextureCube *inValue, const qint32 inCount)
{
    Q_UNUSED(inCount)
    setConstantValueOfType(this, inConstant, inValue, 1);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant,
                                                 QSSGRenderTextureCube **inValue,
                                                 const qint32 inCount)
{
    setConstantValueOfType(this, inConstant, inValue, inCount);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *inConstant, QSSGRenderImage2D *inValue, const qint32 inCount)
{
    setSamplerConstantValueOfType(this, inConstant, inValue, inCount);
}
void QSSGRenderShaderProgram::setConstantValue(QSSGRenderShaderConstantBase *, QSSGRenderDataBuffer *, const qint32)
{
    // this is merely a dummy right now
}

void QSSGRenderShaderProgram::bindComputeInput(QSSGRenderDataBuffer *inBuffer, quint32 inIndex)
{
    QSSGRenderBackend::QSSGRenderBackendBufferObject obj(nullptr);
    if (inBuffer)
        obj = inBuffer->handle();
    m_backend->programSetStorageBuffer(inIndex, obj);
}

namespace {
void writeErrorMessage(const char *tag, const QByteArray &message)
{
    const auto lines = message.split('\n');
    for (const auto &line : lines)
        qCCritical(RENDER_INVALID_OPERATION, "%s: %s", tag, line.constData());
}
}

QSSGRenderVertFragCompilationResult QSSGRenderShaderProgram::create(const QSSGRef<QSSGRenderContext> &context,
                                                                        const char *programName,
                                                                        QSSGByteView vertShaderSource,
                                                                        QSSGByteView fragShaderSource,
                                                                        QSSGByteView tessControlShaderSource,
                                                                        QSSGByteView tessEvaluationShaderSource,
                                                                        QSSGByteView geometryShaderSource,
                                                                        bool separateProgram,
                                                                        QSSGRenderShaderProgramBinaryType type,
                                                                        bool binaryProgram)
{
    QSSGRenderVertFragCompilationResult result;
    result.m_shaderName = programName;

    // our minimum requirement is a vertex and a fragment shader or geometry shader
    // if we should treat it as a separate program we don't care
    if (!separateProgram && (vertShaderSource.size() == 0 || (fragShaderSource.size() == 0 && geometryShaderSource.size() == 0))) {
        qCCritical(RENDER_INVALID_PARAMETER, "Vertex or fragment (geometry) source have 0 length");
        Q_ASSERT(false);
        return result;
    }

    if (binaryProgram && type != QSSGRenderShaderProgramBinaryType::NVBinary) {
        qCCritical(RENDER_INVALID_PARAMETER, "Unrecoginzed binary format");
        Q_ASSERT(false);
        return result;
    }

    const QSSGRef<QSSGRenderBackend> &backend = context->backend();

    // first create and compile shaders
    QSSGRenderBackend::QSSGRenderBackendVertexShaderObject vtxShader = nullptr;
    if (vertShaderSource.size()) {
        QByteArray errorMessage;
        vtxShader = backend->createVertexShader(vertShaderSource, errorMessage, binaryProgram);
        if (!vtxShader) {
            qCCritical(RENDER_INTERNAL_ERROR, "Failed to generate vertex shader!!");
            qCCritical(RENDER_INTERNAL_ERROR, "Vertex source:\n%s", nonNull((const char *)vertShaderSource.begin()));
            writeErrorMessage("Vertex compilation output:", errorMessage);
            return result;
        }
    }
    QSSGRenderBackend::QSSGRenderBackendFragmentShaderObject fragShader = nullptr;
    if (fragShaderSource.size()) {
        QByteArray errorMessage;
        fragShader = backend->createFragmentShader(fragShaderSource, errorMessage, binaryProgram);
        if (!fragShader) {
            qCCritical(RENDER_INTERNAL_ERROR, "Failed to generate fragment shader!!");
            qCCritical(RENDER_INTERNAL_ERROR, "Fragment source:\n%s", nonNull((const char *)fragShaderSource.begin()));
            writeErrorMessage("Fragment compilation output:", errorMessage);
            return result;
        }
    }
    QSSGRenderBackend::QSSGRenderBackendTessControlShaderObject tcShader = nullptr;
    if (tessControlShaderSource.size()) {
        QByteArray errorMessage;
        tcShader = backend->createTessControlShader(tessControlShaderSource, errorMessage, binaryProgram);
        if (!tcShader) {
            qCCritical(RENDER_INTERNAL_ERROR, "Failed to generate tessellation control shader!!");
            qCCritical(RENDER_INTERNAL_ERROR, "Tessellation control source:\n%s", nonNull((const char *)tessControlShaderSource.begin()));
            writeErrorMessage("Tessellation control compilation output:", errorMessage);
            return result;
        }
    }
    QSSGRenderBackend::QSSGRenderBackendTessEvaluationShaderObject teShader = nullptr;
    if (tessEvaluationShaderSource.size()) {
        QByteArray errorMessage;
        teShader = backend->createTessEvaluationShader(tessEvaluationShaderSource, errorMessage, binaryProgram);
        if (!teShader) {
            qCCritical(RENDER_INTERNAL_ERROR, "Failed to generate tessellation evaluation shader!!");
            qCCritical(RENDER_INTERNAL_ERROR,
                       "Tessellation evaluation source:\n%s",
                       nonNull((const char *)tessEvaluationShaderSource.begin()));
            writeErrorMessage("Tessellation evaluation compilation output:", errorMessage);
            return result;
        }
    }
    QSSGRenderBackend::QSSGRenderBackendGeometryShaderObject geShader = nullptr;
    if (geometryShaderSource.size()) {
        QByteArray errorMessage;
        geShader = backend->createGeometryShader(geometryShaderSource, errorMessage, binaryProgram);
        if (!geShader) {
            qCCritical(RENDER_INTERNAL_ERROR, "Failed to generate geometry shader!!");
            qCCritical(RENDER_INTERNAL_ERROR, "Geometry source:\n%s", nonNull((const char *)geometryShaderSource.begin()));
            writeErrorMessage("Geometry compilation output:", errorMessage);
            return result;
        }
    }

    // shaders were succesfully created
    result.m_shader = new QSSGRenderShaderProgram(context, programName, separateProgram);

    static const bool dumpShader = (qEnvironmentVariableIntValue("QT_QUICK3D_DUMP_SHADERS") > 0);
    if (dumpShader) {
        qCInfo(RENDER_SHADER_INFO, "Vertex source:\n%s", nonNull((const char *)vertShaderSource.begin()));
        qCInfo(RENDER_SHADER_INFO, "Fragment source:\n%s", nonNull((const char *)fragShaderSource.begin()));
    }

    // attach programs
    if (vtxShader)
        result.m_shader->attach(vtxShader);
    if (fragShader)
        result.m_shader->attach(fragShader);
    if (tcShader)
        result.m_shader->attach(tcShader);
    if (teShader)
        result.m_shader->attach(teShader);
    if (geShader)
        result.m_shader->attach(geShader);

    // link program
    if (!result.m_shader->link()) {
        qCCritical(RENDER_INTERNAL_ERROR, "Failed to link program!!");
        writeErrorMessage("Program link output:", result.m_shader->errorMessage());

        // delete program
        result.m_shader = nullptr;
    } else {
        // clean up
        if (vtxShader)
            result.m_shader->detach(vtxShader);
        if (fragShader)
            result.m_shader->detach(fragShader);
        if (tcShader)
            result.m_shader->detach(tcShader);
        if (teShader)
            result.m_shader->detach(teShader);
        if (geShader)
            result.m_shader->detach(geShader);
    }

    backend->releaseVertexShader(vtxShader);
    backend->releaseFragmentShader(fragShader);
    backend->releaseTessControlShader(tcShader);
    backend->releaseTessEvaluationShader(teShader);
    backend->releaseGeometryShader(geShader);

    return result;
}

QSSGRenderVertFragCompilationResult QSSGRenderShaderProgram::create(
            const QSSGRef<QSSGRenderContext> &context, const char *programName,
            quint32 format, const QByteArray &binary)
{
    QSSGRenderVertFragCompilationResult result;
    result.m_shaderName = programName;
    result.m_shader = new QSSGRenderShaderProgram(context, programName, false);
    result.m_shader->link(format, binary);
    return result;
}

QSSGRenderVertFragCompilationResult QSSGRenderShaderProgram::createCompute(const QSSGRef<QSSGRenderContext> &context,
                                                                               const char *programName,
                                                                               QSSGByteView computeShaderSource)
{
    QSSGRenderVertFragCompilationResult result;
    QSSGRef<QSSGRenderShaderProgram> pProgram;
    bool bProgramIsValid = true;

    result.m_shaderName = programName;

    // check source
    if (computeShaderSource.size() == 0) {
        qCCritical(RENDER_INVALID_PARAMETER, "compute source has 0 length");
        Q_ASSERT(false);
        return result;
    }

    const auto &backend = context->backend();
    QByteArray errorMessage;
    QSSGRenderBackend::QSSGRenderBackendComputeShaderObject computeShader =
        backend->createComputeShader(computeShaderSource, errorMessage, false);

    if (computeShader) {
        // shaders were successfully created
        pProgram = new QSSGRenderShaderProgram(context, programName, false);

        if (pProgram) {
            // attach programs
            pProgram->attach(computeShader);

            // link program
            bProgramIsValid = pProgram->link();

            // set program type
            pProgram->m_programType = ProgramType::Compute;
        }
    }

    // if anything went wrong print out
    if (!computeShader || !bProgramIsValid) {
        if (!computeShader) {
            qCCritical(RENDER_INTERNAL_ERROR, "Failed to generate compute shader!!");
            qCCritical(RENDER_INTERNAL_ERROR, "Shader source:\n%s", nonNull((const char *)computeShaderSource.begin()));
            writeErrorMessage("Compute shader compilation output:", errorMessage);
        }
    }

    // set program
    result.m_shader = pProgram;

    return result;
}

QSSGRenderVertFragCompilationResult::QSSGRenderVertFragCompilationResult() = default;

QSSGRenderVertFragCompilationResult::~QSSGRenderVertFragCompilationResult() = default;

QSSGRenderVertFragCompilationResult::QSSGRenderVertFragCompilationResult(const QSSGRenderVertFragCompilationResult &other) = default;

QSSGRenderVertFragCompilationResult &QSSGRenderVertFragCompilationResult::operator=(const QSSGRenderVertFragCompilationResult &other) = default;

QT_END_NAMESPACE
