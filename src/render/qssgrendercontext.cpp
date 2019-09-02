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

#include <QtGui/QMatrix4x4>
#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>
#include <QtQuick3DRender/private/qssgrenderprogrampipeline_p.h>

#include <QtQuick3DUtils/private/qssgutils_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderContext::QSSGRenderContext(const QSSGRef<QSSGRenderBackend> &inBackend)
    : m_backend(inBackend)
    , m_defaultOffscreenRenderTarget(nullptr)
    , m_dephBits(16)
    , m_stencilBits(8)
    , m_nextTextureUnit(1)
    , m_nextConstantBufferUnit(1)
{
    m_maxTextureUnits = m_backend->getMaxCombinedTextureUnits();
    m_maxConstantBufferUnits = 16; // need backend query

    // get default blending functions
    m_backend->getBlendFunc(&m_hardwarePropertyContext.m_blendFunction);
    // set default blend euqation
    m_hardwarePropertyContext.m_blendEquation.m_rgbEquation = QSSGRenderBlendEquation::Add;
    m_hardwarePropertyContext.m_blendEquation.m_alphaEquation = QSSGRenderBlendEquation::Add;
    // default state
    m_hardwarePropertyContext.m_cullingEnabled = m_backend->getRenderState(QSSGRenderState::CullFace);
    m_hardwarePropertyContext.m_depthFunction = m_backend->getDepthFunc();
    m_hardwarePropertyContext.m_blendingEnabled = m_backend->getRenderState(QSSGRenderState::Blend);
    m_hardwarePropertyContext.m_depthWriteEnabled = m_backend->getDepthWrite();
    m_hardwarePropertyContext.m_depthTestEnabled = m_backend->getRenderState(QSSGRenderState::DepthTest);
    m_hardwarePropertyContext.m_scissorTestEnabled = m_backend->getRenderState(QSSGRenderState::ScissorTest);
    m_backend->getScissorRect(&m_hardwarePropertyContext.m_scissorRect);
    m_backend->getViewportRect(&m_hardwarePropertyContext.m_viewport);

    m_backend->setClearColor(&m_hardwarePropertyContext.m_clearColor);
}

QSSGRenderContext::~QSSGRenderContext()
{
    Q_ASSERT(m_constantToImpMap.size() == 0);
    m_constantToImpMap.clear();
    Q_ASSERT(m_storageToImpMap.size() == 0);
    m_storageToImpMap.clear();
}

void QSSGRenderContext::maxTextureSize(qint32 &oWidth, qint32 &oHeight)
{
    qint32 theMaxTextureSize = 0;
    m_backend->getRenderBackendValue(QSSGRenderBackend::QSSGRenderBackendQuery::MaxTextureSize, &theMaxTextureSize);

    oWidth = theMaxTextureSize;
    oHeight = theMaxTextureSize;
}

void QSSGRenderContext::setDepthStencilState(const QSSGRef<QSSGRenderDepthStencilState> &inDepthStencilState)
{
    if (inDepthStencilState) {
        m_backend->setDepthStencilState(inDepthStencilState->handle());
        // currently we have a mixture therefore we need to update the context state
        setDepthFunction(inDepthStencilState->depthFunction());
        setDepthWriteEnabled(inDepthStencilState->depthMask());
        setDepthTestEnabled(inDepthStencilState->depthEnabled());
        setStencilTestEnabled(inDepthStencilState->stencilEnabled());
    }
}

void QSSGRenderContext::setRasterizerState(const QSSGRef<QSSGRenderRasterizerState> &inRasterizerState)
{
    if (inRasterizerState)
        m_backend->setRasterizerState(inRasterizerState->handle());
}

void QSSGRenderContext::registerConstantBuffer(QSSGRenderConstantBuffer *buffer)
{
    Q_ASSERT(buffer);
    m_constantToImpMap.insert(buffer->name(), buffer);
}

QSSGRef<QSSGRenderConstantBuffer> QSSGRenderContext::getConstantBuffer(const QByteArray &bufferName) const
{
    const auto entry = m_constantToImpMap.constFind(bufferName);
    if (entry != m_constantToImpMap.cend())
        return entry.value();
    return nullptr;
}

