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

#ifndef QDEMON_RENDER_CONTEXT_H
#define QDEMON_RENDER_CONTEXT_H

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

#include <private/qssgdataref_p.h>

#include <QtQuick3DRender/private/qtquick3drenderglobal_p.h>
#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>
#include <QtQuick3DRender/private/qssgglimplobjects_p.h>
#include <QtQuick3DRender/private/qssgrenderbackendgles2_p.h>
#include <QtQuick3DRender/private/qssgrenderbackendgl3_p.h>
#include <QtQuick3DRender/private/qssgrenderbackendgl4_p.h>
#include <QtQuick3DRender/private/qssgrenderbackendnull_p.h>
#include <QtQuick3DRender/private/qssgrendervertexbuffer_p.h>
#include <QtQuick3DRender/private/qssgrenderindexbuffer_p.h>
#include <QtQuick3DRender/private/qssgrenderconstantbuffer_p.h>
#include <QtQuick3DRender/private/qssgrenderframebuffer_p.h>
#include <QtQuick3DRender/private/qssgrenderrenderbuffer_p.h>
#include <QtQuick3DRender/private/qssgrenderdepthstencilstate_p.h>
#include <QtQuick3DRender/private/qssgrenderrasterizerstate_p.h>
#include <QtQuick3DRender/private/qssgrenderinputassembler_p.h>
#include <QtQuick3DRender/private/qssgrenderattriblayout_p.h>
#include <QtQuick3DRender/private/qssgrenderimagetexture_p.h>
#include <QtQuick3DRender/private/qssgrenderocclusionquery_p.h>
#include <QtQuick3DRender/private/qssgrendertimerquery_p.h>
#include <QtQuick3DRender/private/qssgrendersync_p.h>
#include <QtQuick3DRender/private/qssgrendertexture2darray_p.h>
#include <QtQuick3DRender/private/qssgrendertexturecube_p.h>
#include <QtQuick3DRender/private/qssgrenderstoragebuffer_p.h>
#include <QtQuick3DRender/private/qssgrenderatomiccounterbuffer_p.h>
#include <QtQuick3DRender/private/qssgrenderdrawindirectbuffer_p.h>
#include <QtQuick3DRender/private/qssgrenderpathrender_p.h>
#include <QtQuick3DRender/private/qssgrenderpathspecification_p.h>

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QSharedPointer>

#include <QtGui/QSurfaceFormat>

QT_BEGIN_NAMESPACE

// When SW fallback is defined we can support (some) object/layer advanced blend modes. If defined,
// the HW implementation is still preperred if available through extensions. SW fallback can't be
// used in custom shaders.
#define ADVANCED_BLEND_SW_FALLBACK

enum class QSSGRenderShaderProgramBinaryType
{
    Unknown = 0,
    NVBinary = 1,
};

// context dirty flags
enum class QSSGRenderContextDirtyValues
{
    InputAssembler = 1 << 0,
};

Q_DECLARE_FLAGS(QSSGRenderContextDirtyFlags, QSSGRenderContextDirtyValues)
Q_DECLARE_OPERATORS_FOR_FLAGS(QSSGRenderContextDirtyFlags)

typedef QHash<QByteArray, QSSGRef<QSSGRenderConstantBuffer>> TContextConstantBufferMap;
typedef QHash<QByteArray, QSSGRef<QSSGRenderStorageBuffer>> TContextStorageBufferMap;
typedef QHash<QByteArray, QSSGRef<QSSGRenderAtomicCounterBuffer>> TContextAtomicCounterBufferMap;
typedef QHash<QSSGRenderBackend::QSSGRenderBackendRasterizerStateObject, QSSGRenderRasterizerState *> TContextRasterizerStateMap;
typedef QHash<QString, QSSGRenderPathFontSpecification *> TContextPathFontSpecificationMap;

class QSSGRenderProgramPipeline;

