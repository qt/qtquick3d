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

#include <QtQuick3DRender/private/qssgrenderbackendgles2_p.h>
#include <QtQuick3DRender/private/qssgrenderbackendinputassemblergl_p.h>
#include <QtQuick3DRender/private/qssgrenderbackendrenderstatesgl_p.h>
#include <QtQuick3DRender/private/qssgrenderbackendshaderprogramgl_p.h>
#include <QtQuick3DRender/private/qssgopenglextensions_p.h>

QT_BEGIN_NAMESPACE

#ifdef RENDER_BACKEND_LOG_GL_ERRORS
#define RENDER_LOG_ERROR_PARAMS(x) checkGLError(#x, __FILE__, __LINE__)
#else
#define RENDER_LOG_ERROR_PARAMS(x) checkGLError()
#endif

#if defined(QT_OPENGL_ES) || defined(QT_OPENGL_ES_2_ANGLE)
#define GL_CALL_TIMER_EXT(x)                                                                                           \
    m_QSSGExtensions->x;                                                                                             \
    RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_TESSELATION_EXT(x)                                                                                     \
    m_QSSGExtensions->x;                                                                                             \
    RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_MULTISAMPLE_EXT(x)                                                                                     \
    m_QSSGExtensions->x;                                                                                             \
    RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_EXTRA_FUNCTION(x)                                                                                      \
    m_glExtraFunctions->x;                                                                                             \
    RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_EXTENSION_FUNCTION(x)                                                                                  \
    m_QSSGExtensions->x;                                                                                             \
    RENDER_LOG_ERROR_PARAMS(x);
#else
#define GL_CALL_TIMER_EXT(x)
#define GL_CALL_TESSELATION_EXT(x)
#define GL_CALL_MULTISAMPLE_EXT(x)
#define GL_CALL_EXTRA_FUNCTION(x)                                                                                      \
    m_glExtraFunctions->x;                                                                                             \
    RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_EXTENSION_FUNCTION(x)
#endif

#ifndef GL_DEPTH_STENCIL_OES
#define GL_DEPTH_STENCIL_OES 0x84F9
#endif

QByteArray exts3tc()
{
    return QByteArrayLiteral("GL_EXT_texture_compression_s3tc");
}
QByteArray extsdxt()
{
    return QByteArrayLiteral("GL_EXT_texture_compression_dxt1");
}
QByteArray extsAniso()
{
    return QByteArrayLiteral("GL_EXT_texture_filter_anisotropic");
}
QByteArray extsTexSwizzle()
{
    return QByteArrayLiteral("GL_ARB_texture_swizzle");
}
QByteArray extsFPRenderTarget()
{
    return QByteArrayLiteral("GL_EXT_color_buffer_float");
}
QByteArray extsTimerQuery()
{
    return QByteArrayLiteral("GL_EXT_timer_query");
}
QByteArray extsGpuShader5()
{
    return QByteArrayLiteral("EXT_gpu_shader5");
}

QByteArray extDepthTexture()
{
    return QByteArrayLiteral("GL_OES_packed_depth_stencil");
}
QByteArray extvao()
{
    return QByteArrayLiteral("GL_OES_vertex_array_object");
}
QByteArray extStdDd()
{
    return QByteArrayLiteral("GL_OES_standard_derivatives");
}
QByteArray extTexLod()
{
    return QByteArrayLiteral("GL_EXT_shader_texture_lod");
}

/// constructor
QSSGRenderBackendGLES2Impl::QSSGRenderBackendGLES2Impl(const QSurfaceFormat &format)
    : QSSGRenderBackendGLBase(format)
{
    // clear support bits
    m_backendSupport.caps.u32Values = 0;

    const char *extensions = getExtensionString();
    m_extensions = QByteArray(extensions).split(' ');

    // get extension count
    GLint numExtensions = m_extensions.size();

    for (qint32 i = 0; i < numExtensions; i++) {

        const QByteArray &extensionString = m_extensions.at(i);

        // search for extension
        if (!m_backendSupport.caps.bits.bDXTImagesSupported
            && (exts3tc().compare(extensionString) == 0 || extsdxt().compare(extensionString) == 0)) {
            m_backendSupport.caps.bits.bDXTImagesSupported = true;
        } else if (!m_backendSupport.caps.bits.bAnistropySupported && extsAniso().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bAnistropySupported = true;
        } else if (!m_backendSupport.caps.bits.bFPRenderTargetsSupported && extsFPRenderTarget().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bFPRenderTargetsSupported = true;
        } else if (!m_backendSupport.caps.bits.bTimerQuerySupported && extsTimerQuery().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bTimerQuerySupported = true;
        } else if (!m_backendSupport.caps.bits.bGPUShader5ExtensionSupported && extsGpuShader5().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bGPUShader5ExtensionSupported = true;
        } else if (!m_backendSupport.caps.bits.bTextureSwizzleSupported && extsTexSwizzle().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bTextureSwizzleSupported = true;
        } else if (!m_backendSupport.caps.bits.bDepthStencilSupported && extDepthTexture().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bDepthStencilSupported = true;
        } else if (!m_backendSupport.caps.bits.bVertexArrayObjectSupported && extvao().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bVertexArrayObjectSupported = true;
        } else if (!m_backendSupport.caps.bits.bStandardDerivativesSupported && extStdDd().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bStandardDerivativesSupported = true;
        } else if (!m_backendSupport.caps.bits.bTextureLodSupported && extTexLod().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bTextureLodSupported = true;
        }
    }

    qCInfo(RENDER_TRACE_INFO, "OpenGL extensions: %s", extensions);

    // constant buffers support is always not true
    m_backendSupport.caps.bits.bConstantBufferSupported = false;

    // query hardware
    GL_CALL_EXTRA_FUNCTION(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &m_maxAttribCount));

    // internal state tracker
    m_pCurrentMiscState = new QSSGRenderBackendMiscStateGL();

    // finally setup caps based on device
    setAndInspectHardwareCaps();

    // Initialize extensions
