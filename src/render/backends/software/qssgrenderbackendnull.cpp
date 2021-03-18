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

#include <QtQuick3DRender/private/qssgrenderbackendnull_p.h>

#include <QSurfaceFormat>

QT_BEGIN_NAMESPACE

namespace {
struct QSSGNullBackend : public QSSGRenderBackend
{
    ~QSSGNullBackend() override = default;

    /// backend interface

    QSSGRenderContextType getRenderContextType() const override { return QSSGRenderContextType::NullContext; }
    QByteArray getShadingLanguageVersion() override { return ""; }
    qint32 getMaxCombinedTextureUnits() override { return 32; }
    bool getRenderBackendCap(QSSGRenderBackendCaps) const override { return false; }
    void getRenderBackendValue(QSSGRenderBackendQuery inQuery, qint32 *params) const override
    {
        if (params) {
            switch (inQuery) {
            case QSSGRenderBackendQuery::MaxTextureSize:
                *params = 4096;
                break;
            case QSSGRenderBackendQuery::MaxTextureArrayLayers:
                *params = 0;
                break;
            default:
                Q_ASSERT(false);
                *params = 0;
                break;
            }
        }
    }
    qint32 getDepthBits() const override { return 16; }
    qint32 getStencilBits() const override { return 0; }
    qint32 getMaxSamples() const override { return 4; }
    void setRenderState(bool, const QSSGRenderState) override {}
    bool getRenderState(const QSSGRenderState) override { return false; }
    QSSGRenderBackendDepthStencilStateObject createDepthStencilState(bool,
                                                                       bool,
                                                                       QSSGRenderBoolOp,
                                                                       bool,
                                                                       QSSGRenderStencilFunction &,
                                                                       QSSGRenderStencilFunction &,
                                                                       QSSGRenderStencilOperation &,
                                                                       QSSGRenderStencilOperation &) override
    {
        return QSSGRenderBackendDepthStencilStateObject(1);
    }
    void releaseDepthStencilState(QSSGRenderBackendDepthStencilStateObject) override {}
    QSSGRenderBackendRasterizerStateObject createRasterizerState(float, float) override
    {
        return QSSGRenderBackendRasterizerStateObject(1);
    }
    void releaseRasterizerState(QSSGRenderBackendRasterizerStateObject) override {}
    void setDepthStencilState(QSSGRenderBackendDepthStencilStateObject) override {}
    void setRasterizerState(QSSGRenderBackendRasterizerStateObject) override {}
    QSSGRenderBoolOp getDepthFunc() override { return QSSGRenderBoolOp::Equal; }
    void setDepthFunc(const QSSGRenderBoolOp) override {}
    bool getDepthWrite() override { return false; }

    void setDepthWrite(bool) override {}
    void setColorWrites(bool, bool, bool, bool) override {}
    void setMultisample(bool) override {}
    void getBlendFunc(QSSGRenderBlendFunctionArgument *) override {}
    void setBlendFunc(const QSSGRenderBlendFunctionArgument &) override {}
    void setBlendEquation(const QSSGRenderBlendEquationArgument &) override {}
    void setBlendBarrier(void) override {}
    QSSGCullFaceMode getCullFaceMode() override
    {
        return QSSGCullFaceMode::Unknown;
    }
    void setCullFaceMode(const QSSGCullFaceMode) override {}
    void getScissorRect(QRect *) override {}
    void setScissorRect(const QRect &) override {}
    void getViewportRect(QRect *) override {}
    void setViewportRect(const QRect &) override {}
    void setClearColor(const QVector4D *) override {}
    void clear(QSSGRenderClearFlags) override {}
    QSSGRenderBackendBufferObject createBuffer(QSSGRenderBufferType, QSSGRenderBufferUsageType, QSSGByteView) override
    {
        return QSSGRenderBackendBufferObject(1);
    }
    void bindBuffer(QSSGRenderBackendBufferObject, QSSGRenderBufferType) override {}
    void releaseBuffer(QSSGRenderBackendBufferObject) override {}

