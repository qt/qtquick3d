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

#include <QtQuick3DRender/private/qssgrenderbackendgl3_p.h>
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

#define GL_CALL_EXTRA_FUNCTION(x)                                                                                      \
    m_glExtraFunctions->x;                                                                                             \
    RENDER_LOG_ERROR_PARAMS(x);

#if defined(QT_OPENGL_ES)
#define GL_CALL_TIMER_EXT(x)                                                                                           \
    m_QSSGExtensions->x;                                                                                             \
    RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_TESSELATION_EXT(x)                                                                                     \
    m_QSSGExtensions->x;                                                                                             \
    RENDER_LOG_ERROR_PARAMS(x);
#else
#define GL_CALL_TIMER_EXT(x)                                                                                           \
    m_timerExtension->x;                                                                                               \
    RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_TESSELATION_EXT(x)                                                                                     \
    m_tessellationShader->x;                                                                                           \
    RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_MULTISAMPLE_EXT(x)                                                                                     \
    m_multiSample->x;                                                                                                  \
    RENDER_LOG_ERROR_PARAMS(x);
#endif

#ifndef GL_PATCH_VERTICES
#define GL_PATCH_VERTICES 0x8E72
#endif

namespace QSSGGlExtStrings {
QByteArray extsAstcHDR()
{
    return QByteArrayLiteral("GL_KHR_texture_compression_astc_hdr");
}
QByteArray extsAstcLDR()
{
    return QByteArrayLiteral("GL_KHR_texture_compression_astc_ldr");
}
}

/// constructor
QSSGRenderBackendGL3Impl::QSSGRenderBackendGL3Impl(const QSurfaceFormat &format) : QSSGRenderBackendGLBase(format)
{
    // clear support bits
    m_backendSupport.caps.u32Values = 0;

    // get extension count
    GLint numExtensions = 0;
    GL_CALL_EXTRA_FUNCTION(glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions));

    QByteArray extensionBuffer;

    for (qint32 i = 0; i < numExtensions; i++) {
        const GLubyte *glExt = GL_CALL_EXTRA_FUNCTION(glGetStringi(GL_EXTENSIONS, GLuint(i)));
        const QByteArray extensionString(reinterpret_cast<const char *>(glExt));

        m_extensions.push_back(extensionString);

        if (extensionBuffer.size())
            extensionBuffer.append(" ");
        extensionBuffer.append(extensionString);

        // search for extension
        if (!m_backendSupport.caps.bits.bDXTImagesSupported
            && (QSSGGlExtStrings::exts3tc().compare(extensionString) == 0
                || QSSGGlExtStrings::extsdxt().compare(extensionString) == 0)) {
            m_backendSupport.caps.bits.bDXTImagesSupported = true;
        } else if (!m_backendSupport.caps.bits.bAnistropySupported && QSSGGlExtStrings::extsAniso().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bAnistropySupported = true;
        } else if (!m_backendSupport.caps.bits.bFPRenderTargetsSupported
                   && QSSGGlExtStrings::extsFPRenderTarget().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bFPRenderTargetsSupported = true;
        } else if (!m_backendSupport.caps.bits.bTimerQuerySupported
                   && QSSGGlExtStrings::extsTimerQuery().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bTimerQuerySupported = true;
        } else if (!m_backendSupport.caps.bits.bGPUShader5ExtensionSupported
                   && QSSGGlExtStrings::extsGpuShader5().compare(extensionString) == 0) {
            m_backendSupport.caps.bits.bGPUShader5ExtensionSupported = true;
        }
    }

    qCInfo(RENDER_TRACE_INFO, "OpenGL extensions: %s", extensionBuffer.constData());

    // texture swizzle is always true
    m_backendSupport.caps.bits.bTextureSwizzleSupported = true;
    // depthstencil renderbuffer support is always true
    m_backendSupport.caps.bits.bDepthStencilSupported = true;
    // constant buffers support is always true
    m_backendSupport.caps.bits.bConstantBufferSupported = true;
    m_backendSupport.caps.bits.bStandardDerivativesSupported = true;
    m_backendSupport.caps.bits.bVertexArrayObjectSupported = true;
    m_backendSupport.caps.bits.bTextureLodSupported = true;

    if (!isESCompatible()) {
        // render to float textures is always supported on none ES systems which support >=GL3
        m_backendSupport.caps.bits.bFPRenderTargetsSupported = true;
        // multisampled texture is always supported on none ES systems which support >=GL3
        m_backendSupport.caps.bits.bMsTextureSupported = true;
        // timer queries are always supported on none ES systems which support >=GL3
        m_backendSupport.caps.bits.bTimerQuerySupported = true;
    }

    // query hardware
    GL_CALL_EXTRA_FUNCTION(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &m_maxAttribCount));

    // internal state tracker
    m_currentMiscState = new QSSGRenderBackendMiscStateGL();

    // finally setup caps based on device
    setAndInspectHardwareCaps();

    // Initialize extensions