#if defined(QT_OPENGL_ES) || defined(QT_OPENGL_ES_2_ANGLE)
    m_QSSGExtensions = new QSSGOpenGLES2Extensions;
    m_QSSGExtensions->initializeOpenGLFunctions();
#endif
}
/// destructor
QSSGRenderBackendGLES2Impl::~QSSGRenderBackendGLES2Impl()
{
    delete m_pCurrentMiscState;
#if defined(QT_OPENGL_ES) || defined(QT_OPENGL_ES_2_ANGLE)
    delete m_QSSGExtensions;
#endif
}

void QSSGRenderBackendGLES2Impl::setMultisampledTextureData2D(QSSGRenderBackendTextureObject to,
                                                                QSSGRenderTextureTargetType target,
                                                                qint32 samples,
                                                                QSSGRenderTextureFormat internalFormat,
                                                                qint32 width,
                                                                qint32 height,
                                                                bool fixedsamplelocations)
{
    Q_UNUSED(to)
    Q_UNUSED(target)
    Q_UNUSED(samples)
    Q_UNUSED(internalFormat)
    Q_UNUSED(width)
    Q_UNUSED(height)
    Q_UNUSED(fixedsamplelocations)
}

void QSSGRenderBackendGLES2Impl::setTextureData3D(QSSGRenderBackendTextureObject to,
                                                    QSSGRenderTextureTargetType target,
                                                    qint32 level,
                                                    QSSGRenderTextureFormat internalFormat,
                                                    qint32 width,
                                                    qint32 height,
                                                    qint32 depth,
                                                    qint32 border,
                                                    QSSGRenderTextureFormat format,
                                                    QSSGByteView hostData)
{
    GLuint texID = HandleToID_cast(GLuint, quintptr, to);
    GLenum glTarget = GLConversion::fromTextureTargetToGL(target);
    setActiveTexture(GL_TEXTURE0);
    GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, texID));
    bool conversionRequired = format != internalFormat;

    QSSGRenderTextureSwizzleMode swizzleMode = QSSGRenderTextureSwizzleMode::NoSwizzle;
    internalFormat = GLConversion::replaceDeprecatedTextureFormat(getRenderContextType(), internalFormat, swizzleMode);

    GLenum glformat = 0, glInternalFormat = 0, gltype = GL_UNSIGNED_BYTE;

    if (internalFormat.isUncompressedTextureFormat()) {
        GLConversion::fromUncompressedTextureFormatToGL(getRenderContextType(), internalFormat, glformat, gltype, glInternalFormat);
    }

    if (conversionRequired) {
        GLenum dummy;
        GLConversion::fromUncompressedTextureFormatToGL(getRenderContextType(), format, glformat, gltype, dummy);
    } else if (internalFormat.isCompressedTextureFormat()) {
        GLConversion::fromUncompressedTextureFormatToGL(getRenderContextType(), format, glformat, gltype, glInternalFormat);
        glInternalFormat = GLConversion::fromCompressedTextureFormatToGL(internalFormat);
    } else if (format.isDepthTextureFormat()) {
        GLConversion::fromDepthTextureFormatToGL(getRenderContextType(), format, glformat, gltype, glInternalFormat);
    }

    GL_CALL_EXTRA_FUNCTION(
            glTexImage3D(glTarget, level, glInternalFormat, GLsizei(width), GLsizei(height), GLsizei(depth), border, glformat, gltype, hostData));

    GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, 0));
}