    void updateBuffer(QSSGRenderBackendBufferObject, QSSGRenderBufferType, QSSGRenderBufferUsageType, QSSGByteView) override
    {
    }
    void updateBufferRange(QSSGRenderBackendBufferObject, QSSGRenderBufferType, size_t, QSSGByteView) override
    {
    }
    void *mapBuffer(QSSGRenderBackendBufferObject, QSSGRenderBufferType, size_t, size_t, QSSGRenderBufferAccessFlags) override
    {
        return nullptr;
    }
    bool unmapBuffer(QSSGRenderBackendBufferObject, QSSGRenderBufferType) override { return true; }
    void setMemoryBarrier(QSSGRenderBufferBarrierFlags) override {}
    QSSGRenderBackendQueryObject createQuery() override { return QSSGRenderBackendQueryObject(1); }
    void releaseQuery(QSSGRenderBackendQueryObject) override {}
    void beginQuery(QSSGRenderBackendQueryObject, QSSGRenderQueryType) override {}
    void endQuery(QSSGRenderBackendQueryObject, QSSGRenderQueryType) override {}
    void getQueryResult(QSSGRenderBackendQueryObject, QSSGRenderQueryResultType, quint32 *) override {}
    void getQueryResult(QSSGRenderBackendQueryObject, QSSGRenderQueryResultType, quint64 *) override {}
    void setQueryTimer(QSSGRenderBackendQueryObject) override {}
    QSSGRenderBackendSyncObject createSync(QSSGRenderSyncType, QSSGRenderSyncFlags) override
    {
        return QSSGRenderBackendSyncObject(1);
    }
    void releaseSync(QSSGRenderBackendSyncObject) override {}
    void waitSync(QSSGRenderBackendSyncObject, QSSGRenderCommandFlushFlags, quint64) override {}
    QSSGRenderBackendRenderTargetObject createRenderTarget() override
    {
        return QSSGRenderBackendRenderTargetObject(1);
    }
    void releaseRenderTarget(QSSGRenderBackendRenderTargetObject) override {}
    void renderTargetAttach(QSSGRenderBackendRenderTargetObject, QSSGRenderFrameBufferAttachment, QSSGRenderBackendRenderbufferObject) override
    {
    }
    void renderTargetAttach(QSSGRenderBackendRenderTargetObject,
                            QSSGRenderFrameBufferAttachment,
                            QSSGRenderBackendTextureObject,
                            QSSGRenderTextureTargetType) override
    {
    }
    void renderTargetAttach(QSSGRenderBackendRenderTargetObject,
                            QSSGRenderFrameBufferAttachment,
                            QSSGRenderBackendTextureObject,
                            qint32,
                            qint32) override
    {
    }
    void setRenderTarget(QSSGRenderBackendRenderTargetObject) override {}
    bool renderTargetIsValid(QSSGRenderBackendRenderTargetObject) override { return false; }
    void setReadTarget(QSSGRenderBackendRenderTargetObject) override {}
    void setDrawBuffers(QSSGRenderBackendRenderTargetObject, QSSGDataView<qint32>) override {}
    void setReadBuffer(QSSGRenderBackendRenderTargetObject, QSSGReadFace) override {}

    void blitFramebuffer(qint32, qint32, qint32, qint32, qint32, qint32, qint32, qint32,
                         QSSGRenderClearFlags, QSSGRenderTextureMagnifyingOp) override
    {
    }
    void copyFramebufferTexture(qint32, qint32, qint32, qint32, qint32, qint32,
                                QSSGRenderBackendTextureObject, QSSGRenderTextureTargetType) override
    {
    }
    QSSGRenderBackendRenderbufferObject createRenderbuffer(QSSGRenderRenderBufferFormat, qint32, qint32) override
    {
        return QSSGRenderBackendRenderbufferObject(1);
    }
    void releaseRenderbuffer(QSSGRenderBackendRenderbufferObject) override {}

    bool resizeRenderbuffer(QSSGRenderBackendRenderbufferObject, QSSGRenderRenderBufferFormat, qint32, qint32) override
    {
        return false;
    }
    QSSGRenderBackendTextureObject createTexture() override { return QSSGRenderBackendTextureObject(1); }