// Now for scoped property access.
template<typename TDataType>
struct QSSGRenderContextScopedProperty : public QSSGRenderGenericScopedProperty<QSSGRenderContext, TDataType>
{
    // the static assert does not compile with MSVC2015
    //Q_STATIC_ASSERT_X(!std::is_reference<TDataType>::value, "Changing the same data!!!");
    typedef typename QSSGRenderGenericScopedProperty<QSSGRenderContext, TDataType>::TGetter TGetter;
    typedef typename QSSGRenderGenericScopedProperty<QSSGRenderContext, TDataType>::TSetter TSetter;
    QSSGRenderContextScopedProperty(QSSGRenderContext &ctx, TGetter getter, TSetter setter)
        : QSSGRenderGenericScopedProperty<QSSGRenderContext, TDataType>(ctx, getter, setter)
    {
    }
    QSSGRenderContextScopedProperty(QSSGRenderContext &ctx, TGetter getter, TSetter setter, const TDataType &inNewValue)
        : QSSGRenderGenericScopedProperty<QSSGRenderContext, TDataType>(ctx, getter, setter, inNewValue)
    {
    }
};

// TODO: Get rid of this, or at least make it more explicit, i.e, don't assume any patterns.
#define ITERATE_HARDWARE_CONTEXT_PROPERTIES                                                                            \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(RenderTarget, frameBuffer)                                                        \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(ActiveShader, activeShader)                                                       \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(ActiveProgramPipeline, activeProgramPipeline)                                     \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(InputAssembler, inputAssembler)                                                   \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(BlendFunction, blendFunction)                                                     \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(CullingEnabled, cullingEnabled)                                                   \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(DepthFunction, depthFunction)                                                     \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(BlendingEnabled, blendingEnabled)                                                 \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(DepthWriteEnabled, depthWriteEnabled)                                             \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(DepthTestEnabled, depthTestEnabled)                                               \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(StencilTestEnabled, stencilTestEnabled)                                           \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(ScissorTestEnabled, scissorTestEnabled)                                           \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(ScissorRect, scissorRect)                                                         \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(Viewport, viewport)                                                               \
    HANDLE_CONTEXT_HARDWARE_PROPERTY(ClearColor, clearColor)

class Q_QUICK3DRENDER_EXPORT QSSGRenderContext
{
    Q_DISABLE_COPY(QSSGRenderContext)
public:
    QAtomicInt ref;
    // these variables represent the current hardware state of the render context.
    QSSGGLHardPropertyContext m_hardwarePropertyContext;

private:
    const QSSGRef<QSSGRenderBackend> m_backend; ///< pointer to our render backend
    QSSGRenderContextDirtyFlags m_dirtyFlags; ///< context dirty flags

    QSSGRenderBackend::QSSGRenderBackendRenderTargetObject m_defaultOffscreenRenderTarget; ///< this is a special target set from outside if we
    /// never render to a window directly (GL only)
    qint32 m_dephBits; ///< this is the depth bits count of the default window render target
    qint32 m_stencilBits; ///< this is the stencil bits count of the default window render target

protected:
    TContextConstantBufferMap m_constantToImpMap;
    TContextStorageBufferMap m_storageToImpMap;
    TContextAtomicCounterBufferMap m_atomicCounterToImpMap;

    qint32 m_maxTextureUnits;
    qint32 m_nextTextureUnit;
    qint32 m_maxConstantBufferUnits;
    qint32 m_nextConstantBufferUnit;

    QVector<QSSGGLHardPropertyContext> m_propertyStack;

    void doSetClearColor(QVector4D inClearColor)
    {
        m_hardwarePropertyContext.m_clearColor = inClearColor;
        m_backend->setClearColor(&inClearColor);
    }

    void doSetBlendFunction(QSSGRenderBlendFunctionArgument inFunctions)
    {
        qint32_4 values;
        m_hardwarePropertyContext.m_blendFunction = inFunctions;

        m_backend->setBlendFunc(inFunctions);
    }

    void doSetBlendEquation(QSSGRenderBlendEquationArgument inEquations)
    {
        qint32_4 values;
        m_hardwarePropertyContext.m_blendEquation = inEquations;

        m_backend->setBlendEquation(inEquations);
    }