void QSSGRenderBackendGLES2Impl::setTextureData2D(QSSGRenderBackendTextureObject to,
                                                    QSSGRenderTextureTargetType target,
                                                    qint32 level,
                                                    QSSGRenderTextureFormat internalFormat,
                                                    qint32 width,
                                                    qint32 height,
                                                    qint32 border,
                                                    QSSGRenderTextureFormat format,
                                                    QSSGByteView hostData)
{
    GLuint texID = HandleToID_cast(GLuint, quintptr, to);
    GLenum glTarget = GLConversion::fromTextureTargetToGL(target);
    setActiveTexture(GL_TEXTURE0);
    GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, texID));
    bool conversionRequired = format != internalFormat;

    QSSGRenderTextureSwizzleMode swizzleMode = QSSGRenderTextureSwizzleMode::NoSwizzle;
    internalFormat = GLConversion::replaceDeprecatedTextureFormat(getRenderContextType(), internalFormat, swizzleMode);

    GLenum glformat = 0, glInternalFormat = 0, gltype = GL_UNSIGNED_BYTE;

    if (internalFormat.isUncompressedTextureFormat()) {
        GLConversion::fromUncompressedTextureFormatToGL(getRenderContextType(), internalFormat, glformat, gltype, glInternalFormat);
        glInternalFormat = glformat;
    }

    if (conversionRequired) {
        GLenum dummy;
        GLConversion::fromUncompressedTextureFormatToGL(getRenderContextType(), format, glformat, gltype, dummy);
    } else if (internalFormat.isCompressedTextureFormat()) {
        GLConversion::fromUncompressedTextureFormatToGL(getRenderContextType(), format, glformat, gltype, glInternalFormat);
        glInternalFormat = GLConversion::fromCompressedTextureFormatToGL(internalFormat);
    } else if (format.isDepthTextureFormat()) {
        GLConversion::fromDepthTextureFormatToGL(getRenderContextType(), format, glformat, gltype, glInternalFormat);
        if (format == QSSGRenderTextureFormat::Depth24Stencil8) {
            glformat = GL_DEPTH_STENCIL_OES;
            gltype = GL_UNSIGNED_INT_24_8;
        }
        glInternalFormat = glformat;
    }

    Q_ASSERT(glformat == glInternalFormat);
    GL_CALL_EXTRA_FUNCTION(
            glTexImage2D(glTarget, level, glInternalFormat, GLsizei(width), GLsizei(height), border, glformat, gltype, hostData));
    GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, 0));
}

void QSSGRenderBackendGLES2Impl::updateSampler(QSSGRenderBackendSamplerObject /* so */,
                                                 QSSGRenderTextureTargetType target,
                                                 QSSGRenderTextureMinifyingOp minFilter,
                                                 QSSGRenderTextureMagnifyingOp magFilter,
                                                 QSSGRenderTextureCoordOp wrapS,
                                                 QSSGRenderTextureCoordOp wrapT,
                                                 QSSGRenderTextureCoordOp wrapR,
                                                 float minLod,
                                                 float maxLod,
                                                 float lodBias,
                                                 QSSGRenderTextureCompareMode compareMode,
                                                 QSSGRenderTextureCompareOp compareFunc,
                                                 float anisotropy,
                                                 float *borderColor)
{

    // Satisfy the compiler
    // These are not available in GLES 3 and we don't use them right now
    Q_ASSERT(qFuzzyIsNull(lodBias));
    Q_ASSERT(!borderColor);
    Q_UNUSED(lodBias)
    Q_UNUSED(borderColor)
    Q_UNUSED(wrapR)
    Q_UNUSED(minLod)
    Q_UNUSED(maxLod)
    Q_UNUSED(compareMode)
    Q_UNUSED(compareFunc)

    GLenum glTarget = GLConversion::fromTextureTargetToGL(target);

    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_MIN_FILTER, m_conversion.fromTextureMinifyingOpToGL(minFilter)));
    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_MAG_FILTER, m_conversion.fromTextureMagnifyingOpToGL(magFilter)));
    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_WRAP_S, m_conversion.fromTextureCoordOpToGL(wrapS)));
    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_WRAP_T, m_conversion.fromTextureCoordOpToGL(wrapT)));

    if (m_backendSupport.caps.bits.bAnistropySupported) {
        GL_CALL_EXTRA_FUNCTION(glTexParameterf(glTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy));
    }
}

void QSSGRenderBackendGLES2Impl::updateTextureObject(QSSGRenderBackendTextureObject to,
                                                       QSSGRenderTextureTargetType target,
                                                       qint32 baseLevel,
                                                       qint32 maxLevel)
{
    Q_UNUSED(to)

    GLenum glTarget = GLConversion::fromTextureTargetToGL(target);

    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_BASE_LEVEL, baseLevel));
    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_MAX_LEVEL, maxLevel));
}