    void setTextureData2D(QSSGRenderBackendTextureObject,
                          QSSGRenderTextureTargetType,
                          qint32,
                          QSSGRenderTextureFormat,
                          qint32,
                          qint32,
                          qint32,
                          QSSGRenderTextureFormat,
                          QSSGByteView) override
    {
    }
    void setTextureDataCubeFace(QSSGRenderBackendTextureObject,
                                QSSGRenderTextureTargetType,
                                qint32,
                                QSSGRenderTextureFormat,
                                qint32,
                                qint32,
                                qint32,
                                QSSGRenderTextureFormat,
                                QSSGByteView) override
    {
    }
    void createTextureStorage2D(QSSGRenderBackendTextureObject,
                                QSSGRenderTextureTargetType,
                                qint32,
                                QSSGRenderTextureFormat,
                                qint32,
                                qint32) override
    {
    }
    void setTextureSubData2D(QSSGRenderBackendTextureObject,
                             QSSGRenderTextureTargetType,
                             qint32,
                             qint32,
                             qint32,
                             qint32,
                             qint32,
                             QSSGRenderTextureFormat,
                             QSSGByteView) override
    {
    }
    void setCompressedTextureData2D(QSSGRenderBackendTextureObject,
                                    QSSGRenderTextureTargetType,
                                    qint32,
                                    QSSGRenderTextureFormat,
                                    qint32,
                                    qint32,
                                    qint32,
                                    QSSGByteView) override
    {
    }
    void setCompressedTextureDataCubeFace(QSSGRenderBackendTextureObject,
                                          QSSGRenderTextureTargetType,
                                          qint32,
                                          QSSGRenderTextureFormat,
                                          qint32,
                                          qint32,
                                          qint32,
                                          QSSGByteView) override
    {
    }
    void setCompressedTextureSubData2D(QSSGRenderBackendTextureObject,
                                       QSSGRenderTextureTargetType,
                                       qint32,
                                       qint32,
                                       qint32,
                                       qint32,
                                       qint32,
                                       QSSGRenderTextureFormat,
                                       QSSGByteView) override
    {
    }
    void setMultisampledTextureData2D(QSSGRenderBackendTextureObject,
                                      QSSGRenderTextureTargetType,
                                      qint32,
                                      QSSGRenderTextureFormat,
                                      qint32,
                                      qint32,
                                      bool) override
    {
    }
    void setTextureData3D(QSSGRenderBackendTextureObject,
                          QSSGRenderTextureTargetType,
                          qint32,
                          QSSGRenderTextureFormat,
                          qint32,
                          qint32,
                          qint32,
                          qint32,
                          QSSGRenderTextureFormat,
                          QSSGByteView) override
    {
    }
    void generateMipMaps(QSSGRenderBackendTextureObject, QSSGRenderTextureTargetType, QSSGRenderHint) override
    {
    }
    void bindTexture(QSSGRenderBackendTextureObject, QSSGRenderTextureTargetType, qint32) override {}
    void setActiveTexture(qint32) override {}
    void bindImageTexture(QSSGRenderBackendTextureObject, quint32, qint32, bool, qint32, QSSGRenderImageAccessType, QSSGRenderTextureFormat) override
    {
    }
    void releaseTexture(QSSGRenderBackendTextureObject) override {}

    QSSGRenderTextureSwizzleMode getTextureSwizzleMode(const QSSGRenderTextureFormat) const override
    {
        return QSSGRenderTextureSwizzleMode::NoSwizzle;
    }

    QSSGRenderBackendSamplerObject createSampler(QSSGRenderTextureMinifyingOp,
                                                   QSSGRenderTextureMagnifyingOp,
                                                   QSSGRenderTextureCoordOp,
                                                   QSSGRenderTextureCoordOp,
                                                   QSSGRenderTextureCoordOp,
                                                   qint32,
                                                   qint32,
                                                   float,
                                                   QSSGRenderTextureCompareMode,
                                                   QSSGRenderTextureCompareOp,
                                                   float,
                                                   float *) override
    {
        return QSSGRenderBackendSamplerObject(1);
    }

    void updateSampler(QSSGRenderBackendSamplerObject,
                       QSSGRenderTextureTargetType,
                       QSSGRenderTextureMinifyingOp,
                       QSSGRenderTextureMagnifyingOp,
                       QSSGRenderTextureCoordOp,
                       QSSGRenderTextureCoordOp,
                       QSSGRenderTextureCoordOp,
                       float,
                       float,
                       float,
                       QSSGRenderTextureCompareMode,
                       QSSGRenderTextureCompareOp,
                       float,
                       float *) override
    {
    }

