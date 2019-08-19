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

#include <QtQuick3DRender/private/qssgrenderbackendgl4_p.h>
#include <QtQuick3DRender/private/qssgrenderbackendinputassemblergl_p.h>
#include <QtQuick3DRender/private/qssgrenderbackendshaderprogramgl_p.h>

QT_BEGIN_NAMESPACE

#ifdef RENDER_BACKEND_LOG_GL_ERRORS
#define RENDER_LOG_ERROR_PARAMS(x) checkGLError(#x, __FILE__, __LINE__)
#else
#define RENDER_LOG_ERROR_PARAMS(x) checkGLError()
#endif

#define GL_CALL_EXTRA_FUNCTION(x)                                                                                      \
    m_glExtraFunctions->x;                                                                                             \
    RENDER_LOG_ERROR_PARAMS(x);

#if defined(QT_OPENGL_ES)
#define GL_CALL_NVPATH_EXT(x)                                                                                          \
    m_QSSGExtensions->x;                                                                                             \
    RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_QSSG_EXT(x)                                                                                          \
    m_QSSGExtensions->x;                                                                                             \
    RENDER_LOG_ERROR_PARAMS(x);
#else
#define GL_CALL_NVPATH_EXT(x)                                                                                          \
    m_nvPathRendering->x;                                                                                              \
    RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_DIRECTSTATE_EXT(x)                                                                                     \
    m_directStateAccess->x;                                                                                            \
    RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_QSSG_EXT(x)                                                                                          \
    m_QSSGExtensions->x;                                                                                             \
    RENDER_LOG_ERROR_PARAMS(x);
#endif

#ifndef GL_GEOMETRY_SHADER_EXT
#define GL_GEOMETRY_SHADER_EXT 0x8DD9
#endif

namespace QSSGGlExtStrings {
QByteArray extTess()
{
    return QByteArrayLiteral("GL_ARB_tessellation_shader");
}
QByteArray extGeometry()
{
    return QByteArrayLiteral("GL_EXT_geometry_shader4");
}
QByteArray arbCompute()
{
    return QByteArrayLiteral("GL_ARB_compute_shader");
}
QByteArray arbStorageBuffer()
{
    return QByteArrayLiteral("GL_ARB_shader_storage_buffer_object");
}
QByteArray arbAtomicCounterBuffer()
{
    return QByteArrayLiteral("GL_ARB_shader_atomic_counters");
}
QByteArray arbProgInterface()
{
    return QByteArrayLiteral("GL_ARB_program_interface_query");
}
QByteArray arbShaderImageLoadStore()
{
    return QByteArrayLiteral("GL_ARB_shader_image_load_store");
}
QByteArray nvPathRendering()
{
    return QByteArrayLiteral("GL_NV_path_rendering");
}
QByteArray nvBlendAdvanced()
{
    return QByteArrayLiteral("GL_NV_blend_equation_advanced");
}
QByteArray khrBlendAdvanced()
{
    return QByteArrayLiteral("GL_KHR_blend_equation_advanced");
}
QByteArray nvBlendAdvancedCoherent()
{
    return QByteArrayLiteral("GL_NV_blend_equation_advanced_coherent");
}
QByteArray khrBlendAdvancedCoherent()
{
    return QByteArrayLiteral("GL_KHR_blend_equation_advanced_coherent");
}
}

/// constructor
QSSGRenderBackendGL4Impl::QSSGRenderBackendGL4Impl(const QSurfaceFormat &format)
    : QSSGRenderBackendGL3Impl(format)
{
    const QByteArray apiVersion(getVersionString());
    qCInfo(TRACE_INFO, "GL version: %s", apiVersion.constData());

    // get extension count
    GLint numExtensions = 0;
    GL_CALL_EXTRA_FUNCTION(glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions));

    for (qint32 i = 0; i < numExtensions; i++) {
        const GLubyte *glExt = GL_CALL_EXTRA_FUNCTION(glGetStringi(GL_EXTENSIONS, GLuint(i)));
        const QByteArray extensionString(reinterpret_cast<const char *>(glExt));

        // search for extension
        if (!m_backendSupport.caps.bits.bTessellationSupported && QSSGGlExtStrings::extTess().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bTessellationSupported = true;
        } else if (!m_backendSupport.caps.bits.bComputeSupported && QSSGGlExtStrings::arbCompute().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bComputeSupported = true;
        } else if (!m_backendSupport.caps.bits.bGeometrySupported
                   && QSSGGlExtStrings::extGeometry().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bGeometrySupported = true;
        } else if (!m_backendSupport.caps.bits.bStorageBufferSupported
                   && QSSGGlExtStrings::arbStorageBuffer().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bStorageBufferSupported = true;
        } else if (!m_backendSupport.caps.bits.bAtomicCounterBufferSupported
                   && QSSGGlExtStrings::arbAtomicCounterBuffer().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bAtomicCounterBufferSupported = true;
        } else if (!m_backendSupport.caps.bits.bProgramInterfaceSupported
                   && QSSGGlExtStrings::arbProgInterface().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bProgramInterfaceSupported = true;
        } else if (!m_backendSupport.caps.bits.bShaderImageLoadStoreSupported
                   && QSSGGlExtStrings::arbShaderImageLoadStore().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bShaderImageLoadStoreSupported = true;
        } else if (!m_backendSupport.caps.bits.bNVPathRenderingSupported
                   && QSSGGlExtStrings::nvPathRendering().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bNVPathRenderingSupported = true;
        } else if (!m_backendSupport.caps.bits.bNVAdvancedBlendSupported
                   && QSSGGlExtStrings::nvBlendAdvanced().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bNVAdvancedBlendSupported = true;
        } else if (!m_backendSupport.caps.bits.bNVBlendCoherenceSupported
                   && QSSGGlExtStrings::nvBlendAdvancedCoherent().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bNVBlendCoherenceSupported = true;
        } else if (!m_backendSupport.caps.bits.bKHRAdvancedBlendSupported
                   && QSSGGlExtStrings::khrBlendAdvanced().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bKHRAdvancedBlendSupported = true;
        } else if (!m_backendSupport.caps.bits.bKHRBlendCoherenceSupported
                   && QSSGGlExtStrings::khrBlendAdvancedCoherent().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bKHRBlendCoherenceSupported = true;
        }
    }

    // always true for GL4.1 and GLES 3.1 devices
    m_backendSupport.caps.bits.bMsTextureSupported = true;
    m_backendSupport.caps.bits.bProgramPipelineSupported = true;

    if (!isESCompatible()) {
        // TODO: investigate GL 4.0 support
        // we expect minimum GL 4.1 context anything beyond is handeled via extensions
        // Tessellation is always supported on none ES systems which support >=GL4
        m_backendSupport.caps.bits.bTessellationSupported = true;
        // geometry shader is always supported on none ES systems which support >=GL4 ( actually
        // 3.2 already )
        m_backendSupport.caps.bits.bGeometrySupported = true;
    } else {
        // always true for GLES 3.1 devices
        m_backendSupport.caps.bits.bComputeSupported = true;
        m_backendSupport.caps.bits.bProgramInterfaceSupported = true;
        m_backendSupport.caps.bits.bStorageBufferSupported = true;
        m_backendSupport.caps.bits.bAtomicCounterBufferSupported = true;
        m_backendSupport.caps.bits.bShaderImageLoadStoreSupported = true;
    }

