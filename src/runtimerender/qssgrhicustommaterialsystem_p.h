// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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

QT_BEGIN_NAMESPACE

struct QSSGRenderCustomMaterial;
struct QSSGRenderSubset;
struct QSSGRenderModel;
class QSSGLayerRenderData;
struct QSSGRenderableImage;
struct QSSGRenderLayer;
struct QSSGRenderLight;
struct QSSGRenderCamera;
struct QSSGReflectionMapEntry;
class QRhiTexture;


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
                                                            QSSGSubsetRenderable &renderable,
                                                            const QSSGShaderFeatures &featureSet);

    void updateUniformsForCustomMaterial(QSSGRef<QSSGRhiShaderPipeline> &shaderPipeline,
                                         QSSGRhiContext *rhiCtx,
                                         char *ubufData,
                                         QSSGRhiGraphicsPipelineState *ps,
                                         const QSSGRenderCustomMaterial &material,
                                         QSSGSubsetRenderable &renderable,
                                         QSSGLayerRenderData &layerData,
                                         QSSGRenderCamera &camera,
                                         const QVector2D *depthAdjust,
                                         const QMatrix4x4 *alteredModelViewProjection);

    void rhiPrepareRenderable(QSSGRhiGraphicsPipelineState *ps,
                              QSSGSubsetRenderable &renderable,
                              const QSSGShaderFeatures &featureSet,
                              const QSSGRenderCustomMaterial &material,
                              QSSGLayerRenderData &layerData,
                              QRhiRenderPassDescriptor *renderPassDescriptor,
                              int samples,
                              QSSGRenderCamera *camera = nullptr,
                              int cubeFace = -1,
                              QMatrix4x4 *modelViewProjection = nullptr,
                              QSSGReflectionMapEntry *entry = nullptr);
    void applyRhiShaderPropertyValues(char *ubufData,
                                      const QSSGRenderCustomMaterial &inMaterial,
                                      const QSSGRef<QSSGRhiShaderPipeline> &shaderPipeline);
    void rhiRenderRenderable(QSSGRhiContext *rhiCtx,
                             QSSGSubsetRenderable &renderable,
                             QSSGLayerRenderData &inData,
                             bool *needsSetViewport,
                             int cubeFace,
                             QSSGRhiGraphicsPipelineState *state);
};

QT_END_NAMESPACE

#endif