void QSSGRenderBackendGLES2Impl::updateTextureSwizzle(QSSGRenderBackendTextureObject to,
                                                        QSSGRenderTextureTargetType target,
                                                        QSSGRenderTextureSwizzleMode swizzleMode)
{
    Q_UNUSED(to)
    Q_UNUSED(target)
    Q_UNUSED(swizzleMode)
#if defined(QT_OPENGL_ES)
    if (m_backendSupport.caps.bits.bTextureSwizzleSupported) {
        GLint glSwizzle[4];
        GLenum glTarget = m_conversion.fromTextureTargetToGL(target);
        m_conversion.NVRenderConvertSwizzleModeToGL(swizzleMode, glSwizzle);

        // since ES3 spec has no GL_TEXTURE_SWIZZLE_RGBA set it separately
        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_SWIZZLE_R, glSwizzle[0]));
        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_SWIZZLE_G, glSwizzle[1]));
        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_SWIZZLE_B, glSwizzle[2]));
        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_SWIZZLE_A, glSwizzle[3]));
    }
#endif
}

qint32 QSSGRenderBackendGLES2Impl::getDepthBits() const
{
    qint32 depthBits;
    GL_CALL_EXTRA_FUNCTION(
            glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &depthBits));

    return depthBits;
}

qint32 QSSGRenderBackendGLES2Impl::getStencilBits() const
{
    qint32 stencilBits;
    GL_CALL_EXTRA_FUNCTION(
            glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &stencilBits));

    return stencilBits;
}

void QSSGRenderBackendGLES2Impl::generateMipMaps(QSSGRenderBackendTextureObject to,
                                                   QSSGRenderTextureTargetType target,
                                                   QSSGRenderHint /*genType*/)
{
    GLuint texID = HandleToID_cast(GLuint, quintptr, to);
    GLenum glTarget = GLConversion::fromTextureTargetToGL(target);
    setActiveTexture(GL_TEXTURE0);
    GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, texID));
    GL_CALL_EXTRA_FUNCTION(glGenerateMipmap(glTarget));
    GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, 0));
}

QByteArray QSSGRenderBackendGLES2Impl::getShadingLanguageVersion()
{
    return QByteArrayLiteral("#version 100\n");
}

bool QSSGRenderBackendGLES2Impl::setInputAssembler(QSSGRenderBackendInputAssemblerObject iao, QSSGRenderBackendShaderProgramObject po)
{
    if (iao == nullptr) {
        // unbind and return;
        GL_CALL_EXTENSION_FUNCTION(glBindVertexArrayOES(0));
        return true;
    }

    QSSGRenderBackendInputAssemblerGL *inputAssembler = reinterpret_cast<QSSGRenderBackendInputAssemblerGL *>(iao);
    QSSGRenderBackendAttributeLayoutGL *attribLayout = inputAssembler->m_attribLayout;
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);
    QSSGDataRef<QSSGRenderBackendShaderInputEntryGL> shaderAttribBuffer;
    if (pProgram->m_shaderInput)
        shaderAttribBuffer = pProgram->m_shaderInput->m_shaderInputEntries;

    if (inputAssembler->m_vertexbufferHandles.size() <= attribLayout->m_maxInputSlot)
        return false;

    if (inputAssembler->m_vaoID == 0) {
        // generate vao
        GL_CALL_EXTENSION_FUNCTION(glGenVertexArraysOES(1, &inputAssembler->m_vaoID));
        Q_ASSERT(inputAssembler->m_vaoID);
    }

    if (inputAssembler->m_cachedShaderHandle != programID) {
        GL_CALL_EXTENSION_FUNCTION(glBindVertexArrayOES(inputAssembler->m_vaoID));
        inputAssembler->m_cachedShaderHandle = programID;

        for (const auto &attrib : qAsConst(shaderAttribBuffer)) {
            QSSGRenderBackendLayoutEntryGL *entry = attribLayout->getEntryByName(attrib.m_attribName);

            if (entry) {
                QSSGRenderBackendLayoutEntryGL &entryData(*entry);
                if (Q_UNLIKELY(entryData.m_type != attrib.m_type || entryData.m_numComponents != attrib.m_numComponents)) {
                    qCCritical(RENDER_INVALID_OPERATION, "Attrib %s dn't match vertex layout", attrib.m_attribName.constData());
                    Q_ASSERT(false);
                    return false;
                }

                entryData.m_attribIndex = attrib.m_attribLocation;
            } else {
                qCWarning(RENDER_WARNING, "Failed to Bind attribute %s", attrib.m_attribName.constData());
            }
        }

        // disable max possible used first
        // this is currently sufficient since we always re-arrange input attributes from 0
        for (int i = 0; i < attribLayout->m_layoutAttribEntries.size(); i++)
            GL_CALL_EXTRA_FUNCTION(glDisableVertexAttribArray(GLuint(i)));

        // setup all attribs
        GLuint boundArrayBufferId = 0; // 0 means unbound
        for (int idx = 0; idx != shaderAttribBuffer.size(); ++idx)
        {
            QSSGRenderBackendLayoutEntryGL *entry = attribLayout->getEntryByName(shaderAttribBuffer[idx].m_attribName);
            if (entry) {
                const QSSGRenderBackendLayoutEntryGL &entryData(*entry);
                GLuint id = HandleToID_cast(GLuint, quintptr, inputAssembler->m_vertexbufferHandles.mData[entryData.m_inputSlot]);
                if (boundArrayBufferId != id) {
                    GL_CALL_EXTRA_FUNCTION(glBindBuffer(GL_ARRAY_BUFFER, id));
                    boundArrayBufferId = id;
                }
                GL_CALL_EXTRA_FUNCTION(glEnableVertexAttribArray(entryData.m_attribIndex));
                GLuint offset = inputAssembler->m_offsets.at(int(entryData.m_inputSlot));
                GLuint stride = inputAssembler->m_strides.at(int(entryData.m_inputSlot));
                GL_CALL_EXTRA_FUNCTION(glVertexAttribPointer(entryData.m_attribIndex,
                                                             GLint(entryData.m_numComponents),
                                                             GL_FLOAT,
                                                             GL_FALSE,
                                                             GLsizei(stride),
                                                             reinterpret_cast<const void *>(quintptr(entryData.m_offset + offset))));

            } else {
                GL_CALL_EXTRA_FUNCTION(glDisableVertexAttribArray(GLuint(idx)));
            }
        }

        // setup index buffer.
        if (inputAssembler->m_indexbufferHandle) {
            GL_CALL_EXTRA_FUNCTION(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                                                HandleToID_cast(GLuint, quintptr, inputAssembler->m_indexbufferHandle)));
        } else {
            GL_CALL_EXTRA_FUNCTION(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
        }
    } else {
        GL_CALL_EXTENSION_FUNCTION(glBindVertexArrayOES(inputAssembler->m_vaoID));
    }

    return true;
}