#if defined(QT_OPENGL_ES_2)
    m_QSSGExtensions = new QSSGOpenGLES2Extensions;
    m_QSSGExtensions->initializeOpenGLFunctions();
#else
    m_timerExtension = new QOpenGLExtension_ARB_timer_query;
    m_timerExtension->initializeOpenGLFunctions();
    m_tessellationShader = new QOpenGLExtension_ARB_tessellation_shader;
    m_tessellationShader->initializeOpenGLFunctions();
    m_multiSample = new QOpenGLExtension_ARB_texture_multisample;
    m_multiSample->initializeOpenGLFunctions();
    m_QSSGExtensions = new QSSGOpenGLExtensions;
    m_QSSGExtensions->initializeOpenGLFunctions();
#endif
}
/// destructor
QSSGRenderBackendGL3Impl::~QSSGRenderBackendGL3Impl()
{
    delete m_currentMiscState;
#if !defined(QT_OPENGL_ES_2)
    delete m_timerExtension;
    delete m_tessellationShader;
    delete m_multiSample;
#endif
    delete m_QSSGExtensions;
}

void QSSGRenderBackendGL3Impl::setMultisampledTextureData2D(QSSGRenderBackendTextureObject to,
                                                              QSSGRenderTextureTargetType target,
                                                              qint32 samples,
                                                              QSSGRenderTextureFormat internalFormat,
                                                              qint32 width,
                                                              qint32 height,
                                                              bool fixedsamplelocations)
{
    // Not supported by ES 3 yet
#if defined(QT_OPENGL_ES)
    Q_UNUSED(to)
    Q_UNUSED(target)
    Q_UNUSED(samples)
    Q_UNUSED(internalFormat)
    Q_UNUSED(width)
    Q_UNUSED(height)
    Q_UNUSED(fixedsamplelocations)
#else
    GLuint texID = HandleToID_cast(GLuint, quintptr, to);
    GLenum glTarget = GLConversion::fromTextureTargetToGL(target);
    setActiveTexture(GL_TEXTURE0);
    GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, texID));

    QSSGRenderTextureSwizzleMode swizzleMode = QSSGRenderTextureSwizzleMode::NoSwizzle;
    internalFormat = GLConversion::replaceDeprecatedTextureFormat(getRenderContextType(), internalFormat, swizzleMode);

    GLenum glformat = 0, glInternalFormat = 0, gltype = GL_UNSIGNED_BYTE;

    if (internalFormat.isUncompressedTextureFormat())
        GLConversion::fromUncompressedTextureFormatToGL(getRenderContextType(), internalFormat, glformat, gltype, glInternalFormat);
    else if (internalFormat.isDepthTextureFormat())
        GLConversion::fromDepthTextureFormatToGL(getRenderContextType(), internalFormat, glformat, gltype, glInternalFormat);

    GL_CALL_MULTISAMPLE_EXT(
            glTexImage2DMultisample(glTarget, GLsizei(samples), GLint(glInternalFormat), GLsizei(width), GLsizei(height), fixedsamplelocations));

    GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, 0));
#endif
}

void QSSGRenderBackendGL3Impl::setTextureData3D(QSSGRenderBackendTextureObject to,
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

    if (internalFormat.isUncompressedTextureFormat())
        GLConversion::fromUncompressedTextureFormatToGL(getRenderContextType(), internalFormat, glformat, gltype, glInternalFormat);

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
            glTexImage3D(glTarget, level, GLint(glInternalFormat), GLsizei(width), GLsizei(height), GLsizei(depth), border, glformat, gltype, hostData));

    GL_CALL_EXTRA_FUNCTION(glBindTexture(glTarget, 0));
}

