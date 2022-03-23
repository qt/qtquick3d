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

#ifndef QSSG_RENDERER_H
#define QSSG_RENDERER_H

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

#include <QtQuick3DRuntimeRender/private/qssgrenderableobjects_p.h>
#include <QtQuick3DRuntimeRender/private/qssglayerrenderdata_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermesh_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermodel_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderray_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderclippingfrustum_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderkeys_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderpickresult_p.h>
#include <QtQuick3DRuntimeRender/private/qssgshadermapkey_p.h>

#include <QtQuick3DUtils/private/qssgbounds3_p.h>
#include <QtQuick3DUtils/private/qssgoption_p.h>
#include <QtQuick3DUtils/private/qssgdataref_p.h>

QT_BEGIN_NAMESPACE

class QSSGRhiQuadRenderer;

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderer
{
    using PickResultList = QVarLengthArray<QSSGRenderPickResult, 20>; // Lets assume most items are filtered out already

public:
    QAtomicInt ref;
    QSSGRenderer();
    ~QSSGRenderer();

    QSSGShaderDefaultMaterialKeyProperties &defaultMaterialShaderKeyProperties()
    {
        return m_defaultMaterialShaderKeyProperties;
    }

    void setRenderContextInterface(QSSGRenderContextInterface *ctx);

    // Returns true if this layer or a sibling was dirty.
    bool prepareLayerForRender(QSSGRenderLayer &inLayer);

    void rhiPrepare(QSSGRenderLayer &inLayer);
    void rhiRender(QSSGRenderLayer &inLayer);

    void cleanupResources(QList<QSSGRenderGraphObject*> &resources);

    QSSGLayerRenderData *getOrCreateLayerRenderData(QSSGRenderLayer &layer);

    // The QSSGRenderContextInterface calls these, clients should not.
    void beginFrame();
    void endFrame();

    PickResultList syncPickAll(const QSSGRenderLayer &layer,
                               const QSSGRef<QSSGBufferManager> &bufferManager,
                               const QSSGRenderRay &ray);

    QSSGRenderPickResult syncPick(const QSSGRenderLayer &layer,
                                  const QSSGRef<QSSGBufferManager> &bufferManager,
                                  const QSSGRenderRay &ray,
                                  QSSGRenderNode *target = nullptr);

    // Setting this true enables picking for all the models, regardless of
    // the models pickable property.
    void setGlobalPickingEnabled(bool isEnabled);

    QSSGRhiQuadRenderer *rhiQuadRenderer();

    // Callback during the layer render process.
    void beginLayerDepthPassRender(QSSGLayerRenderData &inLayer);
    void endLayerDepthPassRender();
    void beginLayerRender(QSSGLayerRenderData &inLayer);
    void endLayerRender();
    void addMaterialDirtyClear(QSSGRenderGraphObject *material);

    static QSSGRef<QSSGRhiShaderPipeline> generateRhiShaderPipelineImpl(QSSGSubsetRenderable &renderable, const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager,
                                                                        const QSSGRef<QSSGShaderCache> &shaderCache,
                                                                        const QSSGRef<QSSGProgramGenerator> &shaderProgramGenerator,
                                                                        QSSGShaderDefaultMaterialKeyProperties &shaderKeyProperties,
                                                                        const QSSGShaderFeatures &featureSet,
                                                                        QByteArray &shaderString);

    QSSGRef<QSSGRhiShaderPipeline> getRhiShaders(QSSGSubsetRenderable &inRenderable,
                                               const QSSGShaderFeatures &inFeatureSet);

    QSSGLayerGlobalRenderProperties getLayerGlobalRenderProperties();

    QSSGRenderContextInterface *contextInterface() { return m_contextInterface; }

    // Returns true if the renderer expects new frame to be rendered
    // Happens when progressive AA is enabled
    bool rendererRequestsFrames() const;

    // shader implementations, RHI, implemented in qssgrendererimplshaders_rhi.cpp
    QSSGRef<QSSGRhiShaderPipeline> getRhiCubemapShadowBlurXShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiCubemapShadowBlurYShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiOrthographicShadowBlurXShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiOrthographicShadowBlurYShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiSsaoShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiSkyBoxShader(QSSGRenderLayer::TonemapMode tonemapMode, bool isRGBE);
    QSSGRef<QSSGRhiShaderPipeline> getRhiSupersampleResolveShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiProgressiveAAShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiTexturedQuadShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiParticleShader(QSSGRenderParticles::FeatureLevel featureLevel);
    QSSGRef<QSSGRhiShaderPipeline> getRhiSimpleQuadShader();

protected:
    static void getLayerHitObjectList(const QSSGRenderLayer &layer,
                                      const QSSGRef<QSSGBufferManager> &bufferManager,
                                      const QSSGRenderRay &ray,
                                      bool inPickEverything,
                                      PickResultList &outIntersectionResult);
    static void intersectRayWithSubsetRenderable(const QSSGRef<QSSGBufferManager> &bufferManager,
                                                 const QSSGRenderRay &inRay,
                                                 const QSSGRenderNode &node,
                                                 PickResultList &outIntersectionResultList);
    static void intersectRayWithItem2D(const QSSGRenderRay &inRay, const QSSGRenderItem2D &item2D, PickResultList &outIntersectionResultList);

private:
    friend class QSSGRenderContextInterface;
    void releaseResources();
    QSSGRef<QSSGRhiShaderPipeline> getBuiltinRhiShader(const QByteArray &name,
                                                       QSSGRef<QSSGRhiShaderPipeline> &storage);
    QSSGRef<QSSGRhiShaderPipeline> generateRhiShaderPipeline(QSSGSubsetRenderable &inRenderable, const QSSGShaderFeatures &inFeatureSet);

    QSSGRenderContextInterface *m_contextInterface = nullptr; //  We're own by the context interface

    // The shader refs are non-null if we have attempted to generate the
    // shader. This does not mean we were successul, however.

    // RHI
    QSSGRef<QSSGRhiShaderPipeline> m_cubemapShadowBlurXRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_cubemapShadowBlurYRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_orthographicShadowBlurXRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_orthographicShadowBlurYRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_ssaoRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_skyBoxRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_supersampleResolveRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_progressiveAARhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_texturedQuadRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_simpleQuadRhiShader;

    QSSGRef<QSSGRhiShaderPipeline> m_particlesNoLightingSimpleRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_particlesNoLightingMappedRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_particlesNoLightingAnimatedRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_particlesVLightingSimpleRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_particlesVLightingMappedRhiShader;
    QSSGRef<QSSGRhiShaderPipeline> m_particlesVLightingAnimatedRhiShader;

    bool m_globalPickingEnabled = false;

    // Temporary information stored only when rendering a particular layer.
    QSSGLayerRenderData *m_currentLayer = nullptr;
    QMatrix4x4 m_viewProjection;
    QByteArray m_generatedShaderString;

    bool m_progressiveAARenderRequest = false;
    QSSGShaderDefaultMaterialKeyProperties m_defaultMaterialShaderKeyProperties;

    QSet<QSSGRenderGraphObject *> m_materialClearDirty;

    QSSGRhiQuadRenderer *m_rhiQuadRenderer = nullptr;

    QHash<QSSGShaderMapKey, QSSGRef<QSSGRhiShaderPipeline>> m_shaderMap;

    // Skybox shader state
    QSSGRenderLayer::TonemapMode m_skyboxTonemapMode = QSSGRenderLayer::TonemapMode::None;
    bool m_isSkyboxRGBE = false;
};
QT_END_NAMESPACE

#endif
