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

#include <QtQuick3DRender/private/qssgrenderbackendglbase_p.h>
#include <QtQuick3DRender/private/qssgrenderbackendinputassemblergl_p.h>
#include <QtQuick3DRender/private/qssgrenderbackendshaderprogramgl_p.h>
#include <QtQuick3DRender/private/qssgrenderbackendrenderstatesgl_p.h>

QT_BEGIN_NAMESPACE

#ifdef RENDER_BACKEND_LOG_GL_ERRORS
#define RENDER_LOG_ERROR_PARAMS(x) checkGLError(#x, __FILE__, __LINE__)
#else
#define RENDER_LOG_ERROR_PARAMS(x) checkGLError()
#endif

#define GL_CALL_FUNCTION(x)                                                                                            \
    m_glFunctions->x;                                                                                                  \
    RENDER_LOG_ERROR_PARAMS(x);
#define GL_CALL_EXTRA_FUNCTION(x)                                                                                      \
    m_glExtraFunctions->x;                                                                                             \
    RENDER_LOG_ERROR_PARAMS(x);

#ifndef GL_PROGRAM_SEPARABLE
#define GL_PROGRAM_SEPARABLE 0x8258
#endif

#ifndef GL_UNSIGNED_INT_IMAGE_2D
#define GL_UNSIGNED_INT_IMAGE_2D 0x9063
#endif

#ifndef GL_UNSIGNED_INT_ATOMIC_COUNTER
#define GL_UNSIGNED_INT_ATOMIC_COUNTER 0x92DB
#endif

#ifndef GL_PROGRAM_BINARY_LENGTH
#define GL_PROGRAM_BINARY_LENGTH 0x8741
#endif

namespace QSSGGlExtStrings {
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
}

/// constructor
QSSGRenderBackendGLBase::QSSGRenderBackendGLBase(const QSurfaceFormat &format)
    : m_conversion(), m_maxAttribCount(0), m_format(format)
{
    m_glFunctions = new QOpenGLFunctions;
    m_glFunctions->initializeOpenGLFunctions();
    m_glExtraFunctions = new QOpenGLExtraFunctions;
    m_glExtraFunctions->initializeOpenGLFunctions();

    const QByteArray languageVersion = getShadingLanguageVersionString();
    qCInfo(RENDER_TRACE_INFO, "GLSL version: %s", languageVersion.constData());

    const QByteArray apiVersion(getVersionString());
    qCInfo(RENDER_TRACE_INFO, "GL version: %s", apiVersion.constData());

    const QByteArray apiVendor(getVendorString());
    qCInfo(RENDER_TRACE_INFO, "HW vendor: %s", apiVendor.constData());

    const QByteArray apiRenderer(getRendererString());
    qCInfo(RENDER_TRACE_INFO, "Vendor renderer: %s", apiRenderer.constData());

    // internal state tracker
    m_currentRasterizerState = new QSSGRenderBackendRasterizerStateGL();
    m_currentDepthStencilState = new QSSGRenderBackendDepthStencilStateGL();
}
/// destructor
QSSGRenderBackendGLBase::~QSSGRenderBackendGLBase()
{
    delete m_currentRasterizerState;
    delete m_currentDepthStencilState;
    delete m_glFunctions;
    delete m_glExtraFunctions;
}

QSSGRenderContextType QSSGRenderBackendGLBase::getRenderContextType() const
{
    if (m_format.renderableType() == QSurfaceFormat::OpenGLES) {
        if (m_format.majorVersion() == 2)
            return QSSGRenderContextType::GLES2;

        if (m_format.majorVersion() == 3) {
            if (m_format.minorVersion() >= 1)
                return QSSGRenderContextType::GLES3PLUS;
            return QSSGRenderContextType::GLES3;
        }
    } else if (m_format.majorVersion() == 2) {
        return QSSGRenderContextType::GL2;
    } else if (m_format.majorVersion() == 3) {
        return QSSGRenderContextType::GL3;
    } else if (m_format.majorVersion() == 4) {
        return QSSGRenderContextType::GL4;
    }

    return QSSGRenderContextType::NullContext;
}

bool QSSGRenderBackendGLBase::isESCompatible() const
{
    return m_format.renderableType() == QSurfaceFormat::OpenGLES;
}

QByteArray QSSGRenderBackendGLBase::getShadingLanguageVersion()
{
    QByteArray ver;
    QTextStream stream(&ver);
    stream << "#version ";
    const int minor = m_format.minorVersion();
    switch (getRenderContextType()) {
    case QSSGRenderContextType::GLES2:
        stream << "1" << minor << "0\n";
        break;
    case QSSGRenderContextType::GL2:
        stream << "1" << minor << "0\n";
        break;
    case QSSGRenderContextType::GLES3PLUS:
    case QSSGRenderContextType::GLES3:
        stream << "3" << minor << "0 es\n";
        break;
    case QSSGRenderContextType::GL3:
        if (minor == 3)
            stream << "3" << minor << "0\n";
        else
            stream << "1" << 3 + minor << "0\n";
        break;
    case QSSGRenderContextType::GL4:
        stream << "4" << minor << "0\n";
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    return ver;
}

qint32 QSSGRenderBackendGLBase::getMaxCombinedTextureUnits()
{
    qint32 maxUnits;
    GL_CALL_FUNCTION(glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxUnits));
    return maxUnits;
}

bool QSSGRenderBackendGLBase::getRenderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps inCap) const
{
    bool bSupported = false;

    switch (inCap) {
    case QSSGRenderBackendCaps::FpRenderTarget:
        bSupported = m_backendSupport.caps.bits.bFPRenderTargetsSupported;
        break;
    case QSSGRenderBackendCaps::DepthStencilTexture:
        bSupported = m_backendSupport.caps.bits.bDepthStencilSupported;
        break;
    case QSSGRenderBackendCaps::ConstantBuffer:
        bSupported = m_backendSupport.caps.bits.bConstantBufferSupported;
        break;
    case QSSGRenderBackendCaps::DxtImages:
        bSupported = m_backendSupport.caps.bits.bDXTImagesSupported;
        break;
    case QSSGRenderBackendCaps::MsTexture:
        bSupported = m_backendSupport.caps.bits.bMsTextureSupported;
        break;
    case QSSGRenderBackendCaps::TexSwizzle:
        bSupported = m_backendSupport.caps.bits.bTextureSwizzleSupported;
        break;
    case QSSGRenderBackendCaps::FastBlits:
        bSupported = m_backendSupport.caps.bits.bFastBlitsSupported;
        break;
    case QSSGRenderBackendCaps::Tessellation:
        bSupported = m_backendSupport.caps.bits.bTessellationSupported;
        break;
    case QSSGRenderBackendCaps::Compute:
        bSupported = m_backendSupport.caps.bits.bComputeSupported;
        break;
    case QSSGRenderBackendCaps::Geometry:
        bSupported = m_backendSupport.caps.bits.bGeometrySupported;
        break;
    case QSSGRenderBackendCaps::SampleQuery: {
        // On the following context sample query is not supported
        QSSGRenderContextTypes noSamplesQuerySupportedContextFlags(QSSGRenderContextType::GL2 | QSSGRenderContextType::GLES2);
        QSSGRenderContextType ctxType = getRenderContextType();
        bSupported = !(noSamplesQuerySupportedContextFlags & ctxType);
    } break;
    case QSSGRenderBackendCaps::TimerQuery:
        bSupported = m_backendSupport.caps.bits.bTimerQuerySupported;
        break;
    case QSSGRenderBackendCaps::CommandSync: {
        // On the following context sync objects are not supported
        QSSGRenderContextTypes noSyncObjectSupportedContextFlags(QSSGRenderContextType::GL2 | QSSGRenderContextType::GLES2);
        QSSGRenderContextType ctxType = getRenderContextType();
        bSupported = !(noSyncObjectSupportedContextFlags & ctxType);
    } break;
    case QSSGRenderBackendCaps::TextureArray: {
        // On the following context texture arrays are not supported
        QSSGRenderContextTypes noTextureArraySupportedContextFlags(QSSGRenderContextType::GL2 | QSSGRenderContextType::GLES2);
        QSSGRenderContextType ctxType = getRenderContextType();
        bSupported = !(noTextureArraySupportedContextFlags& ctxType);
    } break;
    case QSSGRenderBackendCaps::StorageBuffer:
        bSupported = m_backendSupport.caps.bits.bStorageBufferSupported;
        break;
    case QSSGRenderBackendCaps::ShaderImageLoadStore:
        bSupported = m_backendSupport.caps.bits.bShaderImageLoadStoreSupported;
        break;
    case QSSGRenderBackendCaps::ProgramPipeline:
        bSupported = m_backendSupport.caps.bits.bProgramPipelineSupported;
        break;
    case QSSGRenderBackendCaps::AdvancedBlend:
        bSupported = m_backendSupport.caps.bits.bNVAdvancedBlendSupported | m_backendSupport.caps.bits.bKHRAdvancedBlendSupported;
        break;
    case QSSGRenderBackendCaps::AdvancedBlendKHR:
        bSupported = m_backendSupport.caps.bits.bKHRAdvancedBlendSupported;
        break;
    case QSSGRenderBackendCaps::BlendCoherency:
        bSupported = m_backendSupport.caps.bits.bNVBlendCoherenceSupported | m_backendSupport.caps.bits.bKHRBlendCoherenceSupported;
        break;
    case QSSGRenderBackendCaps::gpuShader5:
        bSupported = m_backendSupport.caps.bits.bGPUShader5ExtensionSupported;
        break;
    case QSSGRenderBackendCaps::VertexArrayObject:
        bSupported = m_backendSupport.caps.bits.bVertexArrayObjectSupported;
        break;
    case QSSGRenderBackendCaps::StandardDerivatives:
        bSupported = m_backendSupport.caps.bits.bStandardDerivativesSupported;
        break;
    case QSSGRenderBackendCaps::TextureLod:
        bSupported = m_backendSupport.caps.bits.bTextureLodSupported;
        break;
    default:
        Q_ASSERT(false);
        bSupported = false;
        break;
    }

    return bSupported;
}