void QSSGRenderBackendGLES2Impl::setDrawBuffers(QSSGRenderBackendRenderTargetObject rto, QSSGDataView<qint32> inDrawBufferSet)
{
    Q_UNUSED(rto)

    m_drawBuffersArray.clear();

    for (int idx = 0, end = inDrawBufferSet.size(); idx < end; ++idx) {
        if (inDrawBufferSet[idx] < 0)
            m_drawBuffersArray.push_back(GL_NONE);
        else
            m_drawBuffersArray.push_back(GLenum(GL_COLOR_ATTACHMENT0 + inDrawBufferSet[idx]));
    }

    GL_CALL_EXTRA_FUNCTION(glDrawBuffers(m_drawBuffersArray.size(), m_drawBuffersArray.data()));
}

void QSSGRenderBackendGLES2Impl::setReadBuffer(QSSGRenderBackendRenderTargetObject rto, QSSGReadFace inReadFace)
{
    Q_UNUSED(rto)
    Q_UNUSED(inReadFace)
}

void QSSGRenderBackendGLES2Impl::renderTargetAttach(QSSGRenderBackendRenderTargetObject,
                                                      QSSGRenderFrameBufferAttachment attachment,
                                                      QSSGRenderBackendTextureObject to,
                                                      qint32 level,
                                                      qint32 layer)
{
    Q_UNUSED(attachment)
    Q_UNUSED(to)
    Q_UNUSED(level)
    Q_UNUSED(layer)
    Q_ASSERT(false);
}

void QSSGRenderBackendGLES2Impl::blitFramebuffer(qint32 srcX0,
                                                   qint32 srcY0,
                                                   qint32 srcX1,
                                                   qint32 srcY1,
                                                   qint32 dstX0,
                                                   qint32 dstY0,
                                                   qint32 dstX1,
                                                   qint32 dstY1,
                                                   QSSGRenderClearFlags flags,
                                                   QSSGRenderTextureMagnifyingOp filter)
{
    GL_CALL_EXTRA_FUNCTION(glBlitFramebuffer(srcX0,
                                             srcY0,
                                             srcX1,
                                             srcY1,
                                             dstX0,
                                             dstY0,
                                             dstX1,
                                             dstY1,
                                             m_conversion.fromClearFlagsToGL(flags),
                                             m_conversion.fromTextureMagnifyingOpToGL(filter)));
}