    void updateTextureObject(QSSGRenderBackendTextureObject, QSSGRenderTextureTargetType, qint32, qint32) override
    {
    }

    void updateTextureSwizzle(QSSGRenderBackendTextureObject, QSSGRenderTextureTargetType, QSSGRenderTextureSwizzleMode) override
    {
    }

    void releaseSampler(QSSGRenderBackendSamplerObject) override {}

    QSSGRenderBackendAttribLayoutObject createAttribLayout(QSSGDataView<QSSGRenderVertexBufferEntry>) override
    {
        return QSSGRenderBackendAttribLayoutObject(1);
    }

    void releaseAttribLayout(QSSGRenderBackendAttribLayoutObject) override {}

    QSSGRenderBackendInputAssemblerObject createInputAssembler(QSSGRenderBackendAttribLayoutObject,
                                                                 QSSGDataView<QSSGRenderBackendBufferObject>,
                                                                 const QSSGRenderBackendBufferObject,
                                                                 QSSGDataView<quint32>,
                                                                 QSSGDataView<quint32>,
                                                                 quint32) override
    {
        return QSSGRenderBackendInputAssemblerObject(1);
    }
    void releaseInputAssembler(QSSGRenderBackendInputAssemblerObject) override {}
    bool setInputAssembler(QSSGRenderBackendInputAssemblerObject, QSSGRenderBackendShaderProgramObject) override
    {
        return false;
    }
    void resetStates() override {};
    void setPatchVertexCount(QSSGRenderBackendInputAssemblerObject, quint32) override {}
    QSSGRenderBackendVertexShaderObject createVertexShader(QSSGByteView, QByteArray &, bool) override
    {
        return QSSGRenderBackendVertexShaderObject(1);
    }
    void releaseVertexShader(QSSGRenderBackendVertexShaderObject) override {}
    QSSGRenderBackendFragmentShaderObject createFragmentShader(QSSGByteView, QByteArray &, bool) override
    {
        return QSSGRenderBackendFragmentShaderObject(1);
    }
    void releaseFragmentShader(QSSGRenderBackendFragmentShaderObject) override {}
    QSSGRenderBackendTessControlShaderObject createTessControlShader(QSSGByteView, QByteArray &, bool) override
    {
        return QSSGRenderBackendTessControlShaderObject(1);
    }
    void releaseTessControlShader(QSSGRenderBackendTessControlShaderObject) override {}
    QSSGRenderBackendTessEvaluationShaderObject createTessEvaluationShader(QSSGByteView, QByteArray &, bool) override
    {
        return QSSGRenderBackendTessEvaluationShaderObject(1);
    }
    void releaseTessEvaluationShader(QSSGRenderBackendTessEvaluationShaderObject) override {}
    QSSGRenderBackendGeometryShaderObject createGeometryShader(QSSGByteView, QByteArray &, bool) override
    {
        return QSSGRenderBackendGeometryShaderObject(1);
    }
    void releaseGeometryShader(QSSGRenderBackendGeometryShaderObject) override {}
    QSSGRenderBackendComputeShaderObject createComputeShader(QSSGByteView, QByteArray &, bool) override
    {
        return QSSGRenderBackendComputeShaderObject(1);
    }
    void releaseComputeShader(QSSGRenderBackendComputeShaderObject) override {}
    void attachShader(QSSGRenderBackendShaderProgramObject, QSSGRenderBackendVertexShaderObject) override {}
    void attachShader(QSSGRenderBackendShaderProgramObject, QSSGRenderBackendFragmentShaderObject) override {}
    void attachShader(QSSGRenderBackendShaderProgramObject, QSSGRenderBackendTessControlShaderObject) override {}
    void attachShader(QSSGRenderBackendShaderProgramObject, QSSGRenderBackendTessEvaluationShaderObject) override {}
    void attachShader(QSSGRenderBackendShaderProgramObject, QSSGRenderBackendGeometryShaderObject) override {}
    void attachShader(QSSGRenderBackendShaderProgramObject, QSSGRenderBackendComputeShaderObject) override {}
    void detachShader(QSSGRenderBackendShaderProgramObject, QSSGRenderBackendVertexShaderObject) override {}
    void detachShader(QSSGRenderBackendShaderProgramObject, QSSGRenderBackendFragmentShaderObject) override {}
    void detachShader(QSSGRenderBackendShaderProgramObject, QSSGRenderBackendTessControlShaderObject) override {}
    void detachShader(QSSGRenderBackendShaderProgramObject, QSSGRenderBackendTessEvaluationShaderObject) override {}
    void detachShader(QSSGRenderBackendShaderProgramObject, QSSGRenderBackendGeometryShaderObject) override {}
    void detachShader(QSSGRenderBackendShaderProgramObject, QSSGRenderBackendComputeShaderObject) override {}
    QSSGRenderBackendShaderProgramObject createShaderProgram(bool) override
    {
        return QSSGRenderBackendShaderProgramObject(1);
    }
    void releaseShaderProgram(QSSGRenderBackendShaderProgramObject) override {}
    QSSGRenderBackendProgramPipeline createProgramPipeline() override
    {
        return QSSGRenderBackendProgramPipeline(1);
    }
    void releaseProgramPipeline(QSSGRenderBackendProgramPipeline) override {}