void QSSGRenderContext::bufferDestroyed(QSSGRenderConstantBuffer *buffer)
{
    const auto it = m_constantToImpMap.constFind(buffer->name());
    if (it != m_constantToImpMap.cend()) {
        Q_ASSERT(it.value()->ref == 1);
        m_constantToImpMap.erase(it);
    }
}

qint32 QSSGRenderContext::nextConstantBufferUnit()
{
    qint32 retval = m_nextConstantBufferUnit;
    ++m_nextConstantBufferUnit;
    // Too many texture units for a single draw call.
    if (retval >= m_maxConstantBufferUnits) {
        Q_ASSERT(false);
        retval = retval % m_maxConstantBufferUnits;
    }
    return retval;
}

void QSSGRenderContext::registerStorageBuffer(QSSGRenderStorageBuffer *buffer)
{
    m_storageToImpMap.insert(buffer->name(), buffer);
}

QSSGRef<QSSGRenderStorageBuffer> QSSGRenderContext::getStorageBuffer(const QByteArray &bufferName)
{
    const auto entry = m_storageToImpMap.constFind(bufferName);
    if (entry != m_storageToImpMap.cend())
        return entry.value();
    return nullptr;
}

void QSSGRenderContext::bufferDestroyed(QSSGRenderStorageBuffer *buffer)
{
    const auto it = m_storageToImpMap.constFind(buffer->name());
    if (it != m_storageToImpMap.cend()) {
        Q_ASSERT(it.value()->ref == 1);
        m_storageToImpMap.erase(it);
    }
}

void QSSGRenderContext::registerAtomicCounterBuffer(QSSGRenderAtomicCounterBuffer *buffer)
{
    m_atomicCounterToImpMap.insert(buffer->bufferName(), buffer);
}

QSSGRef<QSSGRenderAtomicCounterBuffer> QSSGRenderContext::getAtomicCounterBuffer(const QByteArray &bufferName)
{
    const auto entry = m_atomicCounterToImpMap.constFind(bufferName);
    if (entry != m_atomicCounterToImpMap.cend())
        return entry.value();
    return nullptr;
}

QSSGRef<QSSGRenderAtomicCounterBuffer> QSSGRenderContext::getAtomicCounterBufferByParam(const QByteArray &paramName)
{
    // iterate through all atomic counter buffers
    auto it = m_atomicCounterToImpMap.cbegin();
    const auto end = m_atomicCounterToImpMap.cend();
    for (; it != end; ++it) {
        if (it.value() && it.value()->containsParam(paramName))
            break;
    }

    return (it != end) ? it.value() : nullptr;
}

void QSSGRenderContext::bufferDestroyed(QSSGRenderAtomicCounterBuffer *buffer)
{
    const auto it = m_atomicCounterToImpMap.constFind(buffer->bufferName());
    if (it != m_atomicCounterToImpMap.cend()) {
        Q_ASSERT(it.value()->ref == 1);
        m_atomicCounterToImpMap.erase(it);
    }
}

void QSSGRenderContext::setMemoryBarrier(QSSGRenderBufferBarrierFlags barriers)
{
    m_backend->setMemoryBarrier(barriers);
}

// IF this texture isn't on a texture unit, put it on one.
// If it is on a texture unit, mark it as the most recently used texture.
qint32 QSSGRenderContext::nextTextureUnit()
{
    qint32 retval = m_nextTextureUnit;
    ++m_nextTextureUnit;
    // Too many texture units for a single draw call.
    if (retval >= m_maxTextureUnits) {
        Q_ASSERT(false);
        retval = retval % m_maxTextureUnits;
    }
    return retval;
}

QSSGRef<QSSGRenderAttribLayout> QSSGRenderContext::createAttributeLayout(QSSGDataView<QSSGRenderVertexBufferEntry> attribs)
{
    return QSSGRef<QSSGRenderAttribLayout>(new QSSGRenderAttribLayout(this, attribs));
}