void QSSGRenderBackendGL3Impl::updateSampler(QSSGRenderBackendSamplerObject /* so */,
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

    GLenum glTarget = GLConversion::fromTextureTargetToGL(target);

    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_MIN_FILTER, GLint(m_conversion.fromTextureMinifyingOpToGL(minFilter))));
    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_MAG_FILTER, GLint(m_conversion.fromTextureMagnifyingOpToGL(magFilter))));
    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_WRAP_S, GLint(m_conversion.fromTextureCoordOpToGL(wrapS))));
    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_WRAP_T, GLint(m_conversion.fromTextureCoordOpToGL(wrapT))));
    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_WRAP_R, GLint(m_conversion.fromTextureCoordOpToGL(wrapR))));
    GL_CALL_EXTRA_FUNCTION(glTexParameterf(glTarget, GL_TEXTURE_MIN_LOD, minLod));
    GL_CALL_EXTRA_FUNCTION(glTexParameterf(glTarget, GL_TEXTURE_MAX_LOD, maxLod));
    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_COMPARE_MODE, GLint(m_conversion.fromTextureCompareModeToGL(compareMode))));
    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_COMPARE_FUNC, GLint(m_conversion.fromTextureCompareFuncToGL(compareFunc))));

    if (m_backendSupport.caps.bits.bAnistropySupported) {
        GL_CALL_EXTRA_FUNCTION(glTexParameterf(glTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy));
    }
}

void QSSGRenderBackendGL3Impl::updateTextureObject(QSSGRenderBackendTextureObject to,
                                                     QSSGRenderTextureTargetType target,
                                                     qint32 baseLevel,
                                                     qint32 maxLevel)
{
    Q_UNUSED(to)

    GLenum glTarget = GLConversion::fromTextureTargetToGL(target);

    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_BASE_LEVEL, baseLevel));
    GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_MAX_LEVEL, maxLevel));
}

void QSSGRenderBackendGL3Impl::updateTextureSwizzle(QSSGRenderBackendTextureObject to,
                                                      QSSGRenderTextureTargetType target,
                                                      QSSGRenderTextureSwizzleMode swizzleMode)
{
    Q_UNUSED(to)
    if (m_backendSupport.caps.bits.bTextureSwizzleSupported) {
        GLint glSwizzle[4];
        GLenum glTarget = GLConversion::fromTextureTargetToGL(target);
        GLConversion::NVRenderConvertSwizzleModeToGL(swizzleMode, glSwizzle);
#if defined(QT_OPENGL_ES)
        // since ES3 spec has no GL_TEXTURE_SWIZZLE_RGBA set it separately
        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_SWIZZLE_R, glSwizzle[0]));
        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_SWIZZLE_G, glSwizzle[1]));
        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_SWIZZLE_B, glSwizzle[2]));
        GL_CALL_EXTRA_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_SWIZZLE_A, glSwizzle[3]));
#else
        GL_CALL_EXTRA_FUNCTION(glTexParameteriv(glTarget, GL_TEXTURE_SWIZZLE_RGBA, glSwizzle));
#endif
    }
}

qint32 QSSGRenderBackendGL3Impl::getDepthBits() const
{
    qint32 depthBits;
    GL_CALL_EXTRA_FUNCTION(
            glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &depthBits));

    return depthBits;
}

qint32 QSSGRenderBackendGL3Impl::getStencilBits() const
{
    qint32 stencilBits;
    GL_CALL_EXTRA_FUNCTION(
            glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &stencilBits));

    return stencilBits;
}