    void doSetCullingEnabled(bool inEnabled)
    {
        m_hardwarePropertyContext.m_cullingEnabled = inEnabled;
        m_backend->setRenderState(inEnabled, QSSGRenderState::CullFace);
    }

    void doSetDepthFunction(QSSGRenderBoolOp inFunction)
    {
        m_hardwarePropertyContext.m_depthFunction = inFunction;
        m_backend->setDepthFunc(inFunction);
    }

    void doSetBlendingEnabled(bool inEnabled)
    {
        m_hardwarePropertyContext.m_blendingEnabled = inEnabled;
        m_backend->setRenderState(inEnabled, QSSGRenderState::Blend);
    }

    void doSetColorWritesEnabled(bool inEnabled)
    {
        m_hardwarePropertyContext.m_colorWritesEnabled = inEnabled;
        m_backend->setColorWrites(inEnabled, inEnabled, inEnabled, inEnabled);
    }

    void doSetMultisampleEnabled(bool inEnabled)
    {
        m_hardwarePropertyContext.m_multisampleEnabled = inEnabled;
        m_backend->setMultisample(inEnabled);
    }

    void doSetDepthWriteEnabled(bool inEnabled)
    {
        m_hardwarePropertyContext.m_depthWriteEnabled = inEnabled;
        m_backend->setDepthWrite(inEnabled);
    }

    void doSetDepthTestEnabled(bool inEnabled)
    {
        m_hardwarePropertyContext.m_depthTestEnabled = inEnabled;
        m_backend->setRenderState(inEnabled, QSSGRenderState::DepthTest);
    }

    void doSetStencilTestEnabled(bool inEnabled)
    {
        m_hardwarePropertyContext.m_stencilTestEnabled = inEnabled;
        m_backend->setRenderState(inEnabled, QSSGRenderState::StencilTest);
    }

    void doSetScissorTestEnabled(bool inEnabled)
    {
        m_hardwarePropertyContext.m_scissorTestEnabled = inEnabled;
        m_backend->setRenderState(inEnabled, QSSGRenderState::ScissorTest);
    }

    void doSetScissorRect(QRect inRect)
    {
        m_hardwarePropertyContext.m_scissorRect = inRect;
        m_backend->setScissorRect(inRect);
    }

    void doSetViewport(QRect inViewport)
    {
        m_hardwarePropertyContext.m_viewport = inViewport;
        m_backend->setViewportRect(inViewport);
    }

    // Circular dependencies between shader constants and shader programs preclude
    // implementation in header
    void doSetActiveShader(const QSSGRef<QSSGRenderShaderProgram> &inShader);
    void doSetActiveProgramPipeline(const QSSGRef<QSSGRenderProgramPipeline> &inProgramPipeline);

    void doSetInputAssembler(const QSSGRef<QSSGRenderInputAssembler> &inAssembler)
    {
        m_hardwarePropertyContext.m_inputAssembler = inAssembler;
        m_dirtyFlags |= QSSGRenderContextDirtyValues::InputAssembler;
    }

    void doSetRenderTarget(const QSSGRef<QSSGRenderFrameBuffer> &inBuffer)
    {
        if (inBuffer)
            m_backend->setRenderTarget(inBuffer->handle());
        else
            m_backend->setRenderTarget(m_defaultOffscreenRenderTarget);

        m_hardwarePropertyContext.m_frameBuffer = inBuffer;
    }

    void doSetReadTarget(const QSSGRef<QSSGRenderFrameBuffer> &inBuffer)
    {
        if (inBuffer)
            m_backend->setReadTarget(inBuffer->handle());
        else
            m_backend->setReadTarget(QSSGRenderBackend::QSSGRenderBackendRenderTargetObject(nullptr));
    }

    bool bindShaderToInputAssembler(const QSSGRef<QSSGRenderInputAssembler> &inputAssembler,
                                    const QSSGRef<QSSGRenderShaderProgram> &shader);
    bool applyPreDrawProperties();
    void onPostDraw();

public:
    QSSGRenderContext(const QSSGRef<QSSGRenderBackend> &inBackend);
    ~QSSGRenderContext();