#if !defined(QT_OPENGL_ES)
    // Initialize extensions
    m_nvPathRendering = new QOpenGLExtension_NV_path_rendering();
    m_nvPathRendering->initializeOpenGLFunctions();
    m_directStateAccess = new QOpenGLExtension_EXT_direct_state_access();
    m_directStateAccess->initializeOpenGLFunctions();
#endif
}

/// destructor
QSSGRenderBackendGL4Impl::~QSSGRenderBackendGL4Impl()
{
#if !defined(QT_OPENGL_ES)
    delete m_nvPathRendering;
    delete m_directStateAccess;
#endif
}

void QSSGRenderBackendGL4Impl::drawIndirect(QSSGRenderDrawMode drawMode, const void *indirect)
{
    GL_CALL_EXTRA_FUNCTION(glDrawArraysIndirect(m_conversion.fromDrawModeToGL(drawMode, m_backendSupport.caps.bits.bTessellationSupported),
                                                indirect));
}

void QSSGRenderBackendGL4Impl::drawIndexedIndirect(QSSGRenderDrawMode drawMode,
                                                     QSSGRenderComponentType type,
                                                     const void *indirect)
{
    GL_CALL_EXTRA_FUNCTION(glDrawElementsIndirect(m_conversion.fromDrawModeToGL(drawMode, m_backendSupport.caps.bits.bTessellationSupported),
                                                  m_conversion.fromIndexBufferComponentsTypesToGL(type),
                                                  indirect));
}

void QSSGRenderBackendGL4Impl::createTextureStorage2D(QSSGRenderBackendTextureObject to,
                                                        QSSGRenderTextureTargetType target,
                                                        qint32 levels,
                                                        QSSGRenderTextureFormat internalFormat,
                                                        qint32 width,
                                                        qint32 height)
{
    GLuint texID = HandleToID_cast(GLuint, quintptr, to);
    GLenum glTarget = GLConversion::fromTextureTargetToGL(target);
    GL_CALL_EXTRA_FUNCTION(glActiveTexture(GL_TEXTURE0));
    GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, texID));

    // up to now compressed is not supported
    Q_ASSERT(internalFormat.isUncompressedTextureFormat());

    GLenum glformat = 0, glInternalFormat = 0, gltype = GL_UNSIGNED_BYTE;
    GLConversion::fromUncompressedTextureFormatToGL(getRenderContextType(), internalFormat, glformat, gltype, glInternalFormat);

    GL_CALL_EXTRA_FUNCTION(glTexStorage2D(glTarget, levels, glInternalFormat, GLsizei(width), GLsizei(height)));

    GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, 0));
}

void QSSGRenderBackendGL4Impl::setMultisampledTextureData2D(QSSGRenderBackendTextureObject to,
                                                              QSSGRenderTextureTargetType target,
                                                              qint32 samples,
                                                              QSSGRenderTextureFormat internalFormat,
                                                              qint32 width,
                                                              qint32 height,
                                                              bool fixedsamplelocations)
{
    GLuint texID = HandleToID_cast(GLuint, quintptr, to);
    GLenum glTarget = GLConversion::fromTextureTargetToGL(target);
    GL_CALL_EXTRA_FUNCTION(glActiveTexture(GL_TEXTURE0));
    GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, texID));

    QSSGRenderTextureSwizzleMode swizzleMode = QSSGRenderTextureSwizzleMode::NoSwizzle;
    internalFormat = GLConversion::replaceDeprecatedTextureFormat(getRenderContextType(), internalFormat, swizzleMode);

    GLenum glformat = 0, glInternalFormat = 0, gltype = GL_UNSIGNED_BYTE;

    if (internalFormat.isUncompressedTextureFormat())
        GLConversion::fromUncompressedTextureFormatToGL(getRenderContextType(), internalFormat, glformat, gltype, glInternalFormat);
    else if (internalFormat.isDepthTextureFormat())
        GLConversion::fromDepthTextureFormatToGL(getRenderContextType(), internalFormat, glformat, gltype, glInternalFormat);
    GL_CALL_EXTRA_FUNCTION(
            glTexStorage2DMultisample(glTarget, GLsizei(samples), glInternalFormat, GLsizei(width), GLsizei(height), fixedsamplelocations));

    GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, 0));
}