void QSSGRenderBackendGLBase::getRenderBackendValue(QSSGRenderBackendQuery inQuery, qint32 *params) const
{
    if (params) {
        switch (inQuery) {
        case QSSGRenderBackendQuery::MaxTextureSize:
            GL_CALL_FUNCTION(glGetIntegerv(GL_MAX_TEXTURE_SIZE, params));
            break;
        case QSSGRenderBackendQuery::MaxTextureArrayLayers: {
            QSSGRenderContextTypes noTextureArraySupportedContextFlags(QSSGRenderContextType::GL2
                                                                        | QSSGRenderContextType::GLES2);
            QSSGRenderContextType ctxType = getRenderContextType();
            if (!(noTextureArraySupportedContextFlags & ctxType)) {
                GL_CALL_FUNCTION(glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, params));
            } else {
                *params = 0;
            }
        } break;
        case QSSGRenderBackendQuery::MaxConstantBufferSlots: {
            QSSGRenderContextTypes noConstantBufferSupportedContextFlags(QSSGRenderContextType::GL2
                                                                          | QSSGRenderContextType::GLES2);
            QSSGRenderContextType ctxType = getRenderContextType();
            if (!(noConstantBufferSupportedContextFlags & ctxType)) {
                GL_CALL_FUNCTION(glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, params));
            } else {
                *params = 0;
            }
        } break;
        case QSSGRenderBackendQuery::MaxConstantBufferBlockSize: {
            QSSGRenderContextTypes noConstantBufferSupportedContextFlags(QSSGRenderContextType::GL2
                                                                          | QSSGRenderContextType::GLES2);
            QSSGRenderContextType ctxType = getRenderContextType();
            if (!(noConstantBufferSupportedContextFlags & ctxType)) {
                GL_CALL_FUNCTION(glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, params));
            } else {
                *params = 0;
            }
        } break;
        default:
            Q_ASSERT(false);
            *params = 0;
            break;
        }
    }
}

qint32 QSSGRenderBackendGLBase::getDepthBits() const
{
    qint32 depthBits;
    GL_CALL_FUNCTION(glGetIntegerv(GL_DEPTH_BITS, &depthBits));
    return depthBits;
}

qint32 QSSGRenderBackendGLBase::getStencilBits() const
{
    qint32 stencilBits;
    GL_CALL_FUNCTION(glGetIntegerv(GL_STENCIL_BITS, &stencilBits));
    return stencilBits;
}

qint32 QSSGRenderBackendGLBase::getMaxSamples() const
{
    qint32 maxSamples;
    GL_CALL_FUNCTION(glGetIntegerv(GL_MAX_SAMPLES, &maxSamples));
    return maxSamples;
}

void QSSGRenderBackendGLBase::setMultisample(bool bEnable)
{
    Q_ASSERT(m_backendSupport.caps.bits.bMsTextureSupported || !bEnable);
    // For GL ES explicit multisample enabling is not needed
    // and does not exist
    QSSGRenderContextTypes noMsaaEnableContextFlags(QSSGRenderContextType::GLES2 | QSSGRenderContextType::GLES3
                                                     | QSSGRenderContextType::GLES3PLUS);
    QSSGRenderContextType ctxType = getRenderContextType();
    if (!(noMsaaEnableContextFlags & ctxType)) {
        setRenderState(bEnable, QSSGRenderState::Multisample);
    }
}

void QSSGRenderBackendGLBase::setRenderState(bool bEnable, const QSSGRenderState value)
{
    if (value == QSSGRenderState::DepthWrite) {
        GL_CALL_FUNCTION(glDepthMask(bEnable));
    } else {
        if (bEnable) {
            GL_CALL_FUNCTION(glEnable(m_conversion.fromRenderStateToGL(value)));
        } else {
            GL_CALL_FUNCTION(glDisable(m_conversion.fromRenderStateToGL(value)));
        }
    }
}

QSSGRenderBackend::QSSGRenderBackendDepthStencilStateObject QSSGRenderBackendGLBase::createDepthStencilState(
        bool enableDepth,
        bool depthMask,
        QSSGRenderBoolOp depthFunc,
        bool enableStencil,
        QSSGRenderStencilFunction &stencilFuncFront,
        QSSGRenderStencilFunction &stencilFuncBack,
        QSSGRenderStencilOperation &depthStencilOpFront,
        QSSGRenderStencilOperation &depthStencilOpBack)
{
    QSSGRenderBackendDepthStencilStateGL *retval = new QSSGRenderBackendDepthStencilStateGL(enableDepth,
                                                                                                depthMask,
                                                                                                depthFunc,
                                                                                                enableStencil,
                                                                                                stencilFuncFront,
                                                                                                stencilFuncBack,
                                                                                                depthStencilOpFront,
                                                                                                depthStencilOpBack);

    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendDepthStencilStateObject>(retval);
}

void QSSGRenderBackendGLBase::releaseDepthStencilState(QSSGRenderBackendDepthStencilStateObject inDepthStencilState)
{
    QSSGRenderBackendDepthStencilStateGL *inputState = reinterpret_cast<QSSGRenderBackendDepthStencilStateGL *>(inDepthStencilState);
    delete inputState;
}

QSSGRenderBackend::QSSGRenderBackendRasterizerStateObject QSSGRenderBackendGLBase::createRasterizerState(float depthBias,
                                                                                                         float depthScale)
{
    QSSGRenderBackendRasterizerStateGL *retval = new QSSGRenderBackendRasterizerStateGL(depthBias, depthScale);

    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendRasterizerStateObject>(retval);
}

void QSSGRenderBackendGLBase::releaseRasterizerState(QSSGRenderBackendRasterizerStateObject rasterizerState)
{
    delete reinterpret_cast<QSSGRenderBackendRasterizerStateGL *>(rasterizerState);
}

void QSSGRenderBackendGLBase::setDepthStencilState(QSSGRenderBackendDepthStencilStateObject inDepthStencilState)
{
    QSSGRenderBackendDepthStencilStateGL *inputState = reinterpret_cast<QSSGRenderBackendDepthStencilStateGL *>(inDepthStencilState);
    if (inputState && !(*m_currentDepthStencilState == *inputState)) {
        // we check on a per single state base
        if (inputState->m_depthEnable != m_currentDepthStencilState->m_depthEnable) {
            setRenderState(inputState->m_depthEnable, QSSGRenderState::DepthTest);
            m_currentDepthStencilState->m_depthEnable = inputState->m_depthEnable;
        }
        if (inputState->m_stencilEnable != m_currentDepthStencilState->m_stencilEnable) {
            setRenderState(inputState->m_stencilEnable, QSSGRenderState::StencilTest);
            m_currentDepthStencilState->m_stencilEnable = inputState->m_stencilEnable;
        }

        if (inputState->m_depthMask != m_currentDepthStencilState->m_depthMask) {
            GL_CALL_FUNCTION(glDepthMask(inputState->m_depthMask));
            m_currentDepthStencilState->m_depthMask = inputState->m_depthMask;
        }

        if (inputState->m_depthFunc != m_currentDepthStencilState->m_depthFunc) {
            GL_CALL_FUNCTION(glDepthFunc(m_conversion.fromBoolOpToGL(inputState->m_depthFunc)));
            m_currentDepthStencilState->m_depthFunc = inputState->m_depthFunc;
        }

        if (!(inputState->m_depthStencilOpFront == m_currentDepthStencilState->m_depthStencilOpFront)) {
            GL_CALL_FUNCTION(
                    glStencilOpSeparate(GL_FRONT,
                                        m_conversion.fromStencilOpToGL(inputState->m_depthStencilOpFront.m_stencilFail),
                                        m_conversion.fromStencilOpToGL(inputState->m_depthStencilOpFront.m_depthFail),
                                        m_conversion.fromStencilOpToGL(inputState->m_depthStencilOpFront.m_depthPass)));
            m_currentDepthStencilState->m_depthStencilOpFront = inputState->m_depthStencilOpFront;
        }

        if (!(inputState->m_depthStencilOpBack == m_currentDepthStencilState->m_depthStencilOpBack)) {
            GL_CALL_FUNCTION(glStencilOpSeparate(GL_BACK,
                                                 m_conversion.fromStencilOpToGL(inputState->m_depthStencilOpBack.m_stencilFail),
                                                 m_conversion.fromStencilOpToGL(inputState->m_depthStencilOpBack.m_depthFail),
                                                 m_conversion.fromStencilOpToGL(inputState->m_depthStencilOpBack.m_depthPass)));
            m_currentDepthStencilState->m_depthStencilOpBack = inputState->m_depthStencilOpBack;
        }

        if (!(inputState->m_stencilFuncFront == m_currentDepthStencilState->m_stencilFuncFront)) {
            GL_CALL_FUNCTION(glStencilFuncSeparate(GL_FRONT,
                                                   m_conversion.fromBoolOpToGL(inputState->m_stencilFuncFront.m_function),
                                                   inputState->m_stencilFuncFront.m_referenceValue,
                                                   inputState->m_stencilFuncFront.m_mask));
            m_currentDepthStencilState->m_stencilFuncFront = inputState->m_stencilFuncFront;
        }

        if (!(inputState->m_stencilFuncBack == m_currentDepthStencilState->m_stencilFuncBack)) {
            GL_CALL_FUNCTION(glStencilFuncSeparate(GL_BACK,
                                                   m_conversion.fromBoolOpToGL(inputState->m_stencilFuncBack.m_function),
                                                   inputState->m_stencilFuncBack.m_referenceValue,
                                                   inputState->m_stencilFuncBack.m_mask));
            m_currentDepthStencilState->m_stencilFuncBack = inputState->m_stencilFuncBack;
        }
    }
}

void QSSGRenderBackendGLBase::setRasterizerState(QSSGRenderBackendRasterizerStateObject rasterizerState)
{
    QSSGRenderBackendRasterizerStateGL *inputState = (QSSGRenderBackendRasterizerStateGL *)rasterizerState;
    if (inputState && !(*m_currentRasterizerState == *inputState)) {
        // store current state
        *m_currentRasterizerState = *inputState;

        if (m_currentRasterizerState->m_depthBias != 0.0f || m_currentRasterizerState->m_depthScale != 0.0f) {
            GL_CALL_FUNCTION(glEnable(GL_POLYGON_OFFSET_FILL));
        } else {
            GL_CALL_FUNCTION(glDisable(GL_POLYGON_OFFSET_FILL));
        }

        GL_CALL_FUNCTION(glPolygonOffset(m_currentRasterizerState->m_depthBias, m_currentRasterizerState->m_depthScale));
    }
}

bool QSSGRenderBackendGLBase::getRenderState(const QSSGRenderState value)
{
    bool enabled = GL_CALL_FUNCTION(glIsEnabled(m_conversion.fromRenderStateToGL(value)));
    return enabled;
}

QSSGRenderBoolOp QSSGRenderBackendGLBase::getDepthFunc()
{
    qint32 value;
    GL_CALL_FUNCTION(glGetIntegerv(GL_DEPTH_FUNC, &value));
    return GLConversion::fromGLToBoolOp(value);
}