void QSSGRenderBackendGL3Impl::generateMipMaps(QSSGRenderBackendTextureObject to,
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

QByteArray QSSGRenderBackendGL3Impl::getShadingLanguageVersion()
{
    Q_ASSERT(m_format.majorVersion() >= 3);

    QByteArray ver("#version 300");
    if (m_format.majorVersion() == 3)
        ver[10] = '0' + char(m_format.minorVersion());
    else if (m_format.majorVersion() > 3)
        ver[10] = '3';

    if (m_format.renderableType() == QSurfaceFormat::OpenGLES)
        ver.append(" es");

    return ver.append("\n");
}

QSSGRenderContextType QSSGRenderBackendGL3Impl::getRenderContextType() const
{
    Q_ASSERT(m_format.majorVersion() >= 3);

    if (m_format.renderableType() == QSurfaceFormat::OpenGLES) {
        if (m_format.minorVersion() >= 1)
            return QSSGRenderContextType::GLES3PLUS;

        return QSSGRenderContextType::GLES3;
    }

    return QSSGRenderContextType::GL3;
}

bool QSSGRenderBackendGL3Impl::setInputAssembler(QSSGRenderBackendInputAssemblerObject iao, QSSGRenderBackendShaderProgramObject po)
{
    if (iao == nullptr) {
        // unbind and return;
        GL_CALL_EXTRA_FUNCTION(glBindVertexArray(0));
        return true;
    }

    QSSGRenderBackendInputAssemblerGL *inputAssembler = reinterpret_cast<QSSGRenderBackendInputAssemblerGL *>(iao);
    QSSGRenderBackendAttributeLayoutGL *attribLayout = inputAssembler->m_attribLayout;
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);
    QSSGDataRef<QSSGRenderBackendShaderInputEntryGL> shaderAttribBuffer;
    if (pProgram->m_shaderInput)
        shaderAttribBuffer = pProgram->m_shaderInput->m_shaderInputEntries;

    // Need to be careful with the attributes. shaderAttribBuffer contains
    // whatever glGetActiveAttrib() returns. There can however be differences
    // between OpenGL implementations: some will optimize out unused
    // attributes, while others could report all attributes as active,
    // regardless of them being used in practice or not.
    //
    // In addition, not binding any data to an attribute is not an error with
    // OpenGL, and in fact is unavoidable when a model, for example, has no UV
    // coordinates (and associated data, such as tangetst and binormals), but
    // is then used with a shader with lighting, shadows, and such. This needs
    // to be handled gracefully. It can lead to incorrect rendering but the
    // object still needs to be there, without bailing out or flooding the
    // output with warnings.

    // if (attribLayout->m_layoutAttribEntries.size() < shaderAttribBuffer.size())

    if (inputAssembler->m_vertexbufferHandles.size() <= attribLayout->m_maxInputSlot)
        return false;

    if (inputAssembler->m_vaoID == 0) {
        // generate vao
        GL_CALL_EXTRA_FUNCTION(glGenVertexArrays(1, &inputAssembler->m_vaoID));
        Q_ASSERT(inputAssembler->m_vaoID);
    }

    // set patch parameter count if changed
    if (m_backendSupport.caps.bits.bTessellationSupported && m_currentMiscState->m_patchVertexCount != inputAssembler->m_patchVertexCount) {
        m_currentMiscState->m_patchVertexCount = inputAssembler->m_patchVertexCount;
#if defined(QT_OPENGL_ES)
        GL_CALL_TESSELATION_EXT(glPatchParameteriEXT(GL_PATCH_VERTICES, inputAssembler->m_patchVertexCount));
#else
        GL_CALL_TESSELATION_EXT(glPatchParameteri(GL_PATCH_VERTICES, GLint(inputAssembler->m_patchVertexCount)));
#endif
    }

    if (inputAssembler->m_cachedShaderHandle != programID) {
        GL_CALL_EXTRA_FUNCTION(glBindVertexArray(inputAssembler->m_vaoID));
        inputAssembler->m_cachedShaderHandle = programID;

        for (const auto &attrib : qAsConst(shaderAttribBuffer)) {
            QSSGRenderBackendLayoutEntryGL *entry = attribLayout->getEntryByName(attrib.m_attribName);

            if (entry) {
                QSSGRenderBackendLayoutEntryGL &entryData(*entry);
                if (Q_UNLIKELY(entryData.m_type != attrib.m_type || entryData.m_numComponents != attrib.m_numComponents)) {
                    qCCritical(RENDER_INVALID_OPERATION, "Attrib %s doesn't match vertex layout", attrib.m_attribName.constData());
                    Q_ASSERT(false);
                    return false;
                }
                entryData.m_attribIndex = attrib.m_attribLocation;
            } else {
                qCWarning(RENDER_WARNING, "Failed to bind attribute %s", attrib.m_attribName.constData());
            }
        }

        // disable max possible used first
        // this is currently sufficient since we always re-arrange input attributes from 0
        for (int i = 0; i < attribLayout->m_layoutAttribEntries.size(); i++)
            GL_CALL_EXTRA_FUNCTION(glDisableVertexAttribArray(GLuint(i)));

        // setup all attribs
        GLuint boundArrayBufferId = 0; // 0 means unbound
        for (int idx = 0; idx != shaderAttribBuffer.size(); ++idx) {
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
        GL_CALL_EXTRA_FUNCTION(glBindVertexArray(inputAssembler->m_vaoID));
    }
#ifdef _DEBUG
    if (inputAssembler->m_vaoID) {
        for (const auto &attrib : qAsConst(shaderAttribBuffer)) {
            QSSGRenderBackendLayoutEntryGL *entry = attribLayout->getEntryByName(attrib.m_attribName);

            if (entry) {
                QSSGRenderBackendLayoutEntryGL &entryData(*entry);
                if (entryData.m_type != attrib.m_type || entryData.m_numComponents != attrib.m_numComponents
                    || entryData.m_attribIndex != attrib.m_attribLocation) {
                    qCCritical(RENDER_INVALID_OPERATION, "Attrib %s doesn't match vertex layout", qPrintable(attrib.m_attribName));
                    Q_ASSERT(false);
                }
            } else {
                qCWarning(RENDER_WARNING, "Failed to bind attribute %s", qPrintable(attrib.m_attribName));
            }
        }
    }
#endif // _DEBUG

    return true;
}

void QSSGRenderBackendGL3Impl::setDrawBuffers(QSSGRenderBackendRenderTargetObject rto, QSSGDataView<qint32> inDrawBufferSet)
{
    Q_UNUSED(rto)

    m_drawBuffersArray.clear();

    for (int idx = 0, end = inDrawBufferSet.size(); idx < end; ++idx) {
        if (inDrawBufferSet[idx] < 0)
            m_drawBuffersArray.push_back(GL_NONE);
        else
            m_drawBuffersArray.push_back(GL_COLOR_ATTACHMENT0 + GLuint(inDrawBufferSet[idx]));
    }

    GL_CALL_EXTRA_FUNCTION(glDrawBuffers(m_drawBuffersArray.size(), m_drawBuffersArray.data()));
}

void QSSGRenderBackendGL3Impl::setReadBuffer(QSSGRenderBackendRenderTargetObject rto, QSSGReadFace inReadFace)
{
    Q_UNUSED(rto)

    GL_CALL_EXTRA_FUNCTION(glReadBuffer(m_conversion.fromReadFacesToGL(inReadFace)));
}

void QSSGRenderBackendGL3Impl::renderTargetAttach(QSSGRenderBackendRenderTargetObject,
                                                    QSSGRenderFrameBufferAttachment attachment,
                                                    QSSGRenderBackendTextureObject to,
                                                    qint32 level,
                                                    qint32 layer)
{
    // rto must be the current render target
    GLuint texID = HandleToID_cast(GLuint, quintptr, to);

    GLenum glAttach = GLConversion::fromFramebufferAttachmentsToGL(attachment);

    GL_CALL_EXTRA_FUNCTION(glFramebufferTextureLayer(GL_FRAMEBUFFER, glAttach, texID, level, layer))
}

void QSSGRenderBackendGL3Impl::setReadTarget(QSSGRenderBackendRenderTargetObject rto)
{
    GLuint fboID = HandleToID_cast(GLuint, quintptr, rto);

    GL_CALL_EXTRA_FUNCTION(glBindFramebuffer(GL_READ_FRAMEBUFFER, fboID));
}

void QSSGRenderBackendGL3Impl::blitFramebuffer(qint32 srcX0,
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

void QSSGRenderBackendGL3Impl::copyFramebufferTexture(qint32 srcX0,
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

void *QSSGRenderBackendGL3Impl::mapBuffer(QSSGRenderBackendBufferObject,
                                            QSSGRenderBufferType bindFlags,
                                            size_t offset,
                                            size_t length,
                                            QSSGRenderBufferAccessFlags accessFlags)
{
    void *ret = nullptr;
    ret = GL_CALL_EXTRA_FUNCTION(glMapBufferRange(m_conversion.fromBindBufferFlagsToGL(bindFlags),
                                                  GLintptr(offset),
                                                  GLintptr(length),
                                                  m_conversion.fromBufferAccessBitToGL(accessFlags)));

    return ret;
}

bool QSSGRenderBackendGL3Impl::unmapBuffer(QSSGRenderBackendBufferObject, QSSGRenderBufferType bindFlags)
{
    const GLboolean ret = GL_CALL_EXTRA_FUNCTION(glUnmapBuffer(m_conversion.fromBindBufferFlagsToGL(bindFlags)));
    return (ret != 0);
}

qint32 QSSGRenderBackendGL3Impl::getConstantBufferCount(QSSGRenderBackendShaderProgramObject po)
{
    Q_ASSERT(po);
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);

    GLint numUniformBuffers;
    GL_CALL_EXTRA_FUNCTION(glGetProgramiv(programID, GL_ACTIVE_UNIFORM_BLOCKS, &numUniformBuffers));

    return numUniformBuffers;
}

qint32 QSSGRenderBackendGL3Impl::getConstantBufferInfoByID(QSSGRenderBackendShaderProgramObject po,
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

void QSSGRenderBackendGL3Impl::getConstantBufferParamIndices(QSSGRenderBackendShaderProgramObject po, quint32 id, qint32 *indices)
{
    Q_ASSERT(po);
    Q_ASSERT(indices);

    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);

    if (indices) {
        GL_CALL_EXTRA_FUNCTION(glGetActiveUniformBlockiv(programID, id, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, indices));
    }
}

void QSSGRenderBackendGL3Impl::getConstantBufferParamInfoByIndices(QSSGRenderBackendShaderProgramObject po,
                                                                   quint32 count,
                                                                   quint32 *indices,
                                                                   QSSGRenderShaderDataType *type,
                                                                   qint32 *size,
                                                                   qint32 *offset)
{
    Q_ASSERT(po);
    Q_ASSERT(count && count <= INT32_MAX);
    Q_ASSERT(indices);

    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);

    if (count && indices) {
        if (type) {
            QVarLengthArray<qint32, 1024> glTypes(count);
            GL_CALL_EXTRA_FUNCTION(glGetActiveUniformsiv(programID, GLsizei(count), indices, GL_UNIFORM_TYPE, glTypes.data()));
            // convert to UIC types
            for (qint32 idx = 0; idx != qint32(count); ++idx)
                type[idx] = GLConversion::fromShaderGLToPropertyDataTypes(GLenum(glTypes[idx]));
        }
        if (size)
            GL_CALL_EXTRA_FUNCTION(glGetActiveUniformsiv(programID, GLsizei(count), indices, GL_UNIFORM_SIZE, size));
        if (offset)
            GL_CALL_EXTRA_FUNCTION(glGetActiveUniformsiv(programID, GLsizei(count), indices, GL_UNIFORM_OFFSET, offset));
    }
}

void QSSGRenderBackendGL3Impl::programSetConstantBlock(QSSGRenderBackendShaderProgramObject po, quint32 blockIndex, quint32 binding)
{
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);

    GL_CALL_EXTRA_FUNCTION(glUniformBlockBinding(programID, blockIndex, binding));
}

