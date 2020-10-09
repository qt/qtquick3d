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

#ifndef QSSG_RHI_CUSTOM_MATERIAL_SYSTEM_H
#define QSSG_RHI_CUSTOM_MATERIAL_SYSTEM_H

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
#include <QtQuick3DRuntimeRender/private/qssgrendershaderlibrarymanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgvertexpipelineimpl_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderableobjects_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderkeys_p.h>
#include <QtQuick3DRuntimeRender/private/qssgshadermapkey_p.h>
#include <QtCore/qhash.h>
#include <QtGui/QMatrix4x4>
#include <QtGui/QMatrix3x3>

QT_BEGIN_NAMESPACE

struct QSSGRenderCustomMaterial;
struct QSSGRenderSubset;
struct QSSGRenderModel;
struct QSSGLayerRenderData;
struct QSSGRenderableImage;
struct QSSGRenderLayer;
struct QSSGRenderLight;
struct QSSGRenderCamera;
class QRhiTexture;

struct QSSGCustomMaterialRenderContext
{
    // The lights and camera will not change per layer,
    // so that information can be set once for all the shaders.
    const QSSGRenderLayer &layer;
    const QSSGLayerRenderData &layerData;
    const QSSGShaderLightList &lights;
    QSSGRenderCamera &camera;

    // Per-object information.
    const QSSGRenderModel &model;
    const QSSGRenderSubset &subset;
    const QMatrix4x4 &modelViewProjection;
    const QMatrix4x4 &modelMatrix; ///< model to world transformation
    const QMatrix3x3 &normalMatrix;
    const QSSGRenderCustomMaterial &material;
    QRhiTexture *rhiDepthTexture;
    QRhiTexture *rhiAoTexture;
    QRhiTexture *rhiScreenTexture;
    QSSGShaderDefaultMaterialKey materialKey;
    QSSGRenderableImage *firstImage;
    float opacity;

    QSSGCustomMaterialRenderContext(const QSSGRenderLayer &inLayer,
                                      const QSSGLayerRenderData &inData,
                                      const QSSGShaderLightList &inLights,
                                      QSSGRenderCamera &inCamera,
                                      const QSSGRenderModel &inModel,
                                      const QSSGRenderSubset &inSubset,
                                      const QMatrix4x4 &inMvp,
                                      const QMatrix4x4 &inWorld,
                                      const QMatrix3x3 &inNormal,
                                      const QSSGRenderCustomMaterial &inMaterial,
                                      QRhiTexture *inRhiDepthTex,
                                      QRhiTexture *inRhiAoTex,
                                      QRhiTexture *inRhiScreenTex,
                                      QSSGShaderDefaultMaterialKey inMaterialKey,
                                      QSSGRenderableImage *inFirstImage = nullptr,
                                      float inOpacity = 1.0);
    ~QSSGCustomMaterialRenderContext();
};

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGCustomMaterialSystem
{
    Q_DISABLE_COPY(QSSGCustomMaterialSystem)
public:
    QAtomicInt ref;

private:
    typedef QPair<QByteArray, QByteArray> TStrStrPair;
    typedef QHash<QSSGShaderMapKey, QSSGRef<QSSGRhiShaderPipeline>> TShaderMap;

    QSSGRenderContextInterface *context = nullptr;
    TShaderMap shaderMap;

    QSSGLayerGlobalRenderProperties getLayerGlobalRenderProperties(QSSGCustomMaterialRenderContext &inRenderContext);

    void setShaderResources(char *ubufData,
                            const QSSGRenderCustomMaterial &inMaterial,
                            const QByteArray &inPropertyName,
                            const QVariant &propertyValue,
                            QSSGRenderShaderDataType inPropertyType,
                            const QSSGRef<QSSGRhiShaderPipeline> &shaderPipeline);

public:
    QSSGCustomMaterialSystem();
    ~QSSGCustomMaterialSystem();

    void setRenderContextInterface(QSSGRenderContextInterface *inContext);

    // Returns true if the material is dirty and thus will produce a different render result
    // than previously.  This effects things like progressive AA.
    bool prepareForRender(const QSSGRenderModel &inModel,
                          const QSSGRenderSubset &inSubset,
                          QSSGRenderCustomMaterial &inMaterial);

    QSSGRef<QSSGRhiShaderPipeline> shadersForCustomMaterial(QSSGRhiGraphicsPipelineState *ps,
                                                            const QSSGRenderCustomMaterial &material,
                                                            QSSGCustomMaterialRenderable &renderable,
                                                            const ShaderFeatureSetList &featureSet);

    void updateUniformsForCustomMaterial(QSSGRef<QSSGRhiShaderPipeline> &shaderPipeline,
                                         QSSGRhiContext *rhiCtx,
                                         char *ubufData,
                                         QSSGRhiGraphicsPipelineState *ps,
                                         const QSSGRenderCustomMaterial &material,
                                         QSSGCustomMaterialRenderable &renderable,
                                         QSSGLayerRenderData &layerData,
                                         QSSGRenderCamera &camera,
                                         const QVector2D *depthAdjust,
                                         const QMatrix4x4 *alteredModelViewProjection);

    void rhiPrepareRenderable(QSSGRhiGraphicsPipelineState *ps,
                              QSSGCustomMaterialRenderable &renderable,
                              const ShaderFeatureSetList &featureSet,
                              const QSSGRenderCustomMaterial &material,
                              QSSGLayerRenderData &layerData,
                              QRhiRenderPassDescriptor *renderPassDescriptor,
                              int samples);
    void applyRhiShaderPropertyValues(char *ubufData,
                                      const QSSGRenderCustomMaterial &inMaterial,
                                      const QSSGRef<QSSGRhiShaderPipeline> &shaderPipeline);
    void rhiRenderRenderable(QSSGRhiContext *rhiCtx,
                             QSSGCustomMaterialRenderable &renderable,
                             QSSGLayerRenderData &inData,
                             bool *needsSetViewport);
};

QT_END_NAMESPACE

#endif
