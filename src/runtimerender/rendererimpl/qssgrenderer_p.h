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
#include <QtQuick3DRuntimeRender/private/qssgrendererimpllayerrenderdata_p.h>
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
#include <QtQuick3DRuntimeRender/private/qssgshadermapkey_p.h>

#include <QtQuick3DUtils/private/qssgbounds3_p.h>
#include <QtQuick3DUtils/private/qssgoption_p.h>
#include <QtQuick3DUtils/private/qssgdataref_p.h>

#include <limits>

QT_BEGIN_NAMESPACE

class QSSGRhiQuadRenderer;

struct QSSGRenderPickResult
{
    const QSSGRenderGraphObject *m_hitObject = nullptr;
    float m_cameraDistanceSq = std::numeric_limits<float>::max();
    // The local coordinates in X,Y UV space where the hit occurred
    QVector2D m_localUVCoords;
    // The position in world coordinates
    QVector3D m_scenePosition;

    QSSGRenderPickResult(const QSSGRenderGraphObject &inHitObject,
                         float inCameraDistance,
                         const QVector2D &inLocalUVCoords,
                         const QVector3D &inScenePosition);
    QSSGRenderPickResult() = default;
};

Q_STATIC_ASSERT(std::is_trivially_destructible<QSSGRenderPickResult>::value);