QSSGRef<QSSGRenderInputAssembler> QSSGRenderContext::createInputAssembler(
        const QSSGRef<QSSGRenderAttribLayout> &attribLayout,
        QSSGDataView<QSSGRef<QSSGRenderVertexBuffer>> buffers,
        const QSSGRef<QSSGRenderIndexBuffer> &indexBuffer,
        QSSGDataView<quint32> strides,
        QSSGDataView<quint32> offsets,
        QSSGRenderDrawMode primType,
        quint32 patchVertexCount)
{
    return QSSGRef<QSSGRenderInputAssembler>(
            new QSSGRenderInputAssembler(this, attribLayout, buffers, indexBuffer, strides, offsets, primType, patchVertexCount));
}

void QSSGRenderContext::setInputAssembler(const QSSGRef<QSSGRenderInputAssembler> &inputAssembler)
{
    if (m_hardwarePropertyContext.m_inputAssembler != inputAssembler)
        doSetInputAssembler(inputAssembler);
}

QSSGRenderVertFragCompilationResult QSSGRenderContext::compileSource(const char *shaderName,
                                                                             QSSGByteView vertShader,
                                                                             QSSGByteView fragShader,
                                                                             QSSGByteView tessControlShaderSource,
                                                                             QSSGByteView tessEvaluationShaderSource,
                                                                             QSSGByteView geometryShaderSource,
                                                                             bool separateProgram,
                                                                             QSSGRenderShaderProgramBinaryType type,
                                                                             bool binaryProgram)
{
    QSSGRenderVertFragCompilationResult result = QSSGRenderShaderProgram::create(this,
                                                                                     shaderName,
                                                                                     vertShader,
                                                                                     fragShader,
                                                                                     tessControlShaderSource,
                                                                                     tessEvaluationShaderSource,
                                                                                     geometryShaderSource,
                                                                                     separateProgram,
                                                                                     type,
                                                                                     binaryProgram);

    return result;
}

QSSGRenderVertFragCompilationResult QSSGRenderContext::compileBinary(const char *shaderName,
                                                                             QSSGRenderShaderProgramBinaryType type,
                                                                             QSSGByteView vertShader,
                                                                             QSSGByteView fragShader,
                                                                             QSSGByteView tessControlShaderSource,
                                                                             QSSGByteView tessEvaluationShaderSource,
                                                                             QSSGByteView geometryShaderSource)
{
#ifndef _MACOSX
    QSSGRenderVertFragCompilationResult result = QSSGRenderShaderProgram::create(this,
                                                                                     shaderName,
                                                                                     vertShader,
                                                                                     fragShader,
                                                                                     tessControlShaderSource,
                                                                                     tessEvaluationShaderSource,
                                                                                     geometryShaderSource,
                                                                                     false,
                                                                                     type,
                                                                                     true);

    return result;
#else
    Q_ASSERT(false);
    return QSSGRenderVertFragCompilationResult();
#endif
}

QSSGRenderVertFragCompilationResult QSSGRenderContext::compileComputeSource(const QByteArray &shaderName,
                                                                                QSSGByteView computeShaderSource)
{
    QSSGRenderVertFragCompilationResult result = QSSGRenderShaderProgram::createCompute(this, shaderName, computeShaderSource);

    return result;
}


void QSSGRenderContext::shaderDestroyed(QSSGRenderShaderProgram *shader)
{
    if (m_hardwarePropertyContext.m_activeShader.data() == shader)
        setActiveShader(nullptr);
}

QSSGRef<QSSGRenderProgramPipeline> QSSGRenderContext::createProgramPipeline()
{
    return QSSGRef<QSSGRenderProgramPipeline>(new QSSGRenderProgramPipeline(this));
}

QSSGRef<QSSGRenderPathSpecification> QSSGRenderContext::createPathSpecification()
{
    return QSSGRenderPathSpecification::createPathSpecification(this);
}

QSSGRef<QSSGRenderPathRender> QSSGRenderContext::createPathRender(size_t range)
{
    return QSSGRenderPathRender::create(this, range);
}

void QSSGRenderContext::setPathProjectionMatrix(const QMatrix4x4 inPathProjection)
{
    m_backend->setPathProjectionMatrix(inPathProjection);
}

void QSSGRenderContext::setPathModelViewMatrix(const QMatrix4x4 inPathModelview)
{
    m_backend->setPathModelViewMatrix(inPathModelview);
}

