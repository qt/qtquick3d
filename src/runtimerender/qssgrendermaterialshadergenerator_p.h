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

#ifndef QSSG_RENDER_MATERIAL_SHADER_GENERATOR_H
#define QSSG_RENDER_MATERIAL_SHADER_GENERATOR_H

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

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>
#include <QtQuick3DUtils/private/qssgdataref_p.h>

#include <QtGui/QVector4D>
#include <QtGui/QMatrix4x4>

#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderkeys_p.h>

QT_BEGIN_NAMESPACE

struct QSSGRenderLayer;
struct QSSGRenderCamera;
struct QSSGRenderLight;
class QSSGRenderShadowMap;
struct QSSGRenderImage;
class QSSGShaderStageGeneratorInterface;
struct QSSGRenderableImage;
struct QSSGRenderGraphObject;
struct QSSGShaderDefaultMaterialKey;
class QSSGRenderContextInterface;
class QSSGShaderProgramGeneratorInterface;
class QSSGDefaultMaterialVertexPipelineInterface;
class QSSGRhiShaderStagesWithResources;
struct QSSGRhiGraphicsPipelineState;

struct QSSGLayerGlobalRenderProperties
{
    const QSSGRenderLayer &layer;
    QSSGRenderCamera &camera;
    QVector3D cameraDirection;
    const QVector<QSSGRenderLight *> &lights;
    const QVector<QVector3D> &lightDirections;
    QSSGRef<QSSGRenderShadowMap> shadowMapManager;
    QRhiTexture *rhiDepthTexture;
    QRhiTexture *rhiSsaoTexture;
    QSSGRenderImage *lightProbe;
    QSSGRenderImage *lightProbe2;
    float probeHorizon;
    float probeBright;
    float probe2Window;
    float probe2Pos;
    float probe2Fade;
    float probeFOV;
    bool isYUpInFramebuffer;
};

class QSSGMaterialShaderGeneratorInterface
{
    Q_DISABLE_COPY(QSSGMaterialShaderGeneratorInterface)
public:
    QAtomicInt ref;

protected:
    bool m_hasTransparency = false;
    QSSGRenderContextInterface *m_renderContext;
    const QSSGRef<QSSGShaderProgramGeneratorInterface> m_programGenerator;

    QSSGShaderDefaultMaterialKey *m_currentKey = nullptr;
    QSSGDefaultMaterialVertexPipelineInterface *m_currentPipeline = nullptr;
    ShaderFeatureSetList m_currentFeatureSet;
    QVector<QSSGRenderLight *> m_lights;
    QSSGRenderableImage *m_firstImage = nullptr;
    QSSGShaderDefaultMaterialKeyProperties m_defaultMaterialShaderKeyProperties;


protected:
    QSSGMaterialShaderGeneratorInterface(QSSGRenderContextInterface *renderContext);
public:
    virtual ~QSSGMaterialShaderGeneratorInterface();

    QSSGShaderDefaultMaterialKey &key() { return *m_currentKey; }

    struct ImageVariableNames
    {
        QByteArray m_imageSampler;
        QByteArray m_imageFragCoords;
    };

    virtual ImageVariableNames getImageVariableNames(quint32 inIdx) = 0;
    virtual void generateImageUVCoordinates(QSSGShaderStageGeneratorInterface &inVertexPipeline,
                                            quint32 idx,
                                            quint32 uvSet,
                                            QSSGRenderableImage &image) = 0;

    virtual QSSGRef<QSSGRhiShaderStages> generateRhiShaderStages(const QSSGRenderGraphObject &inMaterial,
                                                                 QSSGShaderDefaultMaterialKey inShaderDescription,
                                                                 QSSGShaderStageGeneratorInterface &inVertexPipeline,
                                                                 const ShaderFeatureSetList &inFeatureSet,
                                                                 const QVector<QSSGRenderLight *> &inLights,
                                                                 QSSGRenderableImage *inFirstImage,
                                                                 bool inHasTransparency,
                                                                 const QByteArray &inVertexPipelineName,
                                                                 const QByteArray &inCustomMaterialName = QByteArray()) = 0;

    virtual void setRhiMaterialProperties(QSSGRef<QSSGRhiShaderStagesWithResources> &inProgram,
                                          QSSGRhiGraphicsPipelineState *inPipelineState,
                                          const QSSGRenderGraphObject &inMaterial,
                                          const QVector2D &inCameraVec,
                                          const QMatrix4x4 &inModelViewProjection,
                                          const QMatrix3x3 &inNormalMatrix,
                                          const QMatrix4x4 &inGlobalTransform,
                                          QSSGRenderableImage *inFirstImage,
                                          float inOpacity,
                                          const QSSGLayerGlobalRenderProperties &inRenderProperties,
                                          bool receivesShadows = true) = 0;

    static QSSGRef<QSSGMaterialShaderGeneratorInterface> createCustomMaterialShaderGenerator(QSSGRenderContextInterface *inRenderContext);
};
QT_END_NAMESPACE

#endif
