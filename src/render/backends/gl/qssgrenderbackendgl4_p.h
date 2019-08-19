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

#ifndef QSSG_RENDER_BACKEND_GL4_H
#define QSSG_RENDER_BACKEND_GL4_H

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

/// @file QSSGrenderbackendgl4.h
///       NVRender OpenGL 4 backend definition.

#include <QtQuick3DRender/private/qssgrenderbackendgl3_p.h>

QT_BEGIN_NAMESPACE

class QSSGRenderBackendGL4Impl : public QSSGRenderBackendGL3Impl
{
public:
    /// constructor
    QSSGRenderBackendGL4Impl(const QSurfaceFormat &format);
    /// destructor
    virtual ~QSSGRenderBackendGL4Impl();

public:
    void drawIndirect(QSSGRenderDrawMode drawMode, const void *indirect) override;
    void drawIndexedIndirect(QSSGRenderDrawMode drawMode, QSSGRenderComponentType type, const void *indirect) override;

    void createTextureStorage2D(QSSGRenderBackendTextureObject to,
                                QSSGRenderTextureTargetType target,
                                qint32 levels,
                                QSSGRenderTextureFormat internalFormat,
                                qint32 width,
                                qint32 height) override;

    void setMultisampledTextureData2D(QSSGRenderBackendTextureObject to,
                                      QSSGRenderTextureTargetType target,
                                      qint32 samples,
                                      QSSGRenderTextureFormat internalFormat,
                                      qint32 width,
                                      qint32 height,
                                      bool fixedsamplelocations) override;

    void setConstantValue(QSSGRenderBackendShaderProgramObject po,
                          quint32 id,
                          QSSGRenderShaderDataType type,
                          qint32 count,
                          const void *value,
                          bool transpose) override;

    void setPatchVertexCount(QSSGRenderBackendInputAssemblerObject iao, quint32 count) override;
    virtual QSSGRenderBackendTessControlShaderObject createTessControlShader(QSSGByteView source,
                                                                               QByteArray &errorMessage,
                                                                               bool binary) override;
    virtual QSSGRenderBackendTessEvaluationShaderObject createTessEvaluationShader(QSSGByteView source,
                                                                                     QByteArray &errorMessage,
                                                                                     bool binary) override;
    virtual QSSGRenderBackendGeometryShaderObject createGeometryShader(QSSGByteView source,
                                                                         QByteArray &errorMessage,
                                                                         bool binary) override;

    qint32 getStorageBufferCount(QSSGRenderBackendShaderProgramObject po) override;
    qint32 getStorageBufferInfoByID(QSSGRenderBackendShaderProgramObject po,
                                    quint32 id,
                                    quint32 nameBufSize,
                                    qint32 *paramCount,
                                    qint32 *bufferSize,
                                    qint32 *length,
                                    char *nameBuf) override;
    void programSetStorageBuffer(quint32 index, QSSGRenderBackendBufferObject bo) override;

    qint32 getAtomicCounterBufferCount(QSSGRenderBackendShaderProgramObject po) override;
    qint32 getAtomicCounterBufferInfoByID(QSSGRenderBackendShaderProgramObject po,
                                          quint32 id,
                                          quint32 nameBufSize,
                                          qint32 *paramCount,
                                          qint32 *bufferSize,
                                          qint32 *length,
                                          char *nameBuf) override;
    void programSetAtomicCounterBuffer(quint32 index, QSSGRenderBackendBufferObject bo) override;

    void setMemoryBarrier(QSSGRenderBufferBarrierFlags barriers) override;
    void bindImageTexture(QSSGRenderBackendTextureObject to,
                          quint32 unit,
                          qint32 level,
                          bool layered,
                          qint32 layer,
                          QSSGRenderImageAccessType access,
                          QSSGRenderTextureFormat format) override;

    virtual QSSGRenderBackendComputeShaderObject createComputeShader(QSSGByteView source,
                                                                       QByteArray &errorMessage,
                                                                       bool binary) override;
    void dispatchCompute(QSSGRenderBackendShaderProgramObject po, quint32 numGroupsX, quint32 numGroupsY, quint32 numGroupsZ) override;

    QSSGRenderBackendProgramPipeline createProgramPipeline() override;
    void releaseProgramPipeline(QSSGRenderBackendProgramPipeline ppo) override;
    void setActiveProgramPipeline(QSSGRenderBackendProgramPipeline ppo) override;
    void setProgramStages(QSSGRenderBackendProgramPipeline ppo,
                          QSSGRenderShaderTypeFlags flags,
                          QSSGRenderBackendShaderProgramObject po) override;

    void setBlendEquation(const QSSGRenderBlendEquationArgument &pBlendEquArg) override;
    void setBlendBarrier(void) override;

    QSSGRenderBackendPathObject createPathNVObject(size_t range) override;
    void setPathSpecification(QSSGRenderBackendPathObject inPathObject,
                              QSSGByteView inPathCommands,
                              QSSGDataView<float> inPathCoords) override;
    QSSGBounds3 getPathObjectBoundingBox(QSSGRenderBackendPathObject inPathObject) override;
    QSSGBounds3 getPathObjectFillBox(QSSGRenderBackendPathObject inPathObject) override;
    QSSGBounds3 getPathObjectStrokeBox(QSSGRenderBackendPathObject inPathObject) override;
    void setStrokeWidth(QSSGRenderBackendPathObject inPathObject, float inStrokeWidth) override;