void QSSGRenderContext::setPathStencilDepthOffset(float inSlope, float inBias)
{
    m_backend->setPathStencilDepthOffset(inSlope, inBias);
}
void QSSGRenderContext::setPathCoverDepthFunc(QSSGRenderBoolOp inFunc)
{
    m_backend->setPathCoverDepthFunc(inFunc);
}

void QSSGRenderContext::setClearColor(QVector4D inClearColor)
{
    if (m_hardwarePropertyContext.m_clearColor != inClearColor)
        doSetClearColor(inClearColor);
}

void QSSGRenderContext::setBlendFunction(QSSGRenderBlendFunctionArgument inFunctions)
{
    if (memcmp(&inFunctions, &m_hardwarePropertyContext.m_blendFunction, sizeof(QSSGRenderBlendFunctionArgument))) {
        doSetBlendFunction(inFunctions);
    }
}

void QSSGRenderContext::setBlendEquation(QSSGRenderBlendEquationArgument inEquations)
{
    if (memcmp(&inEquations, &m_hardwarePropertyContext.m_blendEquation, sizeof(QSSGRenderBlendEquationArgument))) {
        doSetBlendEquation(inEquations);
    }
}

void QSSGRenderContext::setCullingEnabled(bool inEnabled)
{
    if (inEnabled != m_hardwarePropertyContext.m_cullingEnabled) {
        doSetCullingEnabled(inEnabled);
    }
}

void QSSGRenderContext::setDepthFunction(QSSGRenderBoolOp inFunction)
{
    if (inFunction != m_hardwarePropertyContext.m_depthFunction) {
        doSetDepthFunction(inFunction);
    }
}

void QSSGRenderContext::setBlendingEnabled(bool inEnabled)
{
    if (inEnabled != m_hardwarePropertyContext.m_blendingEnabled) {
        doSetBlendingEnabled(inEnabled);
    }
}

void QSSGRenderContext::setColorWritesEnabled(bool inEnabled)
{
    if (inEnabled != m_hardwarePropertyContext.m_colorWritesEnabled) {
        doSetColorWritesEnabled(inEnabled);
    }
}


void QSSGRenderContext::setDepthWriteEnabled(bool inEnabled)
{
    if (inEnabled != m_hardwarePropertyContext.m_depthWriteEnabled)
        doSetDepthWriteEnabled(inEnabled);
}

void QSSGRenderContext::setDepthTestEnabled(bool inEnabled)
{
    if (inEnabled != m_hardwarePropertyContext.m_depthTestEnabled) {
        doSetDepthTestEnabled(inEnabled);
    }
}

void QSSGRenderContext::setMultisampleEnabled(bool inEnabled)
{
    if (inEnabled != m_hardwarePropertyContext.m_multisampleEnabled) {
        doSetMultisampleEnabled(inEnabled);
    }
}

void QSSGRenderContext::setStencilTestEnabled(bool inEnabled)
{
    if (inEnabled != m_hardwarePropertyContext.m_stencilTestEnabled) {
        doSetStencilTestEnabled(inEnabled);
    }
}

void QSSGRenderContext::setScissorTestEnabled(bool inEnabled)
{
    if (inEnabled != m_hardwarePropertyContext.m_scissorTestEnabled) {
        doSetScissorTestEnabled(inEnabled);
    }
}

void QSSGRenderContext::setScissorRect(QRect inRect)
{
    if (memcmp(&inRect, &m_hardwarePropertyContext.m_scissorRect, sizeof(QRect))) {
        doSetScissorRect(inRect);
    }
}

void QSSGRenderContext::setViewport(QRect inViewport)
{
    if (memcmp(&inViewport, &m_hardwarePropertyContext.m_viewport, sizeof(QRect))) {
        doSetViewport(inViewport);
    }
}

void QSSGRenderContext::setActiveShader(const QSSGRef<QSSGRenderShaderProgram> &inShader)
{
    if (inShader != m_hardwarePropertyContext.m_activeShader)
        doSetActiveShader(inShader);
}

QSSGRef<QSSGRenderShaderProgram> QSSGRenderContext::activeShader() const
{
    return m_hardwarePropertyContext.m_activeShader;
}