struct QSSGPickResultProcessResult : public QSSGRenderPickResult
{
    QSSGPickResultProcessResult(const QSSGRenderPickResult &inSrc) : QSSGRenderPickResult(inSrc) {}
    QSSGPickResultProcessResult(const QSSGRenderPickResult &inSrc, bool consumed) : QSSGRenderPickResult(inSrc), m_wasPickConsumed(consumed) {}
    QSSGPickResultProcessResult() = default;
    bool m_wasPickConsumed = false;
};

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderer
{
    typedef QVector<QSSGLayerRenderData *> TLayerRenderList;
    typedef QVector<QSSGRenderPickResult> TPickResultArray;
    typedef QHash<QSSGShaderMapKey, QSSGRef<QSSGRhiShaderPipeline>> TShaderMap;

    using PickResultList = QVarLengthArray<QSSGRenderPickResult, 20>; // Lets assume most items are filtered out already

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

    TLayerRenderList m_lastFrameLayers;

    // Set from the first layer.
    TPickResultArray m_lastPickResults;

    // Temporary information stored only when rendering a particular layer.
    QSSGLayerRenderData *m_currentLayer = nullptr;
    QMatrix4x4 m_viewProjection;
    QByteArray m_generatedShaderString;

    bool m_progressiveAARenderRequest = false;
    QSSGShaderDefaultMaterialKeyProperties m_defaultMaterialShaderKeyProperties;

    QSet<QSSGRenderGraphObject *> m_materialClearDirty;

    QSSGRhiQuadRenderer *m_rhiQuadRenderer = nullptr;

    TShaderMap m_shaderMap;

public:
    QAtomicInt ref;
    QSSGRenderer();
    ~QSSGRenderer();

    typedef QHash<QSSGShaderMapKey, QSSGRef<QSSGRhiShaderPipeline>> ShaderMap;

    QSSGShaderDefaultMaterialKeyProperties &defaultMaterialShaderKeyProperties()
    {
        return m_defaultMaterialShaderKeyProperties;
    }

    void setRenderContextInterface(QSSGRenderContextInterface *ctx);

    // Returns true if this layer or a sibling was dirty.
    bool prepareLayerForRender(QSSGRenderLayer &inLayer, const QSize &surfaceSize);

    void rhiPrepare(QSSGRenderLayer &inLayer);
    void rhiRender(QSSGRenderLayer &inLayer);

    void cleanupResources(QList<QSSGRenderGraphObject*> &resources);

    QSSGRenderLayer *layerForNode(const QSSGRenderNode &inNode) const;
    QSSGLayerRenderData *getOrCreateLayerRenderData(QSSGRenderLayer &layer);

    // The QSSGRenderContextInterface calls these, clients should not.
    void beginFrame();
    void endFrame();

    QSSGRenderPickResult pick(QSSGRenderLayer &inLayer,
                                const QVector2D &inViewportDimensions,
                                const QVector2D &inMouseCoords,
                                bool inPickEverything = false);
    QSSGRenderPickResult syncPick(const QSSGRenderLayer &layer,
                                  const QSSGRef<QSSGBufferManager> &bufferManager,
                                  const QVector2D &inViewportDimensions,
                                  const QVector2D &inMouseCoords);

    // Return the relative hit position, in UV space, of a mouse pick against this object.
    // We need the node in order to figure out which layer rendered this object.
    // We need mapper objects if this is a in a subpresentation because we have to know how
    // to map the mouse coordinates into the subpresentation.  So for instance if inNode is in
    // a subpres then we need to know which image is displaying the subpres in order to map
    // the mouse coordinates into the subpres's render space.
    QSSGOption<QVector2D> facePosition(QSSGRenderNode &inNode,
                                       QSSGBounds3 inBounds,
                                       const QMatrix4x4 &inGlobalTransform,
                                       const QVector2D &inViewportDimensions,
                                       const QVector2D &inMouseCoords,
                                       QSSGDataView<QSSGRenderGraphObject *> inMapperObjects,
                                       QSSGRenderBasisPlanes inPlane);

    QVector3D unprojectToPosition(QSSGRenderNode &inNode, QVector3D &inPosition, const QVector2D &inMouseVec) const;
    QVector3D unprojectWithDepth(QSSGRenderNode &inNode, QVector3D &inPosition, const QVector3D &inMouseVec) const;
    QVector3D projectPosition(QSSGRenderNode &inNode, const QVector3D &inPosition) const;

    QSSGRhiQuadRenderer *rhiQuadRenderer();

    // Callback during the layer render process.
    void layerNeedsFrameClear(QSSGLayerRenderData &inLayer);
    void beginLayerDepthPassRender(QSSGLayerRenderData &inLayer);
    void endLayerDepthPassRender();
    void beginLayerRender(QSSGLayerRenderData &inLayer);
    void endLayerRender();
    //void prepareImageForIbl(QSSGRenderImage &inImage);
    void addMaterialDirtyClear(QSSGRenderGraphObject *material);

    static QSSGRef<QSSGRhiShaderPipeline> generateRhiShaderPipelineImpl(QSSGSubsetRenderable &renderable, const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager,
                                                                        const QSSGRef<QSSGShaderCache> &shaderCache,
                                                                        const QSSGRef<QSSGProgramGenerator> &shaderProgramGenerator,
                                                                        QSSGShaderDefaultMaterialKeyProperties &shaderKeyProperties,
                                                                        const ShaderFeatureSetList &featureSet,
                                                                        QByteArray &shaderString);

    QSSGRef<QSSGRhiShaderPipeline> generateRhiShaderPipeline(QSSGSubsetRenderable &inRenderable,
                                                             const ShaderFeatureSetList &inFeatureSet);
    QSSGRef<QSSGRhiShaderPipeline> getRhiShaders(QSSGSubsetRenderable &inRenderable,
                                               const ShaderFeatureSetList &inFeatureSet);

public:
    QSSGLayerRenderData *getLayerRenderData() { return m_currentLayer; }
    QSSGLayerGlobalRenderProperties getLayerGlobalRenderProperties();

    QSSGRenderContextInterface *contextInterface() { return m_contextInterface; }

    const QSSGRef<QSSGProgramGenerator> &getProgramGenerator();

    // Get the mouse coordinates as they relate to a given layer
    QSSGOption<QVector2D> getLayerMouseCoords(QSSGRenderLayer &inLayer,
                                                const QVector2D &inMouseCoords,
                                                const QVector2D &inViewportDimensions,
                                                bool forceImageIntersect = false) const;

    // Returns true if the renderer expects new frame to be rendered
    // Happens when progressive AA is enabled
    bool rendererRequestsFrames() const;

    static const QSSGRenderGraphObject *getPickObject(QSSGRenderableObject &inRenderableObject);
protected:
    QSSGOption<QVector2D> getLayerMouseCoords(QSSGLayerRenderData &inLayer,
                                                const QVector2D &inMouseCoords,
                                                const QVector2D &inViewportDimensions,
                                                bool forceImageIntersect = false) const;
    QSSGPickResultProcessResult processPickResultList(bool inPickEverything);
    // If the mouse y coordinates need to be flipped we expect that to happen before entry into
    // this function
    void getLayerHitObjectList(QSSGLayerRenderData &inLayer,
                               const QVector2D &inViewportDimensions,
                               const QVector2D &inMouseCoords,
                               bool inPickEverything,
                               TPickResultArray &outIntersectionResult);
    static void getLayerHitObjectList(const QSSGRenderLayer &layer,
                                      const QSSGRef<QSSGBufferManager> &bufferManager,
                                      const QVector2D &inViewportDimensions,
                                      const QVector2D &inMouseCoords,
                                      bool inPickEverything,
                                      PickResultList &outIntersectionResult);
    static void intersectRayWithSubsetRenderable(const QSSGRef<QSSGBufferManager> &bufferManager,
                                                 const QSSGRenderRay &inRay,
                                                 const QSSGRenderNode &node,
                                                 PickResultList &outIntersectionResultList);
    static void intersectRayWithSubsetRenderable(const QSSGRenderRay &inRay,
                                          QSSGRenderableObject &inRenderableObject,
                                          TPickResultArray &outIntersectionResultList);

    // shader implementations, RHI, implemented in qssgrendererimplshaders_rhi.cpp
public:
    QSSGRef<QSSGRhiShaderPipeline> getRhiCubemapShadowBlurXShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiCubemapShadowBlurYShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiOrthographicShadowBlurXShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiOrthographicShadowBlurYShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiSsaoShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiSkyBoxShader(QSSGRenderLayer::TonemapMode tonemapMode, bool isRGBE);
    QSSGRef<QSSGRhiShaderPipeline> getRhiSupersampleResolveShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiProgressiveAAShader();
    QSSGRef<QSSGRhiShaderPipeline> getRhiTexturedQuadShader();

private:
    friend class QSSGRenderContextInterface;
    void releaseResources();

    QSSGRef<QSSGRhiShaderPipeline> getBuiltinRhiShader(const QByteArray &name,
                                                       QSSGRef<QSSGRhiShaderPipeline> &storage);
    // Skybox shader state
    QSSGRenderLayer::TonemapMode m_skyboxTonemapMode = QSSGRenderLayer::TonemapMode::None;
    bool m_isSkyboxRGBE = false;
};
QT_END_NAMESPACE

#endif