QSSGRenderBackend::QSSGRenderBackendTessControlShaderObject QSSGRenderBackendGL4Impl::createTessControlShader(
        QSSGByteView source,
        QByteArray &errorMessage,
        bool binary)
{
#if !defined(QT_OPENGL_ES)
    GLuint shaderID = GL_CALL_EXTRA_FUNCTION(glCreateShader(GL_TESS_CONTROL_SHADER));
#else
    GLuint shaderID = 0;
#endif
    if (shaderID && !compileSource(shaderID, source, errorMessage, binary)) {
        GL_CALL_EXTRA_FUNCTION(glDeleteShader(shaderID));
        shaderID = 0;
    }

    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendTessControlShaderObject>(quintptr(shaderID));
}

QSSGRenderBackend::QSSGRenderBackendTessEvaluationShaderObject QSSGRenderBackendGL4Impl::createTessEvaluationShader(
        QSSGByteView source,
        QByteArray &errorMessage,
        bool binary)
{
#if !defined(QT_OPENGL_ES)
    GLuint shaderID = GL_CALL_EXTRA_FUNCTION(glCreateShader(GL_TESS_EVALUATION_SHADER));
#else
    GLuint shaderID = 0;
#endif

    if (shaderID && !compileSource(shaderID, source, errorMessage, binary)) {
        GL_CALL_EXTRA_FUNCTION(glDeleteShader(shaderID));
        shaderID = 0;
    }

    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendTessEvaluationShaderObject>(quintptr(shaderID));
}

QSSGRenderBackend::QSSGRenderBackendGeometryShaderObject QSSGRenderBackendGL4Impl::createGeometryShader(QSSGByteView source,
                                                                                                              QByteArray &errorMessage,
                                                                                                              bool binary)
{
#if defined(QT_OPENGL_ES)
    GLuint shaderID = GL_CALL_EXTRA_FUNCTION(glCreateShader(GL_GEOMETRY_SHADER_EXT));
#else
    GLuint shaderID = GL_CALL_EXTRA_FUNCTION(glCreateShader(GL_GEOMETRY_SHADER));
#endif
    if (shaderID && !compileSource(shaderID, source, errorMessage, binary)) {
        GL_CALL_EXTRA_FUNCTION(glDeleteShader(shaderID));
        shaderID = 0;
    }

    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendGeometryShaderObject>(quintptr(shaderID));
}

void QSSGRenderBackendGL4Impl::setPatchVertexCount(QSSGRenderBackendInputAssemblerObject iao, quint32 count)
{
    Q_ASSERT(iao);
    Q_ASSERT(count);
    QSSGRenderBackendInputAssemblerGL *inputAssembler = reinterpret_cast<QSSGRenderBackendInputAssemblerGL *>(iao);
    inputAssembler->m_patchVertexCount = count;
}

void QSSGRenderBackendGL4Impl::setMemoryBarrier(QSSGRenderBufferBarrierFlags barriers)
{
    GL_CALL_EXTRA_FUNCTION(glMemoryBarrier(m_conversion.fromMemoryBarrierFlagsToGL(barriers)));
}

void QSSGRenderBackendGL4Impl::bindImageTexture(QSSGRenderBackendTextureObject to,
                                                  quint32 unit,
                                                  qint32 level,
                                                  bool layered,
                                                  qint32 layer,
                                                  QSSGRenderImageAccessType access,
                                                  QSSGRenderTextureFormat format)
{
    GLuint texID = HandleToID_cast(GLuint, quintptr, to);

    GL_CALL_EXTRA_FUNCTION(glBindImageTexture(unit,
                                              texID,
                                              level,
                                              layered,
                                              layer,
                                              m_conversion.fromImageAccessToGL(access),
                                              m_conversion.fromImageFormatToGL(format)));
}

qint32 QSSGRenderBackendGL4Impl::getStorageBufferCount(QSSGRenderBackendShaderProgramObject po)
{
    GLint numStorageBuffers = 0;

    // The static, compile time condition is not ideal (it all should be run
    // time checks), but will be replaced in the future anyway.
#if defined(GL_VERSION_4_3) || defined(QT_OPENGL_ES_3_1)
    Q_ASSERT(po);
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);
    if (m_backendSupport.caps.bits.bProgramInterfaceSupported)
        GL_CALL_EXTRA_FUNCTION(glGetProgramInterfaceiv(programID, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &numStorageBuffers));
#else
    Q_UNUSED(po);
#endif
    return numStorageBuffers;
}

qint32 QSSGRenderBackendGL4Impl::getStorageBufferInfoByID(QSSGRenderBackendShaderProgramObject po,
                                                            quint32 id,
                                                            quint32 nameBufSize,
                                                            qint32 *paramCount,
                                                            qint32 *bufferSize,
                                                            qint32 *length,
                                                            char *nameBuf)
{
    GLint bufferIndex = GL_INVALID_INDEX;

    // The static, compile time condition is not ideal (it all should be run
    // time checks), but will be replaced in the future anyway.
#if defined(GL_VERSION_4_3) || defined(QT_OPENGL_ES_3_1)
    Q_ASSERT(po);
    Q_ASSERT(length);
    Q_ASSERT(nameBuf);
    Q_ASSERT(bufferSize);
    Q_ASSERT(paramCount);
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);
    if (m_backendSupport.caps.bits.bProgramInterfaceSupported) {
        GL_CALL_EXTRA_FUNCTION(glGetProgramResourceName(programID, GL_SHADER_STORAGE_BLOCK, id, nameBufSize, length, nameBuf));

        if (*length > 0) {
#define QUERY_COUNT 3
            GLsizei actualCount;
            GLenum props[QUERY_COUNT] = { GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE, GL_NUM_ACTIVE_VARIABLES };
            GLint params[QUERY_COUNT];
            GL_CALL_EXTRA_FUNCTION(
                    glGetProgramResourceiv(programID, GL_SHADER_STORAGE_BLOCK, id, QUERY_COUNT, props, QUERY_COUNT, &actualCount, params));

            Q_ASSERT(actualCount == QUERY_COUNT);

            bufferIndex = params[0];
            *bufferSize = params[1];
            *paramCount = params[2];
        }
    }