void QSSGRenderContext::setActiveProgramPipeline(const QSSGRef<QSSGRenderProgramPipeline> &inProgramPipeline)
{
    if (inProgramPipeline != m_hardwarePropertyContext.m_activeProgramPipeline)
        doSetActiveProgramPipeline(inProgramPipeline);
}

QSSGRef<QSSGRenderProgramPipeline> QSSGRenderContext::activeProgramPipeline() const
{
    return m_hardwarePropertyContext.m_activeProgramPipeline;
}

void QSSGRenderContext::dispatchCompute(const QSSGRef<QSSGRenderShaderProgram> &inShader, quint32 numGroupsX, quint32 numGroupsY, quint32 numGroupsZ)
{
    Q_ASSERT(inShader);

    if (inShader != m_hardwarePropertyContext.m_activeShader)
        doSetActiveShader(inShader);

    m_backend->dispatchCompute(inShader->handle(), numGroupsX, numGroupsY, numGroupsZ);

    onPostDraw();
}

void QSSGRenderContext::setDrawBuffers(QSSGDataView<qint32> inDrawBufferSet)
{
    m_backend->setDrawBuffers((m_hardwarePropertyContext.m_frameBuffer)
                                      ? m_hardwarePropertyContext.m_frameBuffer->handle()
                                      : nullptr,
                              inDrawBufferSet);
}

void QSSGRenderContext::setReadBuffer(QSSGReadFace inReadFace)
{
    // currently nullptr which means the read target must be set with setReadTarget
    m_backend->setReadBuffer(nullptr, inReadFace);
}

void QSSGRenderContext::readPixels(QRect inRect, QSSGRenderReadPixelFormat inFormat, QSSGByteRef inWriteBuffer)
{
    Q_ASSERT(sizeofPixelFormat(inFormat)*inRect.width()*inRect.height() <= inWriteBuffer.size());
    // nullptr means read from current render target
    m_backend->readPixel(nullptr,
                         inRect.x(),
                         inRect.y(),
                         inRect.width(),
                         inRect.height(),
                         inFormat,
                         inWriteBuffer);
}

void QSSGRenderContext::setRenderTarget(QSSGRef<QSSGRenderFrameBuffer> inBuffer)
{
    if (inBuffer != m_hardwarePropertyContext.m_frameBuffer)
        doSetRenderTarget(inBuffer);
}

void QSSGRenderContext::setReadTarget(QSSGRef<QSSGRenderFrameBuffer> inBuffer)
{
    if (inBuffer != m_hardwarePropertyContext.m_frameBuffer)
        doSetReadTarget(inBuffer);
}

void QSSGRenderContext::resetBlendState()
{
    qint32_4 values;

    m_backend->setRenderState(m_hardwarePropertyContext.m_blendingEnabled, QSSGRenderState::Blend);
    const QSSGRenderBlendFunctionArgument &theBlendArg(m_hardwarePropertyContext.m_blendFunction);
    m_backend->setBlendFunc(theBlendArg);
}

void QSSGRenderContext::pushPropertySet()
{
    m_propertyStack.push_back(m_hardwarePropertyContext);
}