void QSSGRenderBackendGLES2Impl::copyFramebufferTexture(qint32 srcX0,
                                                        qint32 srcY0,
                                                        qint32 width,
                                                        qint32 height,
                                                        qint32 dstX0,
                                                        qint32 dstY0,
                                                        QSSGRenderBackendTextureObject texture,
                                                        QSSGRenderTextureTargetType target)
{
    GLuint texID = HandleToID_cast(GLuint, quintptr, texture);
    GLenum glTarget = GLConversion::fromTextureTargetToGL(target);
    setActiveTexture(GL_TEXTURE0);
    GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, texID))
    GL_CALL_EXTRA_FUNCTION(glCopyTexSubImage2D(GL_TEXTURE_2D, 0, srcX0, srcY0, dstX0, dstY0,
                                               width, height))
}

QSSGRenderBackend::QSSGRenderBackendRenderTargetObject QSSGRenderBackendGLES2Impl::createRenderTarget()
{
    GLuint fboID = 0;
    GL_CALL_EXTRA_FUNCTION(glGenFramebuffers(1, &fboID));
    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendRenderTargetObject>(quintptr(fboID));
}

void QSSGRenderBackendGLES2Impl::releaseRenderTarget(QSSGRenderBackendRenderTargetObject rto)
{
    GLuint fboID = HandleToID_cast(GLuint, quintptr, rto);

    if (fboID)
        GL_CALL_EXTRA_FUNCTION(glDeleteFramebuffers(1, &fboID));
}

void QSSGRenderBackendGLES2Impl::renderTargetAttach(QSSGRenderBackendRenderTargetObject /* rto */,
                                                      QSSGRenderFrameBufferAttachment attachment,
                                                      QSSGRenderBackendRenderbufferObject rbo)
{
    // rto must be the current render target
    GLuint rbID = HandleToID_cast(GLuint, quintptr, rbo);

    GLenum glAttach = GLConversion::fromFramebufferAttachmentsToGL(attachment);

    GL_CALL_EXTRA_FUNCTION(glFramebufferRenderbuffer(GL_FRAMEBUFFER, glAttach, GL_RENDERBUFFER, rbID));
}

void QSSGRenderBackendGLES2Impl::renderTargetAttach(QSSGRenderBackendRenderTargetObject /* rto */,
                                                      QSSGRenderFrameBufferAttachment attachment,
                                                      QSSGRenderBackendTextureObject to,
                                                      QSSGRenderTextureTargetType target)
{
    // rto must be the current render target
    GLuint texID = HandleToID_cast(GLuint, quintptr, to);

    Q_ASSERT(target == QSSGRenderTextureTargetType::Texture2D || m_backendSupport.caps.bits.bMsTextureSupported);

    GLenum glAttach = GLConversion::fromFramebufferAttachmentsToGL(attachment);
    GLenum glTarget = GLConversion::fromTextureTargetToGL(target);

    if (attachment == QSSGRenderFrameBufferAttachment::DepthStencil) {
        GL_CALL_EXTRA_FUNCTION(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, glTarget, texID, 0));
        GL_CALL_EXTRA_FUNCTION(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, glTarget, texID, 0));
    } else {
        GL_CALL_EXTRA_FUNCTION(glFramebufferTexture2D(GL_FRAMEBUFFER, glAttach, glTarget, texID, 0));
    }
}

void QSSGRenderBackendGLES2Impl::setRenderTarget(QSSGRenderBackendRenderTargetObject rto)
{
    GLuint fboID = HandleToID_cast(GLuint, quintptr, rto);

    GL_CALL_EXTRA_FUNCTION(glBindFramebuffer(GL_FRAMEBUFFER, fboID));
}

void QSSGRenderBackendGLES2Impl::setReadTarget(QSSGRenderBackendRenderTargetObject rto)
{
    GLuint fboID = HandleToID_cast(GLuint, quintptr, rto);

    GL_CALL_EXTRA_FUNCTION(glBindFramebuffer(GL_READ_FRAMEBUFFER, fboID));
}