    void setPathProjectionMatrix(const QMatrix4x4 inPathProjection) override;
    void setPathModelViewMatrix(const QMatrix4x4 inPathModelview) override;
    void setPathStencilDepthOffset(float inSlope, float inBias) override;
    void setPathCoverDepthFunc(QSSGRenderBoolOp inDepthFunction) override;
    void stencilStrokePath(QSSGRenderBackendPathObject inPathObject) override;
    void stencilFillPath(QSSGRenderBackendPathObject inPathObject) override;
    void releasePathNVObject(QSSGRenderBackendPathObject po, size_t range) override;

    void stencilFillPathInstanced(QSSGRenderBackendPathObject po,
                                  size_t numPaths,
                                  QSSGRenderPathFormatType type,
                                  const void *charCodes,
                                  QSSGRenderPathFillMode fillMode,
                                  quint32 stencilMask,
                                  QSSGRenderPathTransformType transformType,
                                  const float *transformValues) override;
    void stencilStrokePathInstancedN(QSSGRenderBackendPathObject po,
                                     size_t numPaths,
                                     QSSGRenderPathFormatType type,
                                     const void *charCodes,
                                     qint32 stencilRef,
                                     quint32 stencilMask,
                                     QSSGRenderPathTransformType transformType,
                                     const float *transformValues) override;
    void coverFillPathInstanced(QSSGRenderBackendPathObject po,
                                size_t numPaths,
                                QSSGRenderPathFormatType type,
                                const void *charCodes,
                                QSSGRenderPathCoverMode coverMode,
                                QSSGRenderPathTransformType transformType,
                                const float *transformValues) override;
    void coverStrokePathInstanced(QSSGRenderBackendPathObject po,
                                  size_t numPaths,
                                  QSSGRenderPathFormatType type,
                                  const void *charCodes,
                                  QSSGRenderPathCoverMode coverMode,
                                  QSSGRenderPathTransformType transformType,
                                  const float *transformValues) override;
    void loadPathGlyphs(QSSGRenderBackendPathObject po,
                        QSSGRenderPathFontTarget fontTarget,
                        const void *fontName,
                        QSSGRenderPathFontStyleFlags fontStyle,
                        size_t numGlyphs,
                        QSSGRenderPathFormatType type,
                        const void *charCodes,
                        QSSGRenderPathMissingGlyphs handleMissingGlyphs,
                        QSSGRenderBackendPathObject pathParameterTemplate,
                        float emScale) override;
    virtual QSSGRenderPathReturnValues loadPathGlyphsIndexed(QSSGRenderBackendPathObject po,
                                                                     QSSGRenderPathFontTarget fontTarget,
                                                                     const void *fontName,
                                                                     QSSGRenderPathFontStyleFlags fontStyle,
                                                                     quint32 firstGlyphIndex,
                                                                     size_t numGlyphs,
                                                                     QSSGRenderBackendPathObject pathParameterTemplate,
                                                                     float emScale) override;
    virtual QSSGRenderBackendPathObject loadPathGlyphsIndexedRange(QSSGRenderPathFontTarget fontTarget,
                                                                     const void *fontName,
                                                                     QSSGRenderPathFontStyleFlags fontStyle,
                                                                     QSSGRenderBackendPathObject pathParameterTemplate,
                                                                     float emScale,
                                                                     quint32 *count) override;
    void loadPathGlyphRange(QSSGRenderBackendPathObject po,
                            QSSGRenderPathFontTarget fontTarget,
                            const void *fontName,
                            QSSGRenderPathFontStyleFlags fontStyle,
                            quint32 firstGlyph,
                            size_t numGlyphs,
                            QSSGRenderPathMissingGlyphs handleMissingGlyphs,
                            QSSGRenderBackendPathObject pathParameterTemplate,
                            float emScale) override;
    void getPathMetrics(QSSGRenderBackendPathObject po,
                        size_t numPaths,
                        QSSGRenderPathGlyphFontMetricFlags metricQueryMask,
                        QSSGRenderPathFormatType type,
                        const void *charCodes,
                        size_t stride,
                        float *metrics) override;
    void getPathMetricsRange(QSSGRenderBackendPathObject po,
                             size_t numPaths,
                             QSSGRenderPathGlyphFontMetricFlags metricQueryMask,
                             size_t stride,
                             float *metrics) override;
    void getPathSpacing(QSSGRenderBackendPathObject po,
                        size_t numPaths,
                        QSSGRenderPathListMode pathListMode,
                        QSSGRenderPathFormatType type,
                        const void *charCodes,
                        float advanceScale,
                        float kerningScale,
                        QSSGRenderPathTransformType transformType,
                        float *spacing) override;

private:
#if !defined(QT_OPENGL_ES)
    QOpenGLExtension_NV_path_rendering *m_nvPathRendering;
    QOpenGLExtension_EXT_direct_state_access *m_directStateAccess;
#endif
};

QT_END_NAMESPACE

#endif