// Pop the entire set of properties, potentially forcing the values
// to opengl.
void QSSGRenderContext::popPropertySet(bool inForceSetProperties)
{
    if (!m_propertyStack.empty()) {
        QSSGGLHardPropertyContext &theTopContext(m_propertyStack.back());
        if (inForceSetProperties) {
            doSetRenderTarget(theTopContext.m_frameBuffer);
            doSetActiveShader(theTopContext.m_activeShader);
            doSetActiveProgramPipeline(theTopContext.m_activeProgramPipeline);
            doSetInputAssembler(theTopContext.m_inputAssembler);
            doSetBlendFunction(theTopContext.m_blendFunction);
            doSetCullingEnabled(theTopContext.m_cullingEnabled);
            doSetDepthFunction(theTopContext.m_depthFunction);
            doSetBlendingEnabled(theTopContext.m_blendingEnabled);
            doSetDepthWriteEnabled(theTopContext.m_depthWriteEnabled);
            doSetDepthTestEnabled(theTopContext.m_depthTestEnabled);
            doSetStencilTestEnabled(theTopContext.m_stencilTestEnabled);
            doSetScissorTestEnabled(theTopContext.m_scissorTestEnabled);
            doSetScissorRect(theTopContext.m_scissorRect);
            doSetViewport(theTopContext.m_viewport);
            doSetClearColor(theTopContext.m_clearColor);
        } else {
            setRenderTarget(theTopContext.m_frameBuffer);
            setActiveShader(theTopContext.m_activeShader);
            setActiveProgramPipeline(theTopContext.m_activeProgramPipeline);
            setInputAssembler(theTopContext.m_inputAssembler);
            setBlendFunction(theTopContext.m_blendFunction);
            setCullingEnabled(theTopContext.m_cullingEnabled);
            setDepthFunction(theTopContext.m_depthFunction);
            setBlendingEnabled(theTopContext.m_blendingEnabled);
            setDepthWriteEnabled(theTopContext.m_depthWriteEnabled);
            setDepthTestEnabled(theTopContext.m_depthTestEnabled);
            setStencilTestEnabled(theTopContext.m_stencilTestEnabled);
            setScissorTestEnabled(theTopContext.m_scissorTestEnabled);
            setScissorRect(theTopContext.m_scissorRect);
            setViewport(theTopContext.m_viewport);
            setClearColor(theTopContext.m_clearColor);
        }
        m_propertyStack.pop_back();
    }
}

void QSSGRenderContext::clear(QSSGRenderClearFlags flags)
{
    if ((flags & QSSGRenderClearValues::Depth) && m_hardwarePropertyContext.m_depthWriteEnabled == false) {
        Q_ASSERT(false);
        setDepthWriteEnabled(true);
    }
    m_backend->clear(flags);
}

void QSSGRenderContext::clear(const QSSGRef<QSSGRenderFrameBuffer> &fb, QSSGRenderClearFlags flags)
{
    QSSGRef<QSSGRenderFrameBuffer> previous = m_hardwarePropertyContext.m_frameBuffer;
    if (previous != fb)
        setRenderTarget(fb);

    clear(flags);

    if (previous != fb)
        setRenderTarget(previous);
}

void QSSGRenderContext::blitFramebuffer(qint32 srcX0,
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
    m_backend->blitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, flags, filter);
}

bool QSSGRenderContext::bindShaderToInputAssembler(const QSSGRef<QSSGRenderInputAssembler> &inputAssembler,
                                                     const QSSGRef<QSSGRenderShaderProgram> &shader)
{
    // setup the input assembler object
    return m_backend->setInputAssembler(inputAssembler->handle(), shader->handle());
}

bool QSSGRenderContext::applyPreDrawProperties()
{
    // Get the currently bound vertex and shader
    const QSSGRef<QSSGRenderInputAssembler> &inputAssembler = m_hardwarePropertyContext.m_inputAssembler;
    QSSGRef<QSSGRenderShaderProgram> &shader(m_hardwarePropertyContext.m_activeShader);

    // we could render through a program pipline
    if (shader == nullptr && m_hardwarePropertyContext.m_activeProgramPipeline)
        shader = m_hardwarePropertyContext.m_activeProgramPipeline->vertexStage();

    if (inputAssembler == nullptr || shader == nullptr) {
        qCCritical(INVALID_OPERATION, "Attempting to render no valid shader or input assembler setup");
        Q_ASSERT(false);
        return false;
    }

    return bindShaderToInputAssembler(inputAssembler, shader);
}

void QSSGRenderContext::onPostDraw()
{
    // reset input assembler binding
    m_backend->setInputAssembler(nullptr, nullptr);
    // Texture unit 0 is used for setting up and loading textures.
    // Bugs happen if we load a texture then setup the sampler.
    // Then we load another texture.  Because when loading we use texture unit 0,
    // the render bindings for the first texture are blown away.
    // Again, for this reason, texture unit 0 is reserved for loading textures.
    m_nextTextureUnit = 1;
    m_nextConstantBufferUnit = 0;
}

void QSSGRenderContext::draw(QSSGRenderDrawMode drawMode, quint32 count, quint32 offset)
{
    if (!applyPreDrawProperties())
        return;

    const QSSGRef<QSSGRenderIndexBuffer> &theIndexBuffer = m_hardwarePropertyContext.m_inputAssembler->indexBuffer();
    if (theIndexBuffer == nullptr)
        m_backend->draw(drawMode, offset, count);
    else
        theIndexBuffer->draw(drawMode, count, offset);

    onPostDraw();
}