void QSSGRenderBackendGLBase::setDepthFunc(const QSSGRenderBoolOp func)
{
    GL_CALL_FUNCTION(glDepthFunc(m_conversion.fromBoolOpToGL(func)));
}

bool QSSGRenderBackendGLBase::getDepthWrite()
{
    qint32 value;
    GL_CALL_FUNCTION(glGetIntegerv(GL_DEPTH_WRITEMASK, reinterpret_cast<GLint *>(&value)));
    return (value != 0);
}

void QSSGRenderBackendGLBase::setDepthWrite(bool bEnable)
{
    GL_CALL_FUNCTION(glDepthMask(bEnable));
}

void QSSGRenderBackendGLBase::setColorWrites(bool bRed, bool bGreen, bool bBlue, bool bAlpha)
{
    GL_CALL_FUNCTION(glColorMask(bRed, bGreen, bBlue, bAlpha));
}

void QSSGRenderBackendGLBase::getBlendFunc(QSSGRenderBlendFunctionArgument *pBlendFuncArg)
{
    Q_ASSERT(pBlendFuncArg);
    qint32_4 values;

    GL_CALL_FUNCTION(glGetIntegerv(GL_BLEND_SRC_RGB, reinterpret_cast<GLint *>(&values.x)));
    GL_CALL_FUNCTION(glGetIntegerv(GL_BLEND_SRC_ALPHA, reinterpret_cast<GLint *>(&values.y)));
    GL_CALL_FUNCTION(glGetIntegerv(GL_BLEND_DST_RGB, reinterpret_cast<GLint *>(&values.z)));
    GL_CALL_FUNCTION(glGetIntegerv(GL_BLEND_DST_ALPHA, reinterpret_cast<GLint *>(&values.w)));

    pBlendFuncArg->m_srcRgb = GLConversion::fromGLToSrcBlendFunc(values.x);
    pBlendFuncArg->m_srcAlpha = GLConversion::fromGLToSrcBlendFunc(values.y);
    pBlendFuncArg->m_dstRgb = GLConversion::fromGLToDstBlendFunc(values.z);
    pBlendFuncArg->m_dstAlpha = GLConversion::fromGLToDstBlendFunc(values.w);
}

void QSSGRenderBackendGLBase::setBlendFunc(const QSSGRenderBlendFunctionArgument &blendFuncArg)
{
    qint32_4 values;

    values.x = GLConversion::fromSrcBlendFuncToGL(blendFuncArg.m_srcRgb);
    values.y = GLConversion::fromDstBlendFuncToGL(blendFuncArg.m_dstRgb);
    values.z = GLConversion::fromSrcBlendFuncToGL(blendFuncArg.m_srcAlpha);
    values.w = GLConversion::fromDstBlendFuncToGL(blendFuncArg.m_dstAlpha);

    GL_CALL_FUNCTION(glBlendFuncSeparate(values.x, values.y, values.z, values.w));
}