void QSSGRenderBackendGL3Impl::programSetConstantBuffer(quint32 index, QSSGRenderBackendBufferObject bo)
{
    Q_ASSERT(bo);

    GLuint bufID = HandleToID_cast(GLuint, quintptr, bo);
    GL_CALL_EXTRA_FUNCTION(glBindBufferBase(GL_UNIFORM_BUFFER, index, bufID));
}

QSSGRenderBackend::QSSGRenderBackendQueryObject QSSGRenderBackendGL3Impl::createQuery()
{
    quint32 glQueryID = 0;

    GL_CALL_EXTRA_FUNCTION(glGenQueries(1, &glQueryID));

    return reinterpret_cast<QSSGRenderBackendQueryObject>(quintptr(glQueryID));
}

void QSSGRenderBackendGL3Impl::releaseQuery(QSSGRenderBackendQueryObject qo)
{
    GLuint queryID = HandleToID_cast(GLuint, quintptr, qo);

    GL_CALL_EXTRA_FUNCTION(glDeleteQueries(1, &queryID));
}

void QSSGRenderBackendGL3Impl::beginQuery(QSSGRenderBackendQueryObject qo, QSSGRenderQueryType type)
{
    GLuint queryID = HandleToID_cast(GLuint, quintptr, qo);

    GL_CALL_EXTRA_FUNCTION(glBeginQuery(m_conversion.fromQueryTypeToGL(type), queryID));
}