void QSSGRenderContext::drawIndirect(QSSGRenderDrawMode drawMode, quint32 offset)
{
    if (!applyPreDrawProperties())
        return;

    const QSSGRef<QSSGRenderIndexBuffer> &theIndexBuffer = m_hardwarePropertyContext.m_inputAssembler->indexBuffer();
    if (theIndexBuffer == nullptr)
        m_backend->drawIndirect(drawMode, reinterpret_cast<const void *>(quintptr(offset)));
    else
        theIndexBuffer->drawIndirect(drawMode, offset);

    onPostDraw();
}

QMatrix4x4 QSSGRenderContext::applyVirtualViewportToProjectionMatrix(const QMatrix4x4 &inProjection,
                                                                       const QRectF &inViewport,
                                                                       const QRectF &inVirtualViewport)
{
    if (inVirtualViewport == inViewport)
        return inProjection;
    // Run conversion to floating point once.
    QRectF theVirtualViewport(inVirtualViewport);
    QRectF theViewport(inViewport);
    if (Q_UNLIKELY(qFuzzyIsNull(theVirtualViewport.width()) || qFuzzyIsNull(theVirtualViewport.height()) || qFuzzyIsNull(theViewport.width())
                   || qFuzzyIsNull(theViewport.height()))) {
        Q_ASSERT(false);
        return inProjection;
    }
    QMatrix4x4 theScaleTransMat;
    const qreal theHeightDiff = theViewport.height() - theVirtualViewport.height();
    const qreal theViewportOffY = theVirtualViewport.y() - theViewport.y();
    QVector2D theCameraOffsets = QVector2D(float(theVirtualViewport.width() - theViewport.width() + (theVirtualViewport.x() - theViewport.x())) * 2.0f,
                                           float(theHeightDiff + (theViewportOffY - theHeightDiff)) * 2.0f);
    QVector2D theCameraScale = QVector2D(float(theVirtualViewport.width() / theViewport.width()),
                                         float(theVirtualViewport.height() / theViewport.height()));

    QVector3D theTranslation(theCameraOffsets.x() / float(theViewport.width()), theCameraOffsets.y() / float(theViewport.height()), 0.0f);
    QVector4D column3 = theScaleTransMat.column(3);
    column3.setX(theTranslation.x());
    column3.setY(theTranslation.y());
    theScaleTransMat.setColumn(3, column3);
    QVector4D column0 = theScaleTransMat.column(0);
    column0.setX(theCameraScale.x());
    theScaleTransMat.setColumn(0, column0);
    QVector4D column1 = theScaleTransMat.column(1);
    column1.setY(theCameraScale.y());
    theScaleTransMat.setColumn(1, column1);

    return theScaleTransMat * inProjection;
}

void QSSGRenderContext::doSetActiveShader(const QSSGRef<QSSGRenderShaderProgram> &inShader)
{
    if (!m_backend) {
        m_hardwarePropertyContext.m_activeShader = nullptr;
        return;
    }

    if (m_hardwarePropertyContext.m_activeShader != inShader)
        m_hardwarePropertyContext.m_activeShader = inShader;

    if (inShader)
        m_backend->setActiveProgram(inShader->handle());
    else
        m_backend->setActiveProgram(nullptr);
}

void QSSGRenderContext::doSetActiveProgramPipeline(const QSSGRef<QSSGRenderProgramPipeline> &inProgramPipeline)
{
    if (inProgramPipeline) {
        // invalid any bound shader
        doSetActiveShader(nullptr);
        inProgramPipeline->bind();
    } else {
        m_backend->setActiveProgramPipeline(nullptr);
    }

    m_hardwarePropertyContext.m_activeProgramPipeline = inProgramPipeline;
}
QSSGRef<QSSGRenderContext> QSSGRenderContext::createNull()
{
    return QSSGRef<QSSGRenderContext>(new QSSGRenderContext(QSSGRenderBackendNULL::createBackend()));;
}
QT_END_NAMESPACE