    bool linkProgram(QSSGRenderBackendShaderProgramObject, QByteArray &) override { return false; }
    bool linkProgram(QSSGRenderBackendShaderProgramObject , QByteArray &, quint32 , const QByteArray &) override { return false; }
    void getProgramBinary(QSSGRenderBackendShaderProgramObject ,quint32 &, QByteArray &) override {};
    void setActiveProgram(QSSGRenderBackendShaderProgramObject) override {}
    void setActiveProgramPipeline(QSSGRenderBackendProgramPipeline) override {}
    void setProgramStages(QSSGRenderBackendProgramPipeline, QSSGRenderShaderTypeFlags, QSSGRenderBackendShaderProgramObject) override
    {
    }
    void dispatchCompute(QSSGRenderBackendShaderProgramObject, quint32, quint32, quint32) override {}
    qint32 getConstantCount(QSSGRenderBackendShaderProgramObject) override { return 0; }
    qint32 getConstantBufferCount(QSSGRenderBackendShaderProgramObject) override { return 0; }
    qint32 getConstantInfoByID(QSSGRenderBackendShaderProgramObject,
                               quint32,
                               quint32,
                               qint32 *,
                               QSSGRenderShaderDataType *,
                               qint32 *,
                               char *) override
    {
        return 0;
    }

    qint32 getConstantBufferInfoByID(QSSGRenderBackendShaderProgramObject, quint32, quint32, qint32 *, qint32 *, qint32 *, char *) override
    {
        return 0;
    }

    void getConstantBufferParamIndices(QSSGRenderBackendShaderProgramObject, quint32, qint32 *) override {}
    void getConstantBufferParamInfoByIndices(QSSGRenderBackendShaderProgramObject, quint32, quint32 *, QSSGRenderShaderDataType *, qint32 *, qint32 *) override
    {
    }
    void programSetConstantBlock(QSSGRenderBackendShaderProgramObject, quint32, quint32) override {}
    void programSetConstantBuffer(quint32, QSSGRenderBackendBufferObject) override {}

    qint32 getStorageBufferCount(QSSGRenderBackendShaderProgramObject) override { return 0; }
    qint32 getStorageBufferInfoByID(QSSGRenderBackendShaderProgramObject, quint32, quint32, qint32 *, qint32 *, qint32 *, char *) override
    {
        return -1;
    }
    void programSetStorageBuffer(quint32, QSSGRenderBackendBufferObject) override {}

    void setConstantValue(QSSGRenderBackendShaderProgramObject, quint32, QSSGRenderShaderDataType, qint32, const void *, bool) override
    {
    }

    void draw(QSSGRenderDrawMode, quint32, quint32) override {}

    void drawIndexed(QSSGRenderDrawMode, quint32, QSSGRenderComponentType, const void *) override {}

    void readPixel(QSSGRenderBackendRenderTargetObject, qint32, qint32, qint32, qint32, QSSGRenderReadPixelFormat, QSSGByteRef) override
    {
    }
    QSurfaceFormat format() const override { return QSurfaceFormat(); }
};
}

QSSGRef<QSSGRenderBackend> QSSGRenderBackendNULL::createBackend()
{
    return QSSGRef<QSSGRenderBackend>(new QSSGNullBackend());
}

QT_END_NAMESPACE