#else
    Q_UNUSED(po);
    Q_UNUSED(id);
    Q_UNUSED(nameBufSize);
    Q_UNUSED(paramCount);
    Q_UNUSED(bufferSize);
    Q_UNUSED(length);
    Q_UNUSED(nameBuf);
#endif
    return bufferIndex;
}

void QSSGRenderBackendGL4Impl::programSetStorageBuffer(quint32 index, QSSGRenderBackendBufferObject bo)
{
    // The static, compile time condition is not ideal (it all should be run
    // time checks), but will be replaced in the future anyway.
#if defined(GL_VERSION_4_3) || defined(QT_OPENGL_ES_3_1)
    GL_CALL_EXTRA_FUNCTION(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, HandleToID_cast(GLuint, quintptr, bo)));
#else
    Q_UNUSED(index);
    Q_UNUSED(bo);
#endif
}

qint32 QSSGRenderBackendGL4Impl::getAtomicCounterBufferCount(QSSGRenderBackendShaderProgramObject po)
{
    GLint numAtomicCounterBuffers = 0;
    // The static, compile time condition is not ideal (it all should be run
    // time checks), but will be replaced in the future anyway.
#if defined(GL_VERSION_4_3) || defined(QT_OPENGL_ES_3_1)
    Q_ASSERT(po);
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);
    if (m_backendSupport.caps.bits.bProgramInterfaceSupported)
        GL_CALL_EXTRA_FUNCTION(glGetProgramInterfaceiv(programID, GL_ATOMIC_COUNTER_BUFFER, GL_ACTIVE_RESOURCES, &numAtomicCounterBuffers));
#else
    Q_UNUSED(po);
#endif
    return numAtomicCounterBuffers;
}

qint32 QSSGRenderBackendGL4Impl::getAtomicCounterBufferInfoByID(QSSGRenderBackendShaderProgramObject po,
                                                                  quint32 id,
                                                                  quint32 nameBufSize,
                                                                  qint32 *paramCount,
                                                                  qint32 *bufferSize,
                                                                  qint32 *length,
                                                                  char *nameBuf)
{
    GLint bufferIndex = GL_INVALID_INDEX;

    // The static, compile time condition is not ideal (it all should be run
    // time checks), but will be replaced in the future anyway.
#if defined(GL_VERSION_4_3) || defined(QT_OPENGL_ES_3_1)
    Q_ASSERT(po);
    Q_ASSERT(length);
    Q_ASSERT(nameBuf);
    Q_ASSERT(bufferSize);
    Q_ASSERT(paramCount);
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);
    if (m_backendSupport.caps.bits.bProgramInterfaceSupported) {
        {
#define QUERY_COUNT 3
            GLsizei actualCount;
            GLenum props[QUERY_COUNT] = { GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE, GL_NUM_ACTIVE_VARIABLES };
            GLint params[QUERY_COUNT];
            GL_CALL_EXTRA_FUNCTION(
                    glGetProgramResourceiv(programID, GL_ATOMIC_COUNTER_BUFFER, id, QUERY_COUNT, props, QUERY_COUNT, &actualCount, params));

            Q_ASSERT(actualCount == QUERY_COUNT);

            bufferIndex = params[0];
            *bufferSize = params[1];
            *paramCount = params[2];

            GLenum props1[1] = { GL_ATOMIC_COUNTER_BUFFER_INDEX };
            GL_CALL_EXTRA_FUNCTION(glGetProgramResourceiv(programID, GL_UNIFORM, id, 1, props1, 1, &actualCount, params));

            Q_ASSERT(actualCount == 1);

            *nameBuf = '\0';
            GL_CALL_EXTRA_FUNCTION(glGetProgramResourceName(programID, GL_UNIFORM, params[0], nameBufSize, length, nameBuf));
        }
    }
#else
    Q_UNUSED(po);
    Q_UNUSED(id);
    Q_UNUSED(nameBufSize);
    Q_UNUSED(paramCount);
    Q_UNUSED(bufferSize);
    Q_UNUSED(length);
    Q_UNUSED(nameBuf);
#endif
    return bufferIndex;
}

void QSSGRenderBackendGL4Impl::programSetAtomicCounterBuffer(quint32 index, QSSGRenderBackendBufferObject bo)
{
    // The static, compile time condition is not ideal (it all should be run
    // time checks), but will be replaced in the future anyway.
#if defined(GL_VERSION_4_3) || defined(QT_OPENGL_ES_3_1)
    GL_CALL_EXTRA_FUNCTION(glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, index, HandleToID_cast(GLuint, quintptr, bo)));
#else
    Q_UNUSED(index)
    Q_UNUSED(bo)
#endif
}