void QSSGRenderBackendGL3Impl::endQuery(QSSGRenderBackendQueryObject, QSSGRenderQueryType type)
{
    GL_CALL_EXTRA_FUNCTION(glEndQuery(m_conversion.fromQueryTypeToGL(type)));
}

void QSSGRenderBackendGL3Impl::getQueryResult(QSSGRenderBackendQueryObject qo,
                                                QSSGRenderQueryResultType resultType,
                                                quint32 *params)
{
    GLuint queryID = HandleToID_cast(GLuint, quintptr, qo);

    if (params)
        GL_CALL_EXTRA_FUNCTION(glGetQueryObjectuiv(queryID, m_conversion.fromQueryResultTypeToGL(resultType), params));
}

void QSSGRenderBackendGL3Impl::getQueryResult(QSSGRenderBackendQueryObject qo,
                                                QSSGRenderQueryResultType resultType,
                                                quint64 *params)
{
    // TODO: params type!
    if (m_backendSupport.caps.bits.bTimerQuerySupported) {
        GLuint queryID = HandleToID_cast(GLuint, quintptr, qo);

        if (params)
#if defined(QT_OPENGL_ES)
            GL_CALL_TIMER_EXT(glGetQueryObjectui64vEXT(queryID, m_conversion.fromQueryResultTypeToGL(resultType), reinterpret_cast<GLuint64 *>(params)));
#else
            GL_CALL_TIMER_EXT(glGetQueryObjectui64v(queryID, m_conversion.fromQueryResultTypeToGL(resultType), reinterpret_cast<GLuint64 *>(params)));
#endif
    }
}