bool QSSGRenderBackendGLES2Impl::renderTargetIsValid(QSSGRenderBackendRenderTargetObject /* rto */)
{
    GLenum completeStatus = GL_CALL_EXTRA_FUNCTION(glCheckFramebufferStatus(GL_FRAMEBUFFER));
    switch (completeStatus) {
#define HANDLE_INCOMPLETE_STATUS(x)                                                                                    \
    case x:                                                                                                            \
        qCCritical(RENDER_INTERNAL_ERROR, "Framebuffer is not complete: %s", #x);                                             \
        return false;
        HANDLE_INCOMPLETE_STATUS(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
        HANDLE_INCOMPLETE_STATUS(GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS)
        HANDLE_INCOMPLETE_STATUS(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
        HANDLE_INCOMPLETE_STATUS(GL_FRAMEBUFFER_UNSUPPORTED)
#undef HANDLE_INCOMPLETE_STATUS
    }
    return true;
}

QSSGRenderBackend::QSSGRenderBackendRenderbufferObject QSSGRenderBackendGLES2Impl::createRenderbuffer(QSSGRenderRenderBufferFormat storageFormat,
                                                                                                            qint32 width,
                                                                                                            qint32 height)
{
    GLuint bufID = 0;

    GL_CALL_EXTRA_FUNCTION(glGenRenderbuffers(1, &bufID));
    GL_CALL_EXTRA_FUNCTION(glBindRenderbuffer(GL_RENDERBUFFER, bufID));
    GL_CALL_EXTRA_FUNCTION(glRenderbufferStorage(GL_RENDERBUFFER,
                                                 GLConversion::fromRenderBufferFormatsToRenderBufferGL(storageFormat),
                                                 GLsizei(width),
                                                 GLsizei(height)));

    // check for error
    GLenum error = m_glFunctions->glGetError();
    if (error != GL_NO_ERROR) {
        qCCritical(RENDER_GL_ERROR, "%s", GLConversion::processGLError(error));
        Q_ASSERT(false);
        GL_CALL_EXTRA_FUNCTION(glDeleteRenderbuffers(1, &bufID));
        bufID = 0;
    }

    GL_CALL_EXTRA_FUNCTION(glBindRenderbuffer(GL_RENDERBUFFER, 0));

    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendRenderbufferObject>(quintptr(bufID));
}

void QSSGRenderBackendGLES2Impl::releaseRenderbuffer(QSSGRenderBackendRenderbufferObject rbo)
{
    GLuint bufID = HandleToID_cast(GLuint, quintptr, rbo);

    if (bufID)
        GL_CALL_EXTRA_FUNCTION(glDeleteRenderbuffers(1, &bufID));
}

bool QSSGRenderBackendGLES2Impl::resizeRenderbuffer(QSSGRenderBackendRenderbufferObject rbo,
                                                      QSSGRenderRenderBufferFormat storageFormat,
                                                      qint32 width,
                                                      qint32 height)
{
    bool success = true;
    GLuint bufID = HandleToID_cast(GLuint, quintptr, rbo);

    Q_ASSERT(bufID);

    GL_CALL_EXTRA_FUNCTION(glBindRenderbuffer(GL_RENDERBUFFER, bufID));
    GL_CALL_EXTRA_FUNCTION(glRenderbufferStorage(GL_RENDERBUFFER,
                                                 GLConversion::fromRenderBufferFormatsToRenderBufferGL(storageFormat),
                                                 GLsizei(width),
                                                 GLsizei(height)));

    // check for error
    GLenum error = m_glFunctions->glGetError();
    if (error != GL_NO_ERROR) {
        qCCritical(RENDER_GL_ERROR, "%s", GLConversion::processGLError(error));
        Q_ASSERT(false);
        success = false;
    }

    return success;
}

void *QSSGRenderBackendGLES2Impl::mapBuffer(QSSGRenderBackendBufferObject,
                                              QSSGRenderBufferType bindFlags,
                                              size_t offset,
                                              size_t length,
                                              QSSGRenderBufferAccessFlags accessFlags)
{
    void *ret = nullptr;
    ret = GL_CALL_EXTRA_FUNCTION(glMapBufferRange(m_conversion.fromBindBufferFlagsToGL(bindFlags),
                                                  GLintptr(offset),
                                                  GLsizeiptr(length),
                                                  m_conversion.fromBufferAccessBitToGL(accessFlags)));

    return ret;
}

bool QSSGRenderBackendGLES2Impl::unmapBuffer(QSSGRenderBackendBufferObject, QSSGRenderBufferType bindFlags)
{
    const GLboolean ret = GL_CALL_EXTRA_FUNCTION(glUnmapBuffer(m_conversion.fromBindBufferFlagsToGL(bindFlags)));
    return (ret != 0);
}

qint32 QSSGRenderBackendGLES2Impl::getConstantBufferCount(QSSGRenderBackendShaderProgramObject po)
{
    Q_ASSERT(po);
    GLint numUniformBuffers = 0;
    if (getRenderBackendCap(QSSGRenderBackendCaps::ConstantBuffer)) {
        QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
        GLuint programID = static_cast<GLuint>(pProgram->m_programID);

        GL_CALL_EXTRA_FUNCTION(glGetProgramiv(programID, GL_ACTIVE_UNIFORM_BLOCKS, &numUniformBuffers));
    }
    return numUniformBuffers;
}

qint32 QSSGRenderBackendGLES2Impl::getConstantBufferInfoByID(QSSGRenderBackendShaderProgramObject po,
                                                               quint32 id,
                                                               quint32 nameBufSize,
                                                               qint32 *paramCount,
                                                               qint32 *bufferSize,
                                                               qint32 *length,
                                                               char *nameBuf)
{
    Q_ASSERT(po);
    Q_ASSERT(length);
    Q_ASSERT(nameBuf);

    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);
    GLuint blockIndex = GL_INVALID_INDEX;

    GL_CALL_EXTRA_FUNCTION(glGetActiveUniformBlockName(programID, id, GLsizei(nameBufSize), length, nameBuf));

    if (*length > 0) {
        blockIndex = GL_CALL_EXTRA_FUNCTION(glGetUniformBlockIndex(programID, nameBuf));
        if (blockIndex != GL_INVALID_INDEX) {
            GL_CALL_EXTRA_FUNCTION(glGetActiveUniformBlockiv(programID, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, bufferSize));
            GL_CALL_EXTRA_FUNCTION(glGetActiveUniformBlockiv(programID, blockIndex, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, paramCount));
        }
    }

    return qint32(blockIndex);
}

void QSSGRenderBackendGLES2Impl::getConstantBufferParamIndices(QSSGRenderBackendShaderProgramObject po, quint32 id, qint32 *indices)
{
    Q_ASSERT(po);
    Q_ASSERT(indices);

    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);

    if (indices) {
        GL_CALL_EXTRA_FUNCTION(glGetActiveUniformBlockiv(programID, id, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, indices));
    }
}

void QSSGRenderBackendGLES2Impl::getConstantBufferParamInfoByIndices(QSSGRenderBackendShaderProgramObject po,
                                                                       quint32 count,
                                                                       quint32 *indices,
                                                                       QSSGRenderShaderDataType *type,
                                                                       qint32 *size,
                                                                       qint32 *offset)
{
    Q_ASSERT(po);
    Q_ASSERT(count);
    Q_ASSERT(indices);

    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);

    if (count && indices) {
        if (type) {
            QVarLengthArray<qint32, 1024> glTypes(count);
            GL_CALL_EXTRA_FUNCTION(glGetActiveUniformsiv(programID, GLsizei(count), indices, GL_UNIFORM_TYPE, glTypes.data()));
            // convert to UIC types
            for (int idx = 0; idx != int(count); ++idx)
                type[idx] = GLConversion::fromShaderGLToPropertyDataTypes(GLenum(glTypes[idx]));
        }
        if (size) {
            GL_CALL_EXTRA_FUNCTION(glGetActiveUniformsiv(programID, GLsizei(count), indices, GL_UNIFORM_SIZE, size));
        }
        if (offset) {
            GL_CALL_EXTRA_FUNCTION(glGetActiveUniformsiv(programID, GLsizei(count), indices, GL_UNIFORM_OFFSET, offset));
        }
    }
}