void QSSGRenderBackendGL4Impl::setConstantValue(QSSGRenderBackendShaderProgramObject po,
                                                  quint32 id,
                                                  QSSGRenderShaderDataType type,
                                                  qint32 count,
                                                  const void *value,
                                                  bool transpose)
{
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);

    GLenum glType = GLConversion::fromPropertyDataTypesToShaderGL(type);

    switch (glType) {
    case GL_FLOAT:
        GL_CALL_EXTRA_FUNCTION(glProgramUniform1fv(programID, GLint(id), count, static_cast<const GLfloat *>(value)));
        break;
    case GL_FLOAT_VEC2:
        GL_CALL_EXTRA_FUNCTION(glProgramUniform2fv(programID, GLint(id), count, static_cast<const GLfloat *>(value)));
        break;
    case GL_FLOAT_VEC3:
        GL_CALL_EXTRA_FUNCTION(glProgramUniform3fv(programID, GLint(id), count, static_cast<const GLfloat *>(value)));
        break;
    case GL_FLOAT_VEC4:
        GL_CALL_EXTRA_FUNCTION(glProgramUniform4fv(programID, GLint(id), count, static_cast<const GLfloat *>(value)));
        break;
    case GL_INT:
        GL_CALL_EXTRA_FUNCTION(glProgramUniform1iv(programID, GLint(id), count, static_cast<const GLint *>(value)));
        break;
    case GL_BOOL: {
        // Cast int value to be 0 or 1, matching to bool
        GLint *boolValue = (GLint *)value;
        *boolValue = *(GLboolean *)value;
        GL_CALL_EXTRA_FUNCTION(glProgramUniform1iv(programID, GLint(id), count, boolValue));
    } break;
    case GL_INT_VEC2:
    case GL_BOOL_VEC2:
        GL_CALL_EXTRA_FUNCTION(glProgramUniform2iv(programID, GLint(id), count, static_cast<const GLint *>(value)));
        break;
    case GL_INT_VEC3:
    case GL_BOOL_VEC3:
        GL_CALL_EXTRA_FUNCTION(glProgramUniform3iv(programID, GLint(id), count, static_cast<const GLint *>(value)));
        break;
    case GL_INT_VEC4:
    case GL_BOOL_VEC4:
        GL_CALL_EXTRA_FUNCTION(glProgramUniform4iv(programID, GLint(id), count, static_cast<const GLint *>(value)));
        break;
    case GL_FLOAT_MAT3:
        GL_CALL_EXTRA_FUNCTION(glProgramUniformMatrix3fv(programID, GLint(id), count, transpose, static_cast<const GLfloat *>(value)));
        break;
    case GL_FLOAT_MAT4:
        GL_CALL_EXTRA_FUNCTION(glProgramUniformMatrix4fv(programID, GLint(id), count, transpose, static_cast<const GLfloat *>(value)));
        break;
    case GL_IMAGE_2D:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_2D_ARRAY:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_CUBE: {
        if (count <= 1) {
            GLint sampler = *static_cast<const GLint *>(value);
            GL_CALL_EXTRA_FUNCTION(glProgramUniform1i(programID, GLint(id), sampler));
        } else {
            const GLint *sampler = static_cast<const GLint *>(value);
            GL_CALL_EXTRA_FUNCTION(glProgramUniform1iv(programID, GLint(id), count, sampler));
        }
    } break;
    default:
        qCCritical(INTERNAL_ERROR, "Unknown shader type format %d", int(type));
        Q_ASSERT(false);
        break;
    }
}

QSSGRenderBackend::QSSGRenderBackendComputeShaderObject QSSGRenderBackendGL4Impl::createComputeShader(QSSGByteView source,
                                                                                                            QByteArray &errorMessage,
                                                                                                            bool binary)
{
    GLuint shaderID = 0;
    shaderID = m_glExtraFunctions->glCreateShader(GL_COMPUTE_SHADER);

    if (shaderID && !compileSource(shaderID, source, errorMessage, binary)) {
        GL_CALL_EXTRA_FUNCTION(glDeleteShader(shaderID));
        shaderID = 0;
    }
    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendComputeShaderObject>(quintptr(shaderID));
}

void QSSGRenderBackendGL4Impl::dispatchCompute(QSSGRenderBackendShaderProgramObject, quint32 numGroupsX, quint32 numGroupsY, quint32 numGroupsZ)
{
    GL_CALL_EXTRA_FUNCTION(glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ));
}

QSSGRenderBackend::QSSGRenderBackendProgramPipeline QSSGRenderBackendGL4Impl::createProgramPipeline()
{
    GLuint pipeline;
    GL_CALL_EXTRA_FUNCTION(glGenProgramPipelines(1, &pipeline));

    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendProgramPipeline>(quintptr(pipeline));
}

void QSSGRenderBackendGL4Impl::releaseProgramPipeline(QSSGRenderBackendProgramPipeline ppo)
{
    GLuint pipeline = HandleToID_cast(GLuint, quintptr, ppo);
    GL_CALL_EXTRA_FUNCTION(glDeleteProgramPipelines(1, &pipeline));
}

void QSSGRenderBackendGL4Impl::setActiveProgramPipeline(QSSGRenderBackendProgramPipeline ppo)
{
    GLuint pipeline = HandleToID_cast(GLuint, quintptr, ppo);

    GL_CALL_EXTRA_FUNCTION(glBindProgramPipeline(pipeline));
}

void QSSGRenderBackendGL4Impl::setProgramStages(QSSGRenderBackendProgramPipeline ppo,
                                                  QSSGRenderShaderTypeFlags flags,
                                                  QSSGRenderBackendShaderProgramObject po)
{
    GLuint pipeline = HandleToID_cast(GLuint, quintptr, ppo);
    GLuint programID = 0;

    if (po) {
        QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
        programID = static_cast<GLuint>(pProgram->m_programID);
    }

    GL_CALL_EXTRA_FUNCTION(glUseProgramStages(pipeline, m_conversion.fromShaderTypeFlagsToGL(flags), programID));
}