    const QSSGRef<QSSGRenderBackend> &backend() { return m_backend; }

    void maxTextureSize(qint32 &oWidth, qint32 &oHeight);

    const char *shadingLanguageVersion() { return m_backend->getShadingLanguageVersion(); }

    QSSGRenderContextType renderContextType() const { return m_backend->getRenderContextType(); }

    qint32 depthBits() const
    {
        // only query this if a framebuffer is bound
        if (m_hardwarePropertyContext.m_frameBuffer)
            return m_backend->getDepthBits();
        else
            return m_dephBits;
    }

    qint32 stencilBits() const
    {
        // only query this if a framebuffer is bound
        if (m_hardwarePropertyContext.m_frameBuffer)
            return m_backend->getStencilBits();
        else
            return m_stencilBits;
    }

    bool renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps inCap) const
    {
        return m_backend->getRenderBackendCap(inCap);
    }

    bool supportsMultisampleTextures() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::MsTexture);
    }

    bool supportsConstantBuffer() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::ConstantBuffer);
    }

    bool supportsDXTImages() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::DxtImages);
    }

    bool supportsDepthStencil() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::DepthStencilTexture);
    }

    bool supportsFpRenderTarget() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::FpRenderTarget);
    }

    bool supportsTessellation() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::Tessellation);
    }

    bool supportsGeometryStage() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::Geometry);
    }

    bool supportsCompute() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::Compute);
    }

    bool supportsSampleQuery() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::SampleQuery);
    }

    bool supportsTimerQuery() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::TimerQuery);
    }

    bool supportsCommandSync() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::CommandSync);
    }
    bool supportsTextureArray() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::TextureArray);
    }
    bool supportsStorageBuffer() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::StorageBuffer);
    }
    bool supportsAtomicCounterBuffer() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::AtomicCounterBuffer);
    }
    bool supportsShaderImageLoadStore() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::ShaderImageLoadStore);
    }
    bool supportsProgramPipeline() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::ProgramPipeline);
    }
    bool supportsPathRendering() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::PathRendering);
    }
    // Are blend modes really supported in HW?
    bool supportsAdvancedBlendHW() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::AdvancedBlend);
    }
    bool supportsAdvancedBlendHwKHR() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::AdvancedBlendKHR);
    }
    bool supportsBlendCoherency() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::BlendCoherency);
    }
    bool supportsStandardDerivatives() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::StandardDerivatives);
    }
    bool supportsTextureLod() const
    {
        return renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::TextureLod);
    }

    void setDefaultRenderTarget(quint64 targetID)
    {
        m_defaultOffscreenRenderTarget = reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendRenderTargetObject>(targetID);
    }

    void setDefaultDepthBufferBitCount(qint32 depthBits) { m_dephBits = depthBits; }

    void setDepthStencilState(const QSSGRef<QSSGRenderDepthStencilState> &inDepthStencilState);

    void setRasterizerState(const QSSGRef<QSSGRenderRasterizerState> &inRasterizerState);

    void registerConstantBuffer(QSSGRenderConstantBuffer *buffer);
    QSSGRef<QSSGRenderConstantBuffer> getConstantBuffer(const QByteArray &bufferName) const;
    void bufferDestroyed(QSSGRenderConstantBuffer *buffer);

    qint32 nextConstantBufferUnit();

    void registerStorageBuffer(QSSGRenderStorageBuffer *buffer);
    QSSGRef<QSSGRenderStorageBuffer> getStorageBuffer(const QByteArray &bufferName);
    void bufferDestroyed(QSSGRenderStorageBuffer *buffer);

    void registerAtomicCounterBuffer(QSSGRenderAtomicCounterBuffer *buffer);
    QSSGRef<QSSGRenderAtomicCounterBuffer> getAtomicCounterBuffer(const QByteArray &bufferName);
    QSSGRef<QSSGRenderAtomicCounterBuffer> getAtomicCounterBufferByParam(const QByteArray &paramName);
    void bufferDestroyed(QSSGRenderAtomicCounterBuffer *buffer);

    void setMemoryBarrier(QSSGRenderBufferBarrierFlags barriers);

    qint32 nextTextureUnit();

    QSSGRef<QSSGRenderAttribLayout> createAttributeLayout(QSSGDataView<QSSGRenderVertexBufferEntry> attribs);
    QSSGRef<QSSGRenderInputAssembler> createInputAssembler(const QSSGRef<QSSGRenderAttribLayout> &attribLayout,
                                                               QSSGDataView<QSSGRef<QSSGRenderVertexBuffer>> buffers,
                                                               const QSSGRef<QSSGRenderIndexBuffer> &indexBuffer,
                                                               QSSGDataView<quint32> strides,
                                                               QSSGDataView<quint32> offsets,
                                                               QSSGRenderDrawMode primType = QSSGRenderDrawMode::Triangles,
                                                               quint32 patchVertexCount = 1);
    void setInputAssembler(const QSSGRef<QSSGRenderInputAssembler> &inputAssembler);

    QSSGRenderVertFragCompilationResult compileSource(
            const char *shaderName,
            QSSGByteView vertShader,
            QSSGByteView fragShader,
            QSSGByteView tessControlShaderSource = QSSGByteView(),
            QSSGByteView tessEvaluationShaderSource = QSSGByteView(),
            QSSGByteView geometryShaderSource = QSSGByteView(),
            bool separateProgram = false,
            QSSGRenderShaderProgramBinaryType type = QSSGRenderShaderProgramBinaryType::Unknown,
            bool binaryProgram = false);

    QSSGRenderVertFragCompilationResult compileBinary(const char *shaderName,
            QSSGRenderShaderProgramBinaryType type,
            QSSGByteView vertShader,
            QSSGByteView fragShader,
            QSSGByteView tessControlShaderSource = QSSGByteView(),
            QSSGByteView tessEvaluationShaderSource = QSSGByteView(),
            QSSGByteView geometryShaderSource = QSSGByteView());

    QSSGRenderVertFragCompilationResult compileComputeSource(const QByteArray &shaderName,
                                                                       QSSGByteView computeShaderSource);

    void shaderDestroyed(QSSGRenderShaderProgram *shader);

    QSSGRef<QSSGRenderProgramPipeline> createProgramPipeline();
    QSSGRef<QSSGRenderPathSpecification> createPathSpecification();
    QSSGRef<QSSGRenderPathRender> createPathRender(size_t range = 1);
    void setPathProjectionMatrix(const QMatrix4x4 inPathProjection);
    void setPathModelViewMatrix(const QMatrix4x4 inPathModelview);
    void setPathStencilDepthOffset(float inSlope, float inBias);
    void setPathCoverDepthFunc(QSSGRenderBoolOp inFunc);

    QSSGRef<QSSGRenderPathFontSpecification> createPathFontSpecification(const QString &fontName);
    void releasePathFontSpecification(QSSGRenderPathFontSpecification *inPathSpec);

    void setClearColor(QVector4D inClearColor);
    QVector4D clearColor() const { return m_hardwarePropertyContext.m_clearColor; }

    void setBlendFunction(QSSGRenderBlendFunctionArgument inFunctions);
    QSSGRenderBlendFunctionArgument blendFunction() const
    {
        return m_hardwarePropertyContext.m_blendFunction;
    }

    void setBlendEquation(QSSGRenderBlendEquationArgument inEquations);
    QSSGRenderBlendEquationArgument blendEquation() const
    {
        return m_hardwarePropertyContext.m_blendEquation;
    }

    void setCullingEnabled(bool inEnabled);
    bool isCullingEnabled() const { return m_hardwarePropertyContext.m_cullingEnabled; }

    void setDepthFunction(QSSGRenderBoolOp inFunction);
    QSSGRenderBoolOp depthFunction() const { return m_hardwarePropertyContext.m_depthFunction; }

    void setBlendingEnabled(bool inEnabled);
    bool isBlendingEnabled() const { return m_hardwarePropertyContext.m_blendingEnabled; }

    void setDepthWriteEnabled(bool inEnabled);
    bool isDepthWriteEnabled() const { return m_hardwarePropertyContext.m_depthWriteEnabled; }
    void setDepthTestEnabled(bool inEnabled);
    bool isDepthTestEnabled() const { return m_hardwarePropertyContext.m_depthTestEnabled; }

    void setStencilTestEnabled(bool inEnabled);
    bool isStencilTestEnabled() const { return m_hardwarePropertyContext.m_stencilTestEnabled; }

    void setScissorTestEnabled(bool inEnabled);
    bool isScissorTestEnabled() const { return m_hardwarePropertyContext.m_scissorTestEnabled; }
    void setScissorRect(QRect inRect);
    QRect scissorRect() const { return m_hardwarePropertyContext.m_scissorRect; }

    void setViewport(QRect inViewport);
    QRect viewport() const { return m_hardwarePropertyContext.m_viewport; }

    void setColorWritesEnabled(bool inEnabled);
    bool isColorWritesEnabled() const { return m_hardwarePropertyContext.m_colorWritesEnabled; }

    void setMultisampleEnabled(bool inEnabled);
    bool isMultisampleEnabled() const { return m_hardwarePropertyContext.m_multisampleEnabled; }

    void setActiveShader(const QSSGRef<QSSGRenderShaderProgram> &inShader);
    QSSGRef<QSSGRenderShaderProgram> activeShader() const;

    void setActiveProgramPipeline(const QSSGRef<QSSGRenderProgramPipeline> &inProgramPipeline);
    QSSGRef<QSSGRenderProgramPipeline> activeProgramPipeline() const;

    void dispatchCompute(const QSSGRef<QSSGRenderShaderProgram> &inShader, quint32 numGroupsX, quint32 numGroupsY, quint32 numGroupsZ);

    void setDrawBuffers(QSSGDataView<qint32> inDrawBufferSet);
    void setReadBuffer(QSSGReadFace inReadFace);

    void readPixels(QRect inRect, QSSGRenderReadPixelFormat inFormat, QSSGByteRef inWriteBuffer);

    void setRenderTarget(QSSGRef<QSSGRenderFrameBuffer> inBuffer);
    void setReadTarget(QSSGRef<QSSGRenderFrameBuffer> inBuffer);
    QSSGRef<QSSGRenderFrameBuffer> renderTarget() const
    {
        return m_hardwarePropertyContext.m_frameBuffer;
    }

    void resetBlendState();

    // Push the entire set of properties.
    void pushPropertySet();

    // Pop the entire set of properties, potentially forcing the values
    // to opengl.
    void popPropertySet(bool inForceSetProperties);

    // clear current bound render target
    void clear(QSSGRenderClearFlags flags);
    // clear passed in rendertarget
    void clear(const QSSGRef<QSSGRenderFrameBuffer> &fb, QSSGRenderClearFlags flags);

    // copy framebuffer content between read target and render target
    void blitFramebuffer(qint32 srcX0,
                         qint32 srcY0,
                         qint32 srcX1,
                         qint32 srcY1,
                         qint32 dstX0,
                         qint32 dstY0,
                         qint32 dstX1,
                         qint32 dstY1,
                         QSSGRenderClearFlags flags,
                         QSSGRenderTextureMagnifyingOp filter);

    void draw(QSSGRenderDrawMode drawMode, quint32 count, quint32 offset);
    void drawIndirect(QSSGRenderDrawMode drawMode, quint32 offset);

    QSurfaceFormat format() const { return m_backend->format(); }
    void resetStates()
    {
        pushPropertySet();
        popPropertySet(true);
    }

    // Used during layer rendering because we can't set the *actual* viewport to what it should
    // be due to hardware problems.
    // Set during begin render.
    static QMatrix4x4 applyVirtualViewportToProjectionMatrix(const QMatrix4x4 &inProjection,
                                                             const QRectF &inViewport,
                                                             const QRectF &inVirtualViewport);

    static QSSGRef<QSSGRenderContext> createGl(const QSurfaceFormat &format);

    static QSSGRef<QSSGRenderContext> createNull();
};

QT_END_NAMESPACE

#endif