void QSSGRenderBackendGL3Impl::setQueryTimer(QSSGRenderBackendQueryObject qo)
{
    if (m_backendSupport.caps.bits.bTimerQuerySupported) {
        GLuint queryID = HandleToID_cast(GLuint, quintptr, qo);
#if defined(QT_OPENGL_ES)
        GL_CALL_TIMER_EXT(glQueryCounterEXT(queryID, GL_TIMESTAMP));
#else
        GL_CALL_TIMER_EXT(glQueryCounter(queryID, GL_TIMESTAMP));
#endif
    }
}

QSSGRenderBackend::QSSGRenderBackendSyncObject QSSGRenderBackendGL3Impl::createSync(QSSGRenderSyncType syncType,
                                                                                          QSSGRenderSyncFlags)
{
    GLsync syncID = nullptr;

    syncID = GL_CALL_EXTRA_FUNCTION(glFenceSync(m_conversion.fromSyncTypeToGL(syncType), 0));

    return QSSGRenderBackendSyncObject(syncID);
}

void QSSGRenderBackendGL3Impl::releaseSync(QSSGRenderBackendSyncObject so)
{
    GLsync syncID = GLsync(so);

    GL_CALL_EXTRA_FUNCTION(glDeleteSync(syncID));
}

void QSSGRenderBackendGL3Impl::waitSync(QSSGRenderBackendSyncObject so, QSSGRenderCommandFlushFlags, quint64)
{
    GLsync syncID = GLsync(so);

    GL_CALL_EXTRA_FUNCTION(glWaitSync(syncID, 0, GL_TIMEOUT_IGNORED));
}

void QSSGRenderBackendGL3Impl::releaseInputAssembler(QSSGRenderBackendInputAssemblerObject iao)
{
    QSSGRenderBackendInputAssemblerGL *inputAssembler = reinterpret_cast<QSSGRenderBackendInputAssemblerGL *>(iao);
    GL_CALL_EXTRA_FUNCTION(glDeleteVertexArrays(1, &inputAssembler->m_vaoID));
    delete inputAssembler;
}

QT_END_NAMESPACE