void QSSGRenderBackendGL4Impl::setBlendEquation(const QSSGRenderBlendEquationArgument &pBlendEquArg)
{
    if (m_backendSupport.caps.bits.bNVAdvancedBlendSupported || m_backendSupport.caps.bits.bKHRAdvancedBlendSupported)
        GL_CALL_EXTRA_FUNCTION(
                glBlendEquation(m_conversion.fromBlendEquationToGL(pBlendEquArg.m_rgbEquation,
                                                                   m_backendSupport.caps.bits.bNVAdvancedBlendSupported,
                                                                   m_backendSupport.caps.bits.bKHRAdvancedBlendSupported)));
}

void QSSGRenderBackendGL4Impl::setBlendBarrier(void)
{
    if (m_backendSupport.caps.bits.bNVAdvancedBlendSupported)
        GL_CALL_QSSG_EXT(glBlendBarrierNV());
}

QSSGRenderBackend::QSSGRenderBackendPathObject QSSGRenderBackendGL4Impl::createPathNVObject(size_t range)
{
    GLuint pathID = GL_CALL_NVPATH_EXT(glGenPathsNV(GLsizei(range)));

    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendPathObject>(quintptr(pathID));
}
void QSSGRenderBackendGL4Impl::setPathSpecification(QSSGRenderBackendPathObject inPathObject,
                                                      QSSGByteView inPathCommands,
                                                      QSSGDataView<float> inPathCoords)
{
    GLuint pathID = HandleToID_cast(GLuint, quintptr, inPathObject);
    GL_CALL_NVPATH_EXT(glPathCommandsNV(pathID,
                                        inPathCommands.size(),
                                        inPathCommands.begin(),
                                        inPathCoords.size(),
                                        GL_FLOAT,
                                        inPathCoords.begin()));
}

QSSGBounds3 QSSGRenderBackendGL4Impl::getPathObjectBoundingBox(QSSGRenderBackendPathObject inPathObject)
{
    float data[4];
#if defined(GL_NV_path_rendering)
    GL_CALL_NVPATH_EXT(glGetPathParameterfvNV(HandleToID_cast(GLuint, quintptr, inPathObject), GL_PATH_OBJECT_BOUNDING_BOX_NV, data));
#endif
    return QSSGBounds3(QVector3D(data[0], data[1], 0.0f), QVector3D(data[2], data[3], 0.0f));
}

QSSGBounds3 QSSGRenderBackendGL4Impl::getPathObjectFillBox(QSSGRenderBackendPathObject inPathObject)
{
    float data[4];
#if defined(GL_NV_path_rendering)
    GL_CALL_NVPATH_EXT(glGetPathParameterfvNV(HandleToID_cast(GLuint, quintptr, inPathObject), GL_PATH_FILL_BOUNDING_BOX_NV, data));
#endif
    return QSSGBounds3(QVector3D(data[0], data[1], 0.0f), QVector3D(data[2], data[3], 0.0f));
}

QSSGBounds3 QSSGRenderBackendGL4Impl::getPathObjectStrokeBox(QSSGRenderBackendPathObject inPathObject)
{
    float data[4];
#if defined(GL_NV_path_rendering)
    GL_CALL_NVPATH_EXT(glGetPathParameterfvNV(HandleToID_cast(GLuint, quintptr, inPathObject), GL_PATH_STROKE_BOUNDING_BOX_NV, data));
#endif
    return QSSGBounds3(QVector3D(data[0], data[1], 0.0f), QVector3D(data[2], data[3], 0.0f));
}

void QSSGRenderBackendGL4Impl::setStrokeWidth(QSSGRenderBackendPathObject inPathObject, float inStrokeWidth)
{
#if defined(GL_NV_path_rendering)
    GL_CALL_NVPATH_EXT(glPathParameterfNV(HandleToID_cast(GLuint, quintptr, inPathObject), GL_PATH_STROKE_WIDTH_NV, inStrokeWidth));
#endif
}

void QSSGRenderBackendGL4Impl::setPathProjectionMatrix(const QMatrix4x4 inPathProjection)
{
#if defined(QT_OPENGL_ES)
    Q_UNUSED(inPathProjection)
#else
    GL_CALL_DIRECTSTATE_EXT(glMatrixLoadfEXT(GL_PROJECTION, inPathProjection.constData()));
#endif
}

void QSSGRenderBackendGL4Impl::setPathModelViewMatrix(const QMatrix4x4 inPathModelview)
{
#if defined(QT_OPENGL_ES)
    Q_UNUSED(inPathModelview)
#else
    GL_CALL_DIRECTSTATE_EXT(glMatrixLoadfEXT(GL_MODELVIEW, inPathModelview.constData()));
#endif
}

void QSSGRenderBackendGL4Impl::setPathStencilDepthOffset(float inSlope, float inBias)
{
    GL_CALL_NVPATH_EXT(glPathStencilDepthOffsetNV(inSlope, inBias));
}

void QSSGRenderBackendGL4Impl::setPathCoverDepthFunc(QSSGRenderBoolOp inDepthFunction)
{
    GL_CALL_NVPATH_EXT(glPathCoverDepthFuncNV(m_conversion.fromBoolOpToGL(inDepthFunction)));
}

void QSSGRenderBackendGL4Impl::stencilStrokePath(QSSGRenderBackendPathObject inPathObject)
{
    GL_CALL_NVPATH_EXT(glStencilStrokePathNV(HandleToID_cast(GLuint, quintptr, inPathObject), 0x1, ~GLuint(0)));
}