void QSSGRenderBackendGLES2Impl::programSetConstantBlock(QSSGRenderBackendShaderProgramObject po, quint32 blockIndex, quint32 binding)
{
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);

    GL_CALL_EXTRA_FUNCTION(glUniformBlockBinding(programID, blockIndex, binding));
}

void QSSGRenderBackendGLES2Impl::programSetConstantBuffer(quint32 index, QSSGRenderBackendBufferObject bo)
{
    Q_ASSERT(bo);

    GLuint bufID = HandleToID_cast(GLuint, quintptr, bo);
    GL_CALL_EXTRA_FUNCTION(glBindBufferBase(GL_UNIFORM_BUFFER, index, bufID));
}

QSSGRenderBackend::QSSGRenderBackendQueryObject QSSGRenderBackendGLES2Impl::createQuery()
{
    quint32 glQueryID = 0;

    return reinterpret_cast<QSSGRenderBackendQueryObject>(quintptr(glQueryID));
}

void QSSGRenderBackendGLES2Impl::releaseQuery(QSSGRenderBackendQueryObject) {}

void QSSGRenderBackendGLES2Impl::beginQuery(QSSGRenderBackendQueryObject, QSSGRenderQueryType) {}

void QSSGRenderBackendGLES2Impl::endQuery(QSSGRenderBackendQueryObject, QSSGRenderQueryType) {}

void QSSGRenderBackendGLES2Impl::getQueryResult(QSSGRenderBackendQueryObject, QSSGRenderQueryResultType, quint32 *)
{
}

void QSSGRenderBackendGLES2Impl::getQueryResult(QSSGRenderBackendQueryObject, QSSGRenderQueryResultType, quint64 *)
{
}

void QSSGRenderBackendGLES2Impl::setQueryTimer(QSSGRenderBackendQueryObject) {}

QSSGRenderBackend::QSSGRenderBackendSyncObject QSSGRenderBackendGLES2Impl::createSync(QSSGRenderSyncType, QSSGRenderSyncFlags)
{
    GLsync syncID = nullptr;
    return QSSGRenderBackendSyncObject(syncID);
}

void QSSGRenderBackendGLES2Impl::releaseSync(QSSGRenderBackendSyncObject) {}

void QSSGRenderBackendGLES2Impl::waitSync(QSSGRenderBackendSyncObject, QSSGRenderCommandFlushFlags, quint64) {}

QT_END_NAMESPACE