void QSSGRenderBackendGLBase::setBlendEquation(const QSSGRenderBlendEquationArgument &)
{
    // needs GL4 / GLES 3.1
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void QSSGRenderBackendGLBase::setBlendBarrier()
{
    // needs GL4 / GLES 3.1
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

QSSGCullFaceMode QSSGRenderBackendGLBase::getCullFaceMode()
{
    GLint value;
    GL_CALL_FUNCTION(glGetIntegerv(GL_CULL_FACE_MODE, &value));
    return GLConversion::fromGLToCullFaceMode(static_cast<GLenum>(value));
}

void QSSGRenderBackendGLBase::setCullFaceMode(const QSSGCullFaceMode cullFaceMode)
{
    GL_CALL_FUNCTION(glCullFace(GLConversion::fromCullFaceModeToGL(cullFaceMode)));
}

void QSSGRenderBackendGLBase::getScissorRect(QRect *pRect)
{
    Q_ASSERT(pRect);
    GL_CALL_FUNCTION(glGetIntegerv(GL_SCISSOR_BOX, reinterpret_cast<GLint *>(pRect)));
}

void QSSGRenderBackendGLBase::setScissorRect(const QRect &rect)
{
    GL_CALL_FUNCTION(glScissor(rect.x(), rect.y(), rect.width(), rect.height()));
}

void QSSGRenderBackendGLBase::getViewportRect(QRect *pRect)
{
    Q_ASSERT(pRect);
    GL_CALL_FUNCTION(glGetIntegerv(GL_VIEWPORT, reinterpret_cast<GLint *>(pRect)));
}

void QSSGRenderBackendGLBase::setViewportRect(const QRect &rect)
{
    GL_CALL_FUNCTION(glViewport(rect.x(), rect.y(), rect.width(), rect.height()););
}

void QSSGRenderBackendGLBase::setClearColor(const QVector4D *pClearColor)
{
    Q_ASSERT(pClearColor);

    GL_CALL_FUNCTION(glClearColor(pClearColor->x(), pClearColor->y(), pClearColor->z(), pClearColor->w()));
}

void QSSGRenderBackendGLBase::clear(QSSGRenderClearFlags flags)
{
    GL_CALL_FUNCTION(glClear(m_conversion.fromClearFlagsToGL(flags)));
}

QSSGRenderBackend::QSSGRenderBackendBufferObject QSSGRenderBackendGLBase::createBuffer(QSSGRenderBufferType bindFlags,
                                                                                             QSSGRenderBufferUsageType usage,
                                                                                             QSSGByteView hostData)
{
    GLuint bufID = 0;

    GL_CALL_FUNCTION(glGenBuffers(1, &bufID));

    if (bufID && hostData.size()) {
        GLenum target = GLConversion::fromBindBufferFlagsToGL(bindFlags);
        if (target != GL_INVALID_ENUM) {
            GL_CALL_FUNCTION(glBindBuffer(target, bufID));
            GL_CALL_FUNCTION(glBufferData(target, hostData.size(), hostData, m_conversion.fromBufferUsageTypeToGL(usage)));
        } else {
            GL_CALL_FUNCTION(glDeleteBuffers(1, &bufID));
            bufID = 0;
            qCCritical(RENDER_GL_ERROR, "%s", GLConversion::processGLError(target));
        }
    }

    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendBufferObject>(quintptr(bufID));
}

void QSSGRenderBackendGLBase::bindBuffer(QSSGRenderBackendBufferObject bo, QSSGRenderBufferType bindFlags)
{
    GLuint bufID = HandleToID_cast(GLuint, quintptr, bo);
    GL_CALL_FUNCTION(glBindBuffer(m_conversion.fromBindBufferFlagsToGL(bindFlags), bufID));
}

void QSSGRenderBackendGLBase::releaseBuffer(QSSGRenderBackendBufferObject bo)
{
    GLuint bufID = HandleToID_cast(GLuint, quintptr, bo);
    GL_CALL_FUNCTION(glDeleteBuffers(1, &bufID));
}

void QSSGRenderBackendGLBase::updateBuffer(QSSGRenderBackendBufferObject bo,
                                             QSSGRenderBufferType bindFlags,
                                             QSSGRenderBufferUsageType usage,
                                             QSSGByteView data)
{
    GLuint bufID = HandleToID_cast(GLuint, quintptr, bo);
    GLenum target = GLConversion::fromBindBufferFlagsToGL(bindFlags);
    GL_CALL_FUNCTION(glBindBuffer(target, bufID));
    GL_CALL_FUNCTION(glBufferData(target, data.size(), data, m_conversion.fromBufferUsageTypeToGL(usage)));
}

void QSSGRenderBackendGLBase::updateBufferRange(QSSGRenderBackendBufferObject bo,
                                                  QSSGRenderBufferType bindFlags,
                                                  size_t offset,
                                                  QSSGByteView data)
{
    GLuint bufID = HandleToID_cast(GLuint, quintptr, bo);
    GLenum target = GLConversion::fromBindBufferFlagsToGL(bindFlags);
    GL_CALL_FUNCTION(glBindBuffer(target, bufID));
    GL_CALL_FUNCTION(glBufferSubData(target, offset, data.size(), data));
}

void *QSSGRenderBackendGLBase::mapBuffer(QSSGRenderBackendBufferObject, QSSGRenderBufferType, size_t, size_t, QSSGRenderBufferAccessFlags)
{
    // needs GL 3 context
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return nullptr;
}

bool QSSGRenderBackendGLBase::unmapBuffer(QSSGRenderBackendBufferObject, QSSGRenderBufferType)
{
    // needs GL 3 context
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return true;
}

void QSSGRenderBackendGLBase::setMemoryBarrier(QSSGRenderBufferBarrierFlags)
{
    // needs GL 4 context
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

QSSGRenderBackend::QSSGRenderBackendQueryObject QSSGRenderBackendGLBase::createQuery()
{
    // needs GL 3 context
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return QSSGRenderBackendQueryObject(nullptr);
}

void QSSGRenderBackendGLBase::releaseQuery(QSSGRenderBackendQueryObject)
{
    // needs GL 3 context
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void QSSGRenderBackendGLBase::beginQuery(QSSGRenderBackendQueryObject, QSSGRenderQueryType)
{
    // needs GL 3 context
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void QSSGRenderBackendGLBase::endQuery(QSSGRenderBackendQueryObject, QSSGRenderQueryType)
{
    // needs GL 3 context
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void QSSGRenderBackendGLBase::getQueryResult(QSSGRenderBackendQueryObject, QSSGRenderQueryResultType, quint32 *)
{
    // needs GL 3 context
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void QSSGRenderBackendGLBase::getQueryResult(QSSGRenderBackendQueryObject, QSSGRenderQueryResultType, quint64 *)
{
    // needs GL 3 context
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void QSSGRenderBackendGLBase::setQueryTimer(QSSGRenderBackendQueryObject)
{
    // needs GL 3 context
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

QSSGRenderBackend::QSSGRenderBackendSyncObject QSSGRenderBackendGLBase::createSync(QSSGRenderSyncType, QSSGRenderSyncFlags)
{
    // needs GL 3 context
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return QSSGRenderBackendSyncObject(nullptr);
}

void QSSGRenderBackendGLBase::releaseSync(QSSGRenderBackendSyncObject)
{
    // needs GL 3 context
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void QSSGRenderBackendGLBase::waitSync(QSSGRenderBackendSyncObject, QSSGRenderCommandFlushFlags, quint64)
{
    // needs GL 3 context
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

QSSGRenderBackend::QSSGRenderBackendRenderTargetObject QSSGRenderBackendGLBase::createRenderTarget()
{
    GLuint fboID = 0;

    GL_CALL_FUNCTION(glGenFramebuffers(1, &fboID));

    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendRenderTargetObject>(quintptr(fboID));
}

void QSSGRenderBackendGLBase::releaseRenderTarget(QSSGRenderBackendRenderTargetObject rto)
{
    GLuint fboID = HandleToID_cast(GLuint, quintptr, rto);

    if (fboID) {
        GL_CALL_FUNCTION(glDeleteFramebuffers(1, &fboID));
    }
}

void QSSGRenderBackendGLBase::renderTargetAttach(QSSGRenderBackendRenderTargetObject /* rto */,
                                                   QSSGRenderFrameBufferAttachment attachment,
                                                   QSSGRenderBackendRenderbufferObject rbo)
{
    // rto must be the current render target
    GLuint rbID = HandleToID_cast(GLuint, quintptr, rbo);

    GLenum glAttach = GLConversion::fromFramebufferAttachmentsToGL(attachment);

    GL_CALL_FUNCTION(glFramebufferRenderbuffer(GL_FRAMEBUFFER, glAttach, GL_RENDERBUFFER, rbID));
}

void QSSGRenderBackendGLBase::renderTargetAttach(QSSGRenderBackendRenderTargetObject /* rto */,
                                                   QSSGRenderFrameBufferAttachment attachment,
                                                   QSSGRenderBackendTextureObject to,
                                                   QSSGRenderTextureTargetType target)
{
    // rto must be the current render target
    GLuint texID = HandleToID_cast(GLuint, quintptr, to);

    Q_ASSERT(target == QSSGRenderTextureTargetType::Texture2D || m_backendSupport.caps.bits.bMsTextureSupported);

    GLenum glAttach = GLConversion::fromFramebufferAttachmentsToGL(attachment);
    GLenum glTarget = GLConversion::fromTextureTargetToGL(target);

    GL_CALL_FUNCTION(glFramebufferTexture2D(GL_FRAMEBUFFER, glAttach, glTarget, texID, 0))
}

void QSSGRenderBackendGLBase::renderTargetAttach(QSSGRenderBackendRenderTargetObject,
                                                   QSSGRenderFrameBufferAttachment,
                                                   QSSGRenderBackendTextureObject,
                                                   qint32,
                                                   qint32)
{
    // Needs GL3 or GLES 3
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void QSSGRenderBackendGLBase::setRenderTarget(QSSGRenderBackendRenderTargetObject rto)
{
    GLuint fboID = HandleToID_cast(GLuint, quintptr, rto);

    GL_CALL_FUNCTION(glBindFramebuffer(GL_FRAMEBUFFER, fboID));
}

bool QSSGRenderBackendGLBase::renderTargetIsValid(QSSGRenderBackendRenderTargetObject /* rto */)
{
    // rto must be the current render target
    GLenum completeStatus = GL_CALL_FUNCTION(glCheckFramebufferStatus(GL_FRAMEBUFFER));
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

QSSGRenderBackend::QSSGRenderBackendRenderbufferObject QSSGRenderBackendGLBase::createRenderbuffer(QSSGRenderRenderBufferFormat storageFormat,
                                                                                                         qint32 width,
                                                                                                         qint32 height)
{
    GLuint bufID = 0;

    GL_CALL_FUNCTION(glGenRenderbuffers(1, &bufID));
    GL_CALL_FUNCTION(glBindRenderbuffer(GL_RENDERBUFFER, bufID));
    GL_CALL_FUNCTION(glRenderbufferStorage(GL_RENDERBUFFER,
                                           GLConversion::fromRenderBufferFormatsToRenderBufferGL(storageFormat),
                                           GLsizei(width),
                                           GLsizei(height)));

    // check for error
    GLenum error = m_glFunctions->glGetError();
    if (error != GL_NO_ERROR) {
        qCCritical(RENDER_GL_ERROR, "%s", GLConversion::processGLError(error));
        Q_ASSERT(false);
        GL_CALL_FUNCTION(glDeleteRenderbuffers(1, &bufID));
        bufID = 0;
    }

    GL_CALL_FUNCTION(glBindRenderbuffer(GL_RENDERBUFFER, 0));

    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendRenderbufferObject>(quintptr(bufID));
}

void QSSGRenderBackendGLBase::releaseRenderbuffer(QSSGRenderBackendRenderbufferObject rbo)
{
    GLuint bufID = HandleToID_cast(GLuint, quintptr, rbo);

    if (bufID) {
        GL_CALL_FUNCTION(glDeleteRenderbuffers(1, &bufID));
    }
}

bool QSSGRenderBackendGLBase::resizeRenderbuffer(QSSGRenderBackendRenderbufferObject rbo,
                                                   QSSGRenderRenderBufferFormat storageFormat,
                                                   qint32 width,
                                                   qint32 height)
{
    bool success = true;
    GLuint bufID = HandleToID_cast(GLuint, quintptr, rbo);

    Q_ASSERT(bufID);

    GL_CALL_FUNCTION(glBindRenderbuffer(GL_RENDERBUFFER, bufID));
    GL_CALL_FUNCTION(glRenderbufferStorage(GL_RENDERBUFFER,
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

QSSGRenderBackend::QSSGRenderBackendTextureObject QSSGRenderBackendGLBase::createTexture()
{
    GLuint texID = 0;

    GL_CALL_FUNCTION(glGenTextures(1, &texID));
    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendTextureObject>(quintptr(texID));
}

void QSSGRenderBackendGLBase::bindTexture(QSSGRenderBackendTextureObject to,
                                            QSSGRenderTextureTargetType target,
                                            qint32 unit)
{
    Q_ASSERT(unit >= 0);
    GLuint texID = HandleToID_cast(GLuint, quintptr, to);
    setActiveTexture(GL_TEXTURE0 + GLenum(unit));

    GL_CALL_FUNCTION(glBindTexture(m_conversion.fromTextureTargetToGL(target), texID));
}

void QSSGRenderBackendGLBase::setActiveTexture(qint32 unit)
{
    if (unit != m_activatedTextureUnit) {
        GL_CALL_FUNCTION(glActiveTexture(GLenum(unit)))
        m_activatedTextureUnit = unit;
    }
}

void QSSGRenderBackendGLBase::bindImageTexture(QSSGRenderBackendTextureObject,
                                                 quint32,
                                                 qint32,
                                                 bool,
                                                 qint32,
                                                 QSSGRenderImageAccessType,
                                                 QSSGRenderTextureFormat)
{
    // needs GL 4 context
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void QSSGRenderBackendGLBase::releaseTexture(QSSGRenderBackendTextureObject to)
{
    GLuint texID = HandleToID_cast(GLuint, quintptr, to);
    GL_CALL_FUNCTION(glDeleteTextures(1, &texID));
}

void QSSGRenderBackendGLBase::setTextureData2D(QSSGRenderBackendTextureObject to,
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
    GL_CALL_FUNCTION(glBindTexture(glTarget, texID));
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

    GL_CALL_FUNCTION(glTexImage2D(glTarget, level, glInternalFormat, GLsizei(width), GLsizei(height), border, glformat, gltype, hostData));

    GL_CALL_FUNCTION(glBindTexture(glTarget, 0));
}

// This will look very SetTextureData2D, but the target for glBindTexture will be different from
// the target for
// glTexImage2D.
void QSSGRenderBackendGLBase::setTextureDataCubeFace(QSSGRenderBackendTextureObject to,
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
    GLenum glTexTarget = GLConversion::fromTextureTargetToGL(QSSGRenderTextureTargetType::TextureCube);
    setActiveTexture(GL_TEXTURE0);
    GL_CALL_FUNCTION(glBindTexture(glTexTarget, texID));
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

    // for es2 internal format must be same as format
    if (getRenderContextType() == QSSGRenderContextType::GLES2)
        glInternalFormat = glformat;

    GL_CALL_FUNCTION(glTexImage2D(glTarget, level, glInternalFormat, GLsizei(width), GLsizei(height), border, glformat, gltype, hostData));

    GL_CALL_FUNCTION(glBindTexture(glTexTarget, 0));
}

void QSSGRenderBackendGLBase::createTextureStorage2D(QSSGRenderBackendTextureObject,
                                                       QSSGRenderTextureTargetType,
                                                       qint32,
                                                       QSSGRenderTextureFormat,
                                                       qint32,
                                                       qint32)
{
    // you need GL 4.2 or GLES 3.1
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void QSSGRenderBackendGLBase::setTextureSubData2D(QSSGRenderBackendTextureObject to,
                                                    QSSGRenderTextureTargetType target,
                                                    qint32 level,
                                                    qint32 xOffset,
                                                    qint32 yOffset,
                                                    qint32 width,
                                                    qint32 height,
                                                    QSSGRenderTextureFormat format,
                                                    QSSGByteView hostData)
{
    GLuint texID = HandleToID_cast(GLuint, quintptr, to);
    GLenum glTarget = GLConversion::fromTextureTargetToGL(target);
    setActiveTexture(GL_TEXTURE0);
    GL_CALL_FUNCTION(glBindTexture(glTarget, texID));

    QSSGRenderTextureSwizzleMode swizzleMode = QSSGRenderTextureSwizzleMode::NoSwizzle;
    format = GLConversion::replaceDeprecatedTextureFormat(getRenderContextType(), format, swizzleMode);

    GLenum glformat = 0, glInternalFormat = 0, gltype = 0;
    GLConversion::fromUncompressedTextureFormatToGL(getRenderContextType(), format, glformat, gltype, glInternalFormat);
    GL_CALL_FUNCTION(glTexSubImage2D(glTarget, level, xOffset, yOffset, GLsizei(width), GLsizei(height), glformat, gltype, hostData));

    GL_CALL_FUNCTION(glBindTexture(glTarget, 0));
}

void QSSGRenderBackendGLBase::setCompressedTextureData2D(QSSGRenderBackendTextureObject to,
                                                           QSSGRenderTextureTargetType target,
                                                           qint32 level,
                                                           QSSGRenderTextureFormat internalFormat,
                                                           qint32 width,
                                                           qint32 height,
                                                           qint32 border,
                                                           QSSGByteView hostData)
{
    GLuint texID = HandleToID_cast(GLuint, quintptr, to);
    GLenum glTarget = GLConversion::fromTextureTargetToGL(target);
    setActiveTexture(GL_TEXTURE0);
    GL_CALL_FUNCTION(glBindTexture(glTarget, texID));

    GLenum glformat = GLConversion::fromCompressedTextureFormatToGL(internalFormat);
    GL_CALL_FUNCTION(glCompressedTexImage2D(glTarget, level, glformat, GLsizei(width), GLsizei(height), border, GLsizei(hostData.size()), hostData));

    GL_CALL_FUNCTION(glBindTexture(glTarget, 0));
}

void QSSGRenderBackendGLBase::setCompressedTextureDataCubeFace(QSSGRenderBackendTextureObject to,
                                                                 QSSGRenderTextureTargetType target,
                                                                 qint32 level,
                                                                 QSSGRenderTextureFormat internalFormat,
                                                                 qint32 width,
                                                                 qint32 height,
                                                                 qint32 border,
                                                                 QSSGByteView hostData)
{
    GLuint texID = HandleToID_cast(GLuint, quintptr, to);
    GLenum glTarget = GLConversion::fromTextureTargetToGL(target);
    GLenum glTexTarget = GLConversion::fromTextureTargetToGL(QSSGRenderTextureTargetType::TextureCube);
    setActiveTexture(GL_TEXTURE0);
    GL_CALL_FUNCTION(glBindTexture(glTexTarget, texID));

    GLenum glformat = GLConversion::fromCompressedTextureFormatToGL(internalFormat);
    GL_CALL_FUNCTION(glCompressedTexImage2D(glTarget, level, glformat, GLsizei(width), GLsizei(height), border, GLsizei(hostData.size()), hostData));

    GL_CALL_FUNCTION(glBindTexture(glTexTarget, 0));
}

void QSSGRenderBackendGLBase::setCompressedTextureSubData2D(QSSGRenderBackendTextureObject to,
                                                              QSSGRenderTextureTargetType target,
                                                              qint32 level,
                                                              qint32 xOffset,
                                                              qint32 yOffset,
                                                              qint32 width,
                                                              qint32 height,
                                                              QSSGRenderTextureFormat format,
                                                              QSSGByteView hostData)
{
    GLuint texID = HandleToID_cast(GLuint, quintptr, to);
    GLenum glTarget = GLConversion::fromTextureTargetToGL(target);
    setActiveTexture(GL_TEXTURE0);
    GL_CALL_FUNCTION(glBindTexture(glTarget, texID));

    GLenum glformat = GLConversion::fromCompressedTextureFormatToGL(format);
    GL_CALL_FUNCTION(
            glCompressedTexSubImage2D(glTarget, level, xOffset, yOffset, GLsizei(width), GLsizei(height), glformat, GLsizei(hostData.size()), hostData));

    GL_CALL_FUNCTION(glBindTexture(glTarget, 0));
}

void QSSGRenderBackendGLBase::setTextureData3D(QSSGRenderBackendTextureObject,
                                                 QSSGRenderTextureTargetType,
                                                 qint32,
                                                 QSSGRenderTextureFormat,
                                                 qint32,
                                                 qint32,
                                                 qint32,
                                                 qint32,
                                                 QSSGRenderTextureFormat,
                                                 QSSGByteView)
{
    // needs GL3 or GLES3
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void QSSGRenderBackendGLBase::generateMipMaps(QSSGRenderBackendTextureObject to,
                                                QSSGRenderTextureTargetType target,
                                                QSSGRenderHint genType)
{
    GLuint texID = HandleToID_cast(GLuint, quintptr, to);
    GLenum glTarget = GLConversion::fromTextureTargetToGL(target);
    setActiveTexture(GL_TEXTURE0);
    GL_CALL_FUNCTION(glBindTexture(glTarget, texID));

    GLenum glValue = GLConversion::fromHintToGL(genType);
    GL_CALL_FUNCTION(glHint(GL_GENERATE_MIPMAP_HINT, glValue));
    GL_CALL_FUNCTION(glGenerateMipmap(glTarget));

    GL_CALL_FUNCTION(glBindTexture(glTarget, 0));
}

QSSGRenderTextureSwizzleMode QSSGRenderBackendGLBase::getTextureSwizzleMode(const QSSGRenderTextureFormat inFormat) const
{
    QSSGRenderTextureSwizzleMode swizzleMode = QSSGRenderTextureSwizzleMode::NoSwizzle;
    GLConversion::replaceDeprecatedTextureFormat(getRenderContextType(), inFormat, swizzleMode);

    return swizzleMode;
}

QSSGRenderBackend::QSSGRenderBackendSamplerObject QSSGRenderBackendGLBase::createSampler(
        QSSGRenderTextureMinifyingOp minFilter,
        QSSGRenderTextureMagnifyingOp magFilter,
        QSSGRenderTextureCoordOp wrapS,
        QSSGRenderTextureCoordOp wrapT,
        QSSGRenderTextureCoordOp wrapR,
        qint32 minLod,
        qint32 maxLod,
        float lodBias,
        QSSGRenderTextureCompareMode compareMode,
        QSSGRenderTextureCompareOp compareFunc,
        float anisotropy,
        float *borderColor)
{
    // Satisfy the compiler
    // We don"t setup the state here for GL
    // but we need to pass on the variables here
    // to satisfy the interface
    Q_UNUSED(minFilter)
    Q_UNUSED(magFilter)
    Q_UNUSED(wrapS)
    Q_UNUSED(wrapT)
    Q_UNUSED(wrapR)
    Q_UNUSED(minLod)
    Q_UNUSED(maxLod)
    Q_UNUSED(lodBias)
    Q_UNUSED(compareMode)
    Q_UNUSED(compareFunc)
    Q_UNUSED(anisotropy)
    Q_UNUSED(borderColor)

    // return a dummy handle
    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendSamplerObject>(0x0001);
}

void QSSGRenderBackendGLBase::updateSampler(QSSGRenderBackendSamplerObject /* so */,
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
    // These are not available in GLES 2 and we don't use them right now
    Q_UNUSED(wrapR)
    Q_UNUSED(lodBias)
    Q_UNUSED(minLod)
    Q_UNUSED(maxLod)
    Q_UNUSED(compareMode)
    Q_UNUSED(compareFunc)
    Q_UNUSED(borderColor)

    GLenum glTarget = GLConversion::fromTextureTargetToGL(target);

    GL_CALL_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_MIN_FILTER, m_conversion.fromTextureMinifyingOpToGL(minFilter)));
    GL_CALL_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_MAG_FILTER, m_conversion.fromTextureMagnifyingOpToGL(magFilter)));
    GL_CALL_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_WRAP_S, m_conversion.fromTextureCoordOpToGL(wrapS)));
    GL_CALL_FUNCTION(glTexParameteri(glTarget, GL_TEXTURE_WRAP_T, m_conversion.fromTextureCoordOpToGL(wrapT)));
    if (m_backendSupport.caps.bits.bAnistropySupported) {
        GL_CALL_FUNCTION(glTexParameterf(glTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy));
    }
}

void QSSGRenderBackendGLBase::updateTextureObject(QSSGRenderBackendTextureObject to,
                                                    QSSGRenderTextureTargetType target,
                                                    qint32 baseLevel,
                                                    qint32 maxLevel)
{
    Q_UNUSED(to)
    Q_UNUSED(target)
    Q_UNUSED(baseLevel)
    Q_UNUSED(maxLevel)
}

void QSSGRenderBackendGLBase::updateTextureSwizzle(QSSGRenderBackendTextureObject to,
                                                     QSSGRenderTextureTargetType target,
                                                     QSSGRenderTextureSwizzleMode swizzleMode)
{
    Q_UNUSED(to)
    Q_UNUSED(target)

    // Nothing to do here still might be called
    Q_ASSERT(swizzleMode == QSSGRenderTextureSwizzleMode::NoSwizzle);

    Q_UNUSED(swizzleMode)
}

void QSSGRenderBackendGLBase::releaseSampler(QSSGRenderBackendSamplerObject so)
{
    GLuint samplerID = HandleToID_cast(GLuint, quintptr, so);
    if (!samplerID)
        return;
    // otherwise nothing to do
}

QSSGRenderBackend::QSSGRenderBackendAttribLayoutObject QSSGRenderBackendGLBase::createAttribLayout(
        QSSGDataView<QSSGRenderVertexBufferEntry> attribs)
{
    quint32 attribLayoutSize = sizeof(QSSGRenderBackendAttributeLayoutGL);
    quint32 entrySize = sizeof(QSSGRenderBackendLayoutEntryGL) * attribs.size();
    quint8 *newMem = static_cast<quint8 *>(::malloc(attribLayoutSize + entrySize));
    QSSGDataRef<QSSGRenderBackendLayoutEntryGL> entryRef = PtrAtOffset<QSSGRenderBackendLayoutEntryGL>(newMem, attribLayoutSize, entrySize);
    quint32 maxInputSlot = 0;

    // copy data
    for (int idx = 0; idx != attribs.size(); ++idx) {
        new (&entryRef[idx]) QSSGRenderBackendLayoutEntryGL();
        entryRef[idx].m_attribName = attribs.mData[idx].m_name;
        entryRef[idx].m_normalize = 0;
        entryRef[idx].m_attribIndex = 0; // will be set later
        entryRef[idx].m_type = GLConversion::fromComponentTypeAndNumCompsToAttribGL(attribs.mData[idx].m_componentType,
                                                                                    attribs.mData[idx].m_numComponents);
        entryRef[idx].m_numComponents = attribs.mData[idx].m_numComponents;
        entryRef[idx].m_inputSlot = attribs.mData[idx].m_inputSlot;
        entryRef[idx].m_offset = attribs.mData[idx].m_firstItemOffset;

        if (maxInputSlot < entryRef[idx].m_inputSlot)
            maxInputSlot = entryRef[idx].m_inputSlot;
    }

    QSSGRenderBackendAttributeLayoutGL *retval = new (newMem) QSSGRenderBackendAttributeLayoutGL(entryRef, maxInputSlot);

    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendAttribLayoutObject>(retval);
}

void QSSGRenderBackendGLBase::releaseAttribLayout(QSSGRenderBackendAttribLayoutObject ao)
{
    QSSGRenderBackendAttributeLayoutGL *attribLayout = reinterpret_cast<QSSGRenderBackendAttributeLayoutGL *>(ao);
    if (attribLayout) { // Created with malloc, so release with free!
        attribLayout->~QSSGRenderBackendAttributeLayoutGL();
        ::free(attribLayout);
        attribLayout = nullptr;
    }
};

QSSGRenderBackend::QSSGRenderBackendInputAssemblerObject QSSGRenderBackendGLBase::createInputAssembler(
        QSSGRenderBackendAttribLayoutObject attribLayout,
        QSSGDataView<QSSGRenderBackendBufferObject> buffers,
        const QSSGRenderBackendBufferObject indexBuffer,
        QSSGDataView<quint32> strides,
        QSSGDataView<quint32> offsets,
        quint32 patchVertexCount)
{
    QSSGRenderBackendAttributeLayoutGL *attribLayoutGL = reinterpret_cast<QSSGRenderBackendAttributeLayoutGL *>(attribLayout);

    QSSGRenderBackendInputAssemblerGL *retval = new QSSGRenderBackendInputAssemblerGL(attribLayoutGL,
                                                                                          buffers,
                                                                                          indexBuffer,
                                                                                          strides,
                                                                                          offsets,
                                                                                          patchVertexCount);

    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendInputAssemblerObject>(retval);
}

void QSSGRenderBackendGLBase::releaseInputAssembler(QSSGRenderBackendInputAssemblerObject iao)
{
    QSSGRenderBackendInputAssemblerGL *inputAssembler = reinterpret_cast<QSSGRenderBackendInputAssemblerGL *>(iao);
    delete inputAssembler;
}

void QSSGRenderBackendGLBase::resetStates()
{
    m_usedAttribCount = m_maxAttribCount;
    m_activatedTextureUnit = ACTIVATED_TEXTURE_UNIT_UNKNOWN;
}

bool QSSGRenderBackendGLBase::compileSource(GLuint shaderID, QSSGByteView source, QByteArray &errorMessage, bool binary)
{
    GLint shaderSourceSize = static_cast<GLint>(source.size());
    const char *shaderSourceData = reinterpret_cast<const char *>(source.begin());
    GLint shaderStatus = GL_TRUE;

    if (!binary) {

        GL_CALL_FUNCTION(glShaderSource(shaderID, 1, &shaderSourceData, &shaderSourceSize));
        GL_CALL_FUNCTION(glCompileShader(shaderID));

        GLint logLen;
        GL_CALL_FUNCTION(glGetShaderiv(shaderID, GL_COMPILE_STATUS, &shaderStatus));
        GL_CALL_FUNCTION(glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLen));

        // Check if some log exists. We also write warnings here
        // Should at least contain more than the null termination
        if (logLen > 2) {
            errorMessage.resize(logLen + 1);

            GLint lenWithoutNull;
            GL_CALL_FUNCTION(glGetShaderInfoLog(shaderID, logLen, &lenWithoutNull, errorMessage.data()));
        }
    } else {
        GL_CALL_FUNCTION(glShaderBinary(1, &shaderID, GL_NVIDIA_PLATFORM_BINARY_NV, shaderSourceData, shaderSourceSize));
        GLenum binaryError = m_glFunctions->glGetError();
        if (binaryError != GL_NO_ERROR) {
            errorMessage = QByteArrayLiteral("Binary shader compilation failed");
            shaderStatus = GL_FALSE;
            qCCritical(RENDER_GL_ERROR, "%s", GLConversion::processGLError(binaryError));
        }
    }

    return (shaderStatus == GL_TRUE);
}

QSSGRenderBackend::QSSGRenderBackendVertexShaderObject QSSGRenderBackendGLBase::createVertexShader(QSSGByteView source,
                                                                                                         QByteArray &errorMessage,
                                                                                                         bool binary)
{
    GLuint shaderID = GL_CALL_FUNCTION(glCreateShader(GL_VERTEX_SHADER));

    if (shaderID && !compileSource(shaderID, source, errorMessage, binary)) {
        GL_CALL_FUNCTION(glDeleteShader(shaderID));
        shaderID = 0;
    }

    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendVertexShaderObject>(quintptr(shaderID));
}

QSSGRenderBackend::QSSGRenderBackendFragmentShaderObject QSSGRenderBackendGLBase::createFragmentShader(QSSGByteView source,
                                                                                                             QByteArray &errorMessage,
                                                                                                             bool binary)
{
    GLuint shaderID = GL_CALL_FUNCTION(glCreateShader(GL_FRAGMENT_SHADER));

    if (shaderID && !compileSource(shaderID, source, errorMessage, binary)) {
        GL_CALL_FUNCTION(glDeleteShader(shaderID));
        shaderID = 0;
    }

    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendFragmentShaderObject>(quintptr(shaderID));
}

QSSGRenderBackend::QSSGRenderBackendTessControlShaderObject QSSGRenderBackendGLBase::createTessControlShader(
        QSSGByteView source,
        QByteArray &errorMessage,
        bool binary)
{
    // needs GL 4 or GLES EXT_tessellation_shader support
    Q_UNUSED(source)
    Q_UNUSED(errorMessage)
    Q_UNUSED(binary)

    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return nullptr;
}

QSSGRenderBackend::QSSGRenderBackendTessEvaluationShaderObject QSSGRenderBackendGLBase::createTessEvaluationShader(
        QSSGByteView source,
        QByteArray &errorMessage,
        bool binary)
{
    // needs GL 4 or GLES EXT_tessellation_shader support
    Q_UNUSED(source)
    Q_UNUSED(errorMessage)
    Q_UNUSED(binary)

    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return nullptr;
}

QSSGRenderBackend::QSSGRenderBackendGeometryShaderObject QSSGRenderBackendGLBase::createGeometryShader(QSSGByteView source,
                                                                                                             QByteArray &errorMessage,
                                                                                                             bool binary)
{
    // needs GL 4 or GLES EXT_geometry_shader support
    Q_UNUSED(source)
    Q_UNUSED(errorMessage)
    Q_UNUSED(binary)

    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return nullptr;
}

QSSGRenderBackend::QSSGRenderBackendComputeShaderObject QSSGRenderBackendGLBase::createComputeShader(QSSGByteView source,
                                                                                                           QByteArray &errorMessage,
                                                                                                           bool binary)
{
    // needs GL 4.3 or GLES3.1 support
    Q_UNUSED(source)
    Q_UNUSED(errorMessage)
    Q_UNUSED(binary)

    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return nullptr;
}

void QSSGRenderBackendGLBase::releaseVertexShader(QSSGRenderBackendVertexShaderObject vso)
{
    GLuint shaderID = HandleToID_cast(GLuint, quintptr, vso);

    GL_CALL_FUNCTION(glDeleteShader(shaderID));
}

void QSSGRenderBackendGLBase::releaseFragmentShader(QSSGRenderBackendFragmentShaderObject fso)
{
    GLuint shaderID = HandleToID_cast(GLuint, quintptr, fso);

    GL_CALL_FUNCTION(glDeleteShader(shaderID));
}

void QSSGRenderBackendGLBase::releaseTessControlShader(QSSGRenderBackendTessControlShaderObject tcso)
{
    GLuint shaderID = HandleToID_cast(GLuint, quintptr, tcso);

    GL_CALL_FUNCTION(glDeleteShader(shaderID));
}

void QSSGRenderBackendGLBase::releaseTessEvaluationShader(QSSGRenderBackendTessEvaluationShaderObject teso)
{
    GLuint shaderID = HandleToID_cast(GLuint, quintptr, teso);

    GL_CALL_FUNCTION(glDeleteShader(shaderID));
}

void QSSGRenderBackendGLBase::releaseGeometryShader(QSSGRenderBackendGeometryShaderObject gso)
{
    GLuint shaderID = HandleToID_cast(GLuint, quintptr, gso);

    GL_CALL_FUNCTION(glDeleteShader(shaderID));
}

void QSSGRenderBackendGLBase::releaseComputeShader(QSSGRenderBackendComputeShaderObject cso)
{
    GLuint shaderID = HandleToID_cast(GLuint, quintptr, cso);

    GL_CALL_FUNCTION(glDeleteShader(shaderID));
}

void QSSGRenderBackendGLBase::attachShader(QSSGRenderBackendShaderProgramObject po, QSSGRenderBackendVertexShaderObject vso)
{
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint shaderID = HandleToID_cast(GLuint, quintptr, vso);

    GL_CALL_FUNCTION(glAttachShader(static_cast<GLuint>(pProgram->m_programID), shaderID));
}

void QSSGRenderBackendGLBase::attachShader(QSSGRenderBackendShaderProgramObject po, QSSGRenderBackendFragmentShaderObject fso)
{
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint shaderID = HandleToID_cast(GLuint, quintptr, fso);

    GL_CALL_FUNCTION(glAttachShader(static_cast<GLuint>(pProgram->m_programID), shaderID));
}

void QSSGRenderBackendGLBase::attachShader(QSSGRenderBackendShaderProgramObject po, QSSGRenderBackendTessControlShaderObject tcso)
{
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint shaderID = HandleToID_cast(GLuint, quintptr, tcso);

    GL_CALL_FUNCTION(glAttachShader(static_cast<GLuint>(pProgram->m_programID), shaderID));
}

void QSSGRenderBackendGLBase::attachShader(QSSGRenderBackendShaderProgramObject po, QSSGRenderBackendTessEvaluationShaderObject teso)
{
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint shaderID = HandleToID_cast(GLuint, quintptr, teso);

    GL_CALL_FUNCTION(glAttachShader(static_cast<GLuint>(pProgram->m_programID), shaderID));
}

void QSSGRenderBackendGLBase::attachShader(QSSGRenderBackendShaderProgramObject po, QSSGRenderBackendGeometryShaderObject gso)
{
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint shaderID = HandleToID_cast(GLuint, quintptr, gso);

    GL_CALL_FUNCTION(glAttachShader(static_cast<GLuint>(pProgram->m_programID), shaderID));
}

void QSSGRenderBackendGLBase::attachShader(QSSGRenderBackendShaderProgramObject po, QSSGRenderBackendComputeShaderObject cso)
{
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint shaderID = HandleToID_cast(GLuint, quintptr, cso);

    GL_CALL_FUNCTION(glAttachShader(static_cast<GLuint>(pProgram->m_programID), shaderID));
}

void QSSGRenderBackendGLBase::detachShader(QSSGRenderBackendShaderProgramObject po, QSSGRenderBackendVertexShaderObject vso)
{
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint shaderID = HandleToID_cast(GLuint, quintptr, vso);

    GL_CALL_FUNCTION(glDetachShader(static_cast<GLuint>(pProgram->m_programID), shaderID));
}

void QSSGRenderBackendGLBase::detachShader(QSSGRenderBackendShaderProgramObject po, QSSGRenderBackendFragmentShaderObject fso)
{
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint shaderID = HandleToID_cast(GLuint, quintptr, fso);

    GL_CALL_FUNCTION(glDetachShader(static_cast<GLuint>(pProgram->m_programID), shaderID));
}

void QSSGRenderBackendGLBase::detachShader(QSSGRenderBackendShaderProgramObject po, QSSGRenderBackendTessControlShaderObject tcso)
{
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint shaderID = HandleToID_cast(GLuint, quintptr, tcso);

    GL_CALL_FUNCTION(glDetachShader(static_cast<GLuint>(pProgram->m_programID), shaderID));
}

void QSSGRenderBackendGLBase::detachShader(QSSGRenderBackendShaderProgramObject po, QSSGRenderBackendTessEvaluationShaderObject teso)
{
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint shaderID = HandleToID_cast(GLuint, quintptr, teso);

    GL_CALL_FUNCTION(glDetachShader(static_cast<GLuint>(pProgram->m_programID), shaderID));
}

void QSSGRenderBackendGLBase::detachShader(QSSGRenderBackendShaderProgramObject po, QSSGRenderBackendGeometryShaderObject gso)
{
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint shaderID = HandleToID_cast(GLuint, quintptr, gso);

    GL_CALL_FUNCTION(glDetachShader(static_cast<GLuint>(pProgram->m_programID), shaderID));
}

void QSSGRenderBackendGLBase::detachShader(QSSGRenderBackendShaderProgramObject po, QSSGRenderBackendComputeShaderObject cso)
{
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint shaderID = HandleToID_cast(GLuint, quintptr, cso);

    GL_CALL_FUNCTION(glDetachShader(static_cast<GLuint>(pProgram->m_programID), shaderID));
}

QSSGRenderBackend::QSSGRenderBackendShaderProgramObject QSSGRenderBackendGLBase::createShaderProgram(bool isSeparable)
{
    QSSGRenderBackendShaderProgramGL *theProgram = nullptr;
    GLuint programID = GL_CALL_FUNCTION(glCreateProgram());

    if (programID) {
        theProgram = new QSSGRenderBackendShaderProgramGL(programID);

        if (!theProgram) {
            GL_CALL_FUNCTION(glDeleteProgram(programID));
        } else if (isSeparable && m_backendSupport.caps.bits.bProgramPipelineSupported) {
            GL_CALL_EXTRA_FUNCTION(glProgramParameteri(programID, GL_PROGRAM_SEPARABLE, GL_TRUE));
        }
    }

    return reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendShaderProgramObject>(theProgram);
}

void QSSGRenderBackendGLBase::releaseShaderProgram(QSSGRenderBackendShaderProgramObject po)
{
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);

    GL_CALL_FUNCTION(glDeleteProgram(programID));

    delete pProgram;
}

void QSSGRenderBackendGLBase::getAttributes(QSSGRenderBackendShaderProgramGL *pProgram)
{
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);
    // release old stuff
    if (pProgram->m_shaderInput) {
        delete pProgram->m_shaderInput;
        pProgram->m_shaderInput = nullptr;
    }

    GLint numAttribs;
    GL_CALL_FUNCTION(glGetProgramiv(programID, GL_ACTIVE_ATTRIBUTES, &numAttribs));

    if (numAttribs) {
        QSSGRenderBackendShaderInputEntryGL *tempShaderInputEntry = static_cast<QSSGRenderBackendShaderInputEntryGL *>(
                ::malloc(sizeof(QSSGRenderBackendShaderInputEntryGL) * size_t(m_maxAttribCount)));

        GLint maxLength;
        GL_CALL_FUNCTION(glGetProgramiv(programID, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLength));
        qint8 *nameBuf = static_cast<qint8 *>(::malloc(size_t(maxLength)));

        // fill in data
        qint32 count = 0;
        for (int idx = 0; idx != numAttribs; ++idx) {
            GLint size = 0;
            GLenum glType;
            QSSGRenderComponentType compType = QSSGRenderComponentType::Unknown;
            quint32 numComps = 0;

            GL_CALL_FUNCTION(glGetActiveAttrib(programID, idx, maxLength, nullptr, &size, &glType, (char *)nameBuf));
            // Skip anything named with gl_
            if (memcmp(nameBuf, "gl_", 3) == 0)
                continue;

            GLConversion::fromAttribGLToComponentTypeAndNumComps(glType, compType, numComps);

            new (&tempShaderInputEntry[count]) QSSGRenderBackendShaderInputEntryGL();
            tempShaderInputEntry[count].m_attribName = QByteArray(reinterpret_cast<const char *>(nameBuf));
            tempShaderInputEntry[count].m_attribLocation = GL_CALL_FUNCTION(glGetAttribLocation(programID, (char *)nameBuf));
            tempShaderInputEntry[count].m_type = glType;
            tempShaderInputEntry[count].m_numComponents = numComps;

            ++count;
        }

        // Now allocate space for the actuall entries
        quint32 shaderInputSize = sizeof(QSSGRenderBackendShaderInputGL);
        quint32 entrySize = sizeof(QSSGRenderBackendShaderInputEntryGL) * count;
        quint8 *newMem = static_cast<quint8 *>(::malloc(shaderInputSize + entrySize));
        QSSGDataRef<QSSGRenderBackendShaderInputEntryGL> entryRef = PtrAtOffset<QSSGRenderBackendShaderInputEntryGL>(newMem, shaderInputSize, entrySize);
        // fill data
        for (int idx = 0; idx != count; ++idx) {
            new (&entryRef[idx]) QSSGRenderBackendShaderInputEntryGL();
            entryRef[idx].m_attribName = tempShaderInputEntry[idx].m_attribName;
            entryRef[idx].m_attribLocation = tempShaderInputEntry[idx].m_attribLocation;
            entryRef[idx].m_type = tempShaderInputEntry[idx].m_type;
            entryRef[idx].m_numComponents = tempShaderInputEntry[idx].m_numComponents;
            // Re-set the entry to release the QByteArray, we can do the plane free later
            tempShaderInputEntry[idx] = QSSGRenderBackendShaderInputEntryGL();
        }

        // placement new
        QSSGRenderBackendShaderInputGL *shaderInput = new (newMem) QSSGRenderBackendShaderInputGL(entryRef);
        // set the pointer
        pProgram->m_shaderInput = shaderInput;

        ::free(nameBuf);
        ::free(tempShaderInputEntry);
    }
}

bool QSSGRenderBackendGLBase::linkProgram(QSSGRenderBackendShaderProgramObject po, QByteArray &errorMessage)
{
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);

    GL_CALL_FUNCTION(glLinkProgram(programID));

    GLint linkStatus, logLen;
    GL_CALL_FUNCTION(glGetProgramiv(programID, GL_LINK_STATUS, &linkStatus));
    GL_CALL_FUNCTION(glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &logLen));

    // if successfully linked get the attribute information
    if (linkStatus)
        getAttributes(pProgram);

    // Check if some log exists. We also write warnings here
    // Should at least contain more than the null termination
    if (logLen > 2) {
        errorMessage.resize(logLen + 1);

        GLint lenWithoutNull;
        GL_CALL_FUNCTION(glGetProgramInfoLog(programID, logLen, &lenWithoutNull, errorMessage.data()));
    }

    return (linkStatus == GL_TRUE);
}

bool QSSGRenderBackendGLBase::linkProgram(QSSGRenderBackendShaderProgramObject po,
                                          QByteArray &errorMessage,
                                          quint32 format, const QByteArray &binary)
{
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);

    GL_CALL_EXTRA_FUNCTION(glProgramBinary(programID, GLenum(format), binary.constData(), binary.size()));

    GLint linkStatus, logLen;
    GL_CALL_FUNCTION(glGetProgramiv(programID, GL_LINK_STATUS, &linkStatus));
    GL_CALL_FUNCTION(glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &logLen));

    // if successfully linked get the attribute information
    if (linkStatus)
        getAttributes(pProgram);

    // Check if some log exists. We also write warnings here
    // Should at least contain more than the null termination
    if (logLen > 2) {
        errorMessage.resize(logLen + 1);

        GLint lenWithoutNull;
        GL_CALL_FUNCTION(glGetProgramInfoLog(programID, logLen, &lenWithoutNull, errorMessage.data()));
    }

    return (linkStatus == GL_TRUE);
}

void QSSGRenderBackendGLBase::getProgramBinary(QSSGRenderBackendShaderProgramObject po, quint32 &format, QByteArray &binary)
{
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);
    GLint binLen, linkStatus;

    GL_CALL_FUNCTION(glGetProgramiv(programID, GL_LINK_STATUS, &linkStatus));
    Q_ASSERT(linkStatus == GL_TRUE);

    GL_CALL_FUNCTION(glGetProgramiv(programID, GL_PROGRAM_BINARY_LENGTH, &binLen));

    binary.resize(binLen);
    GLenum fmt;
    GL_CALL_EXTRA_FUNCTION(glGetProgramBinary(programID, binLen, nullptr, &fmt,
                                              binary.data()));
    format = fmt;
}

void QSSGRenderBackendGLBase::setActiveProgram(QSSGRenderBackendShaderProgramObject po)
{
    GLuint programID = 0;

    if (po) {
        QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
        programID = static_cast<GLuint>(pProgram->m_programID);
    }

    GL_CALL_FUNCTION(glUseProgram(programID));
}

QSSGRenderBackend::QSSGRenderBackendProgramPipeline QSSGRenderBackendGLBase::createProgramPipeline()
{
    // needs GL 4 context
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
    return QSSGRenderBackend::QSSGRenderBackendProgramPipeline(nullptr);
}

void QSSGRenderBackendGLBase::releaseProgramPipeline(QSSGRenderBackendProgramPipeline)
{
    // needs GL 4 context
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void QSSGRenderBackendGLBase::setActiveProgramPipeline(QSSGRenderBackendProgramPipeline)
{
    // needs GL 4 context
    // TODO: should be fixed?
    //        Q_ASSERT(false);
}

void QSSGRenderBackendGLBase::setProgramStages(QSSGRenderBackendProgramPipeline, QSSGRenderShaderTypeFlags, QSSGRenderBackendShaderProgramObject)
{
    // needs GL 4 context
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void QSSGRenderBackendGLBase::dispatchCompute(QSSGRenderBackendShaderProgramObject, quint32, quint32, quint32)
{
    // needs GL 4 context
    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

qint32 QSSGRenderBackendGLBase::getConstantCount(QSSGRenderBackendShaderProgramObject po)
{
    Q_ASSERT(po);
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);

    GLint numUniforms;
    GL_CALL_FUNCTION(glGetProgramiv(programID, GL_ACTIVE_UNIFORMS, &numUniforms));

    return numUniforms;
}

qint32 QSSGRenderBackendGLBase::getConstantBufferCount(QSSGRenderBackendShaderProgramObject po)
{
    // needs GL3 and above
    Q_UNUSED(po)

    return 0;
}

qint32 QSSGRenderBackendGLBase::getConstantInfoByID(QSSGRenderBackendShaderProgramObject po,
                                                      quint32 id,
                                                      quint32 bufSize,
                                                      qint32 *numElem,
                                                      QSSGRenderShaderDataType *type,
                                                      qint32 *binding,
                                                      char *nameBuf)
{
    Q_ASSERT(po);
    QSSGRenderBackendShaderProgramGL *pProgram = reinterpret_cast<QSSGRenderBackendShaderProgramGL *>(po);
    GLuint programID = static_cast<GLuint>(pProgram->m_programID);

    GLenum glType;
    GL_CALL_FUNCTION(glGetActiveUniform(programID, id, GLsizei(bufSize), nullptr, numElem, &glType, nameBuf));
    *type = GLConversion::fromShaderGLToPropertyDataTypes(glType);

    qint32 uniformLoc = GL_CALL_FUNCTION(glGetUniformLocation(programID, nameBuf));

    // get unit binding point
    *binding = -1;
    if (uniformLoc != -1 && (glType == GL_IMAGE_2D || glType == GL_UNSIGNED_INT_IMAGE_2D || glType == GL_UNSIGNED_INT_ATOMIC_COUNTER)) {
        GL_CALL_FUNCTION(glGetUniformiv(programID, uniformLoc, binding));
    }

    return uniformLoc;
}

qint32 QSSGRenderBackendGLBase::getConstantBufferInfoByID(QSSGRenderBackendShaderProgramObject po,
                                                            quint32 id,
                                                            quint32 nameBufSize,
                                                            qint32 *paramCount,
                                                            qint32 *bufferSize,
                                                            qint32 *length,
                                                            char *nameBuf)
{
    // needs GL3 and above
    Q_UNUSED(po)
    Q_UNUSED(id)
    Q_UNUSED(nameBufSize)
    Q_UNUSED(paramCount)
    Q_UNUSED(bufferSize)
    Q_UNUSED(length)
    Q_UNUSED(nameBuf)

    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return -1;
}

void QSSGRenderBackendGLBase::getConstantBufferParamIndices(QSSGRenderBackendShaderProgramObject po, quint32 id, qint32 *indices)
{
    // needs GL3 and above
    Q_UNUSED(po)
    Q_UNUSED(id)
    Q_UNUSED(indices)

    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void QSSGRenderBackendGLBase::getConstantBufferParamInfoByIndices(QSSGRenderBackendShaderProgramObject po,
                                                                    quint32 count,
                                                                    quint32 *indices,
                                                                    QSSGRenderShaderDataType *type,
                                                                    qint32 *size,
                                                                    qint32 *offset)
{
    // needs GL3 and above
    Q_UNUSED(po)
    Q_UNUSED(count)
    Q_UNUSED(indices)
    Q_UNUSED(type)
    Q_UNUSED(size)
    Q_UNUSED(offset)

    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void QSSGRenderBackendGLBase::programSetConstantBlock(QSSGRenderBackendShaderProgramObject po, quint32 blockIndex, quint32 binding)
{
    // needs GL3 and above
    Q_UNUSED(po)
    Q_UNUSED(blockIndex)
    Q_UNUSED(binding)

    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

void QSSGRenderBackendGLBase::programSetConstantBuffer(quint32 index, QSSGRenderBackendBufferObject bo)
{
    // needs GL3 and above
    Q_UNUSED(index)
    Q_UNUSED(bo)

    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;
}

qint32 QSSGRenderBackendGLBase::getStorageBufferCount(QSSGRenderBackendShaderProgramObject po)
{
    // needs GL4 and above
    Q_UNUSED(po)

    return 0;
}

qint32 QSSGRenderBackendGLBase::getStorageBufferInfoByID(QSSGRenderBackendShaderProgramObject po,
                                                           quint32 id,
                                                           quint32 nameBufSize,
                                                           qint32 *paramCount,
                                                           qint32 *bufferSize,
                                                           qint32 *length,
                                                           char *nameBuf)
{
    // needs GL4 and above
    Q_UNUSED(po)
    Q_UNUSED(id)
    Q_UNUSED(nameBufSize)
    Q_UNUSED(paramCount)
    Q_UNUSED(bufferSize)
    Q_UNUSED(length)
    Q_UNUSED(nameBuf)

    qCCritical(RENDER_INVALID_OPERATION) << QObject::tr("Unsupported method: ") << __FUNCTION__;

    return -1;
}

void QSSGRenderBackendGLBase::programSetStorageBuffer(quint32 index, QSSGRenderBackendBufferObject bo)
{
    // needs GL4 and above
    Q_UNUSED(index)
    Q_UNUSED(bo)
}

void QSSGRenderBackendGLBase::setConstantValue(QSSGRenderBackendShaderProgramObject,
                                                 quint32 id,
                                                 QSSGRenderShaderDataType type,
                                                 qint32 count,
                                                 const void *value,
                                                 bool transpose)
{
    GLenum glType = GLConversion::fromPropertyDataTypesToShaderGL(type);

    switch (glType) {
    case GL_FLOAT:
        GL_CALL_FUNCTION(glUniform1fv(GLint(id), count, reinterpret_cast<const GLfloat *>(value)));
        break;
    case GL_FLOAT_VEC2:
        GL_CALL_FUNCTION(glUniform2fv(GLint(id), count, reinterpret_cast<const GLfloat *>(value)));
        break;
    case GL_FLOAT_VEC3:
        GL_CALL_FUNCTION(glUniform3fv(GLint(id), count, reinterpret_cast<const GLfloat *>(value)));
        break;
    case GL_FLOAT_VEC4:
        GL_CALL_FUNCTION(glUniform4fv(GLint(id), count, reinterpret_cast<const GLfloat *>(value)));
        break;
    case GL_INT:
        GL_CALL_FUNCTION(glUniform1iv(GLint(id), count, reinterpret_cast<const GLint *>(value)));
        break;
    case GL_BOOL: {
        const GLint boolValue = value ? *reinterpret_cast<const bool *>(value) : false;
        GL_CALL_FUNCTION(glUniform1iv(GLint(id), count, &boolValue));
    } break;
    case GL_INT_VEC2:
    case GL_BOOL_VEC2:
        GL_CALL_FUNCTION(glUniform2iv(GLint(id), count, reinterpret_cast<const GLint *>(value)));
        break;
    case GL_INT_VEC3:
    case GL_BOOL_VEC3:
        GL_CALL_FUNCTION(glUniform3iv(GLint(id), count, reinterpret_cast<const GLint *>(value)));
        break;
    case GL_INT_VEC4:
    case GL_BOOL_VEC4:
        GL_CALL_FUNCTION(glUniform4iv(GLint(id), count, reinterpret_cast<const GLint *>(value)));
        break;
    case GL_FLOAT_MAT3:
        GL_CALL_FUNCTION(glUniformMatrix3fv(GLint(id), count, transpose, reinterpret_cast<const GLfloat *>(value)));
        break;
    case GL_FLOAT_MAT4:
        GL_CALL_FUNCTION(glUniformMatrix4fv(GLint(id), count, transpose, reinterpret_cast<const GLfloat *>(value)));
        break;
    case GL_IMAGE_2D:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_CUBE: {
        if (count > 1) {
            const GLint *sampler = reinterpret_cast<const GLint *>(value);
            GL_CALL_FUNCTION(glUniform1iv(GLint(id), count, sampler));
        } else {
            const GLint sampler = *reinterpret_cast<const GLint *>(value);
            GL_CALL_FUNCTION(glUniform1i(GLint(id), sampler));
        }
    } break;
    default:
        qCCritical(RENDER_INTERNAL_ERROR, "Unknown shader type format %d", int(type));
        Q_ASSERT(false);
        break;
    }
}

void QSSGRenderBackendGLBase::draw(QSSGRenderDrawMode drawMode, quint32 start, quint32 count)
{
    GL_CALL_FUNCTION(glDrawArrays(m_conversion.fromDrawModeToGL(drawMode, m_backendSupport.caps.bits.bTessellationSupported), GLint(start), GLsizei(count)));
}

void QSSGRenderBackendGLBase::drawIndexed(QSSGRenderDrawMode drawMode,
                                            quint32 count,
                                            QSSGRenderComponentType type,
                                            const void *indices)
{
    GL_CALL_FUNCTION(glDrawElements(m_conversion.fromDrawModeToGL(drawMode, m_backendSupport.caps.bits.bTessellationSupported),
                                    GLint(count),
                                    m_conversion.fromIndexBufferComponentsTypesToGL(type),
                                    indices));
}

void QSSGRenderBackendGLBase::readPixel(QSSGRenderBackendRenderTargetObject /* rto */,
                                          qint32 x,
                                          qint32 y,
                                          qint32 width,
                                          qint32 height,
                                          QSSGRenderReadPixelFormat inFormat,
                                          QSSGByteRef pixels)
{
    GLuint glFormat;
    GLuint glType;
    if (GLConversion::fromReadPixelsToGlFormatAndType(inFormat, &glFormat, &glType)) {
        GL_CALL_FUNCTION(glReadPixels(x, y, width, height, glFormat, glType, pixels));
    }
}

///< private calls
const char *QSSGRenderBackendGLBase::getShadingLanguageVersionString()
{
    const GLubyte *retval = GL_CALL_FUNCTION(glGetString(GL_SHADING_LANGUAGE_VERSION));
    if (retval == nullptr)
        return "";

    return reinterpret_cast<const char *>(retval);
}

const char *QSSGRenderBackendGLBase::getVersionString()
{
    const GLubyte *retval = GL_CALL_FUNCTION(glGetString(GL_VERSION));
    if (retval == nullptr)
        return "";

    return reinterpret_cast<const char *>(retval);
}

const char *QSSGRenderBackendGLBase::getVendorString()
{
    const GLubyte *retval = GL_CALL_FUNCTION(glGetString(GL_VENDOR));
    if (retval == nullptr)
        return "";

    return reinterpret_cast<const char *>(retval);
}

const char *QSSGRenderBackendGLBase::getRendererString()
{
    const GLubyte *retval = GL_CALL_FUNCTION(glGetString(GL_RENDERER));
    if (retval == nullptr)
        return "";

    return reinterpret_cast<const char *>(retval);
}

const char *QSSGRenderBackendGLBase::getExtensionString()
{
    const GLubyte *retval = GL_CALL_FUNCTION(glGetString(GL_EXTENSIONS));
    if (retval == nullptr)
        return "";

    return reinterpret_cast<const char *>(retval);
}

/**
 * @brief This function inspects the various strings to setup
 *		  HW capabilities of the device.
 *		  We can do a lot of smart things here based on GL version
 *		  renderer string and vendor.
 *
 * @return No return
 */
void QSSGRenderBackendGLBase::setAndInspectHardwareCaps()
{
    QByteArray apiVersion(getVersionString());
    qCInfo(RENDER_TRACE_INFO, "GL version: %s", apiVersion.constData());

    // we assume all GLES versions running on mobile with shared memory
    // this means framebuffer blits are slow and should be optimized or avoided
    if (!apiVersion.contains("OpenGL ES")) {
        // no ES device
        m_backendSupport.caps.bits.bFastBlitsSupported = true;
    }
}

QT_END_NAMESPACE