void QSSGRenderBackendGL4Impl::stencilFillPath(QSSGRenderBackendPathObject inPathObject)
{
#if defined(GL_NV_path_rendering)
    GL_CALL_NVPATH_EXT(glStencilFillPathNV(HandleToID_cast(GLuint, quintptr, inPathObject), GL_COUNT_UP_NV, ~GLuint(0)));
#endif
}

void QSSGRenderBackendGL4Impl::releasePathNVObject(QSSGRenderBackendPathObject po, size_t range)
{
    GLuint pathID = HandleToID_cast(GLuint, quintptr, po);

    GL_CALL_NVPATH_EXT(glDeletePathsNV(pathID, GLsizei(range)));
}

void QSSGRenderBackendGL4Impl::stencilFillPathInstanced(QSSGRenderBackendPathObject po,
                                                          size_t numPaths,
                                                          QSSGRenderPathFormatType type,
                                                          const void *charCodes,
                                                          QSSGRenderPathFillMode fillMode,
                                                          quint32 stencilMask,
                                                          QSSGRenderPathTransformType transformType,
                                                          const float *transformValues)
{
    GLuint pathID = HandleToID_cast(GLuint, quintptr, po);

    GL_CALL_NVPATH_EXT(glStencilFillPathInstancedNV(GLsizei(numPaths),
                                                    m_conversion.fromPathTypeToGL(type),
                                                    charCodes,
                                                    pathID,
                                                    m_conversion.fromPathFillModeToGL(fillMode),
                                                    stencilMask,
                                                    m_conversion.fromPathTransformToGL(transformType),
                                                    transformValues));
}

void QSSGRenderBackendGL4Impl::stencilStrokePathInstancedN(QSSGRenderBackendPathObject po,
                                                             size_t numPaths,
                                                             QSSGRenderPathFormatType type,
                                                             const void *charCodes,
                                                             qint32 stencilRef,
                                                             quint32 stencilMask,
                                                             QSSGRenderPathTransformType transformType,
                                                             const float *transformValues)
{
    GLuint pathID = HandleToID_cast(GLuint, quintptr, po);

    GL_CALL_NVPATH_EXT(glStencilStrokePathInstancedNV(GLsizei(numPaths),
                                                      m_conversion.fromPathTypeToGL(type),
                                                      charCodes,
                                                      pathID,
                                                      stencilRef,
                                                      stencilMask,
                                                      m_conversion.fromPathTransformToGL(transformType),
                                                      transformValues));
}

void QSSGRenderBackendGL4Impl::coverFillPathInstanced(QSSGRenderBackendPathObject po,
                                                        size_t numPaths,
                                                        QSSGRenderPathFormatType type,
                                                        const void *charCodes,
                                                        QSSGRenderPathCoverMode coverMode,
                                                        QSSGRenderPathTransformType transformType,
                                                        const float *transformValues)
{
    GLuint pathID = HandleToID_cast(GLuint, quintptr, po);

    GL_CALL_NVPATH_EXT(glCoverFillPathInstancedNV(GLsizei(numPaths),
                                                  m_conversion.fromPathTypeToGL(type),
                                                  charCodes,
                                                  pathID,
                                                  m_conversion.fromPathCoverModeToGL(coverMode),
                                                  m_conversion.fromPathTransformToGL(transformType),
                                                  transformValues));
}

void QSSGRenderBackendGL4Impl::coverStrokePathInstanced(QSSGRenderBackendPathObject po,
                                                          size_t numPaths,
                                                          QSSGRenderPathFormatType type,
                                                          const void *charCodes,
                                                          QSSGRenderPathCoverMode coverMode,
                                                          QSSGRenderPathTransformType transformType,
                                                          const float *transformValues)
{
    GLuint pathID = HandleToID_cast(GLuint, quintptr, po);

    GL_CALL_NVPATH_EXT(glCoverStrokePathInstancedNV(GLsizei(numPaths),
                                                    m_conversion.fromPathTypeToGL(type),
                                                    charCodes,
                                                    pathID,
                                                    m_conversion.fromPathCoverModeToGL(coverMode),
                                                    m_conversion.fromPathTransformToGL(transformType),
                                                    transformValues));
}

void QSSGRenderBackendGL4Impl::loadPathGlyphs(QSSGRenderBackendPathObject po,
                                                QSSGRenderPathFontTarget fontTarget,
                                                const void *fontName,
                                                QSSGRenderPathFontStyleFlags fontStyle,
                                                size_t numGlyphs,
                                                QSSGRenderPathFormatType type,
                                                const void *charCodes,
                                                QSSGRenderPathMissingGlyphs handleMissingGlyphs,
                                                QSSGRenderBackendPathObject pathParameterTemplate,
                                                float emScale)
{
    GLuint pathID = HandleToID_cast(GLuint, quintptr, po);
    GLuint pathTemplateID = (pathParameterTemplate == nullptr) ? ~GLuint(0) : HandleToID_cast(GLuint, quintptr, pathParameterTemplate);

    GL_CALL_NVPATH_EXT(glPathGlyphsNV(pathID,
                                      m_conversion.fromPathFontTargetToGL(fontTarget),
                                      fontName,
                                      m_conversion.fromPathFontStyleToGL(fontStyle),
                                      GLsizei(numGlyphs),
                                      m_conversion.fromPathTypeToGL(type),
                                      charCodes,
                                      m_conversion.fromPathMissingGlyphsToGL(handleMissingGlyphs),
                                      pathTemplateID,
                                      emScale));
}

QSSGRenderPathReturnValues QSSGRenderBackendGL4Impl::loadPathGlyphsIndexed(QSSGRenderBackendPathObject po,
                                                                                     QSSGRenderPathFontTarget fontTarget,
                                                                                     const void *fontName,
                                                                                     QSSGRenderPathFontStyleFlags fontStyle,
                                                                                     quint32 firstGlyphIndex,
                                                                                     size_t numGlyphs,
                                                                                     QSSGRenderBackendPathObject pathParameterTemplate,
                                                                                     float emScale)
{
    GLuint pathID = HandleToID_cast(GLuint, quintptr, po);
    GLuint pathTemplateID = (pathParameterTemplate == nullptr) ? ~GLuint(0) : HandleToID_cast(GLuint, quintptr, pathParameterTemplate);
    GLenum glRet = 0;

    glRet = GL_CALL_QSSG_EXT(glPathGlyphIndexArrayNV(pathID,
                                                       m_conversion.fromPathFontTargetToGL(fontTarget),
                                                       fontName,
                                                       m_conversion.fromPathFontStyleToGL(fontStyle),
                                                       firstGlyphIndex,
                                                       GLsizei(numGlyphs),
                                                       pathTemplateID,
                                                       emScale));

    return GLConversion::fromGLToPathFontReturn(glRet);
}

QSSGRenderBackend::QSSGRenderBackendPathObject QSSGRenderBackendGL4Impl::loadPathGlyphsIndexedRange(
        QSSGRenderPathFontTarget fontTarget,
        const void *fontName,
        QSSGRenderPathFontStyleFlags fontStyle,
        QSSGRenderBackendPathObject pathParameterTemplate,
        float emScale,
        quint32 *count)
{
    GLuint pathTemplateID = (pathParameterTemplate == nullptr) ? ~GLuint(0) : HandleToID_cast(GLuint, quintptr, pathParameterTemplate);
    GLuint baseAndCount[2] = { 0, 0 };

    GL_CALL_QSSG_EXT(glPathGlyphIndexRangeNV(m_conversion.fromPathFontTargetToGL(fontTarget),
                                                       fontName,
                                                       m_conversion.fromPathFontStyleToGL(fontStyle),
                                                       pathTemplateID,
                                                       emScale,
                                                       baseAndCount));

    if (count)
        *count = baseAndCount[1];

    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendPathObject>(quintptr(baseAndCount[0]));
}

void QSSGRenderBackendGL4Impl::loadPathGlyphRange(QSSGRenderBackendPathObject po,
                                                    QSSGRenderPathFontTarget fontTarget,
                                                    const void *fontName,
                                                    QSSGRenderPathFontStyleFlags fontStyle,
                                                    quint32 firstGlyph,
                                                    size_t numGlyphs,
                                                    QSSGRenderPathMissingGlyphs handleMissingGlyphs,
                                                    QSSGRenderBackendPathObject pathParameterTemplate,
                                                    float emScale)
{
    GLuint pathID = HandleToID_cast(GLuint, quintptr, po);
    GLuint pathTemplateID = (pathParameterTemplate == nullptr) ? ~GLuint(0) : HandleToID_cast(GLuint, quintptr, pathParameterTemplate);

    GL_CALL_NVPATH_EXT(glPathGlyphRangeNV(pathID,
                                          m_conversion.fromPathFontTargetToGL(fontTarget),
                                          fontName,
                                          m_conversion.fromPathFontStyleToGL(fontStyle),
                                          firstGlyph,
                                          GLsizei(numGlyphs),
                                          m_conversion.fromPathMissingGlyphsToGL(handleMissingGlyphs),
                                          pathTemplateID,
                                          emScale));
}

void QSSGRenderBackendGL4Impl::getPathMetrics(QSSGRenderBackendPathObject po,
                                                size_t numPaths,
                                                QSSGRenderPathGlyphFontMetricFlags metricQueryMask,
                                                QSSGRenderPathFormatType type,
                                                const void *charCodes,
                                                size_t stride,
                                                float *metrics)
{
    GLuint pathID = HandleToID_cast(GLuint, quintptr, po);

    GL_CALL_NVPATH_EXT(glGetPathMetricsNV(m_conversion.fromPathMetricQueryFlagsToGL(metricQueryMask),
                                          GLsizei(numPaths),
                                          m_conversion.fromPathTypeToGL(type),
                                          charCodes,
                                          pathID,
                                          GLsizei(stride),
                                          metrics));
}

void QSSGRenderBackendGL4Impl::getPathMetricsRange(QSSGRenderBackendPathObject po,
                                                     size_t numPaths,
                                                     QSSGRenderPathGlyphFontMetricFlags metricQueryMask,
                                                     size_t stride,
                                                     float *metrics)
{
    GLuint pathID = HandleToID_cast(GLuint, quintptr, po);

    GL_CALL_NVPATH_EXT(
            glGetPathMetricRangeNV(m_conversion.fromPathMetricQueryFlagsToGL(metricQueryMask), pathID, GLsizei(numPaths), GLsizei(stride), metrics));
}

void QSSGRenderBackendGL4Impl::getPathSpacing(QSSGRenderBackendPathObject po,
                                                size_t numPaths,
                                                QSSGRenderPathListMode pathListMode,
                                                QSSGRenderPathFormatType type,
                                                const void *charCodes,
                                                float advanceScale,
                                                float kerningScale,
                                                QSSGRenderPathTransformType transformType,
                                                float *spacing)
{
    GLuint pathID = HandleToID_cast(GLuint, quintptr, po);

    GL_CALL_NVPATH_EXT(glGetPathSpacingNV(m_conversion.fromPathListModeToGL(pathListMode),
                                          GLsizei(numPaths),
                                          m_conversion.fromPathTypeToGL(type),
                                          charCodes,
                                          pathID,
                                          advanceScale,
                                          kerningScale,
                                          m_conversion.fromPathTransformToGL(transformType),
                                          spacing));
}

QT_END_NAMESPACE
