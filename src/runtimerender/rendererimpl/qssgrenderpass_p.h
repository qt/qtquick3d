// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGRENDERPASS_H
#define QSSGRENDERPASS_H

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

#include <QtCore/qglobal.h>

#include <ssg/qssgrenderhelpers.h>
#include <QtQuick3DUtils/private/qssgaosettings_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicontext_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderableobjects_p.h>

QT_BEGIN_NAMESPACE

class QSSGRenderer;
class QSSGRenderShadowMap;
class QSSGRenderReflectionMap;
class QSSGLayerRenderData;
struct QSSGRenderCamera;
struct QSSGRenderItem2D;

class QSSGRenderPass
{
public:
    enum class Type
    {
        Standalone,
        Main,
        Extension
    };
    // Input:

    virtual ~QSSGRenderPass();
    virtual void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) = 0;
    virtual void renderPass(QSSGRenderer &renderer) = 0;
    virtual Type passType() const = 0;
    virtual void resetForFrame() = 0;

    // Output:

    // Flags: Debug markers(?)

    // Dependency
};

class ShadowMapPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Standalone; }
    void resetForFrame() final;

    std::shared_ptr<QSSGRenderShadowMap> shadowMapManager;
    QSSGRenderableObjectList shadowPassObjects;
    QSSGShaderLightList globalLights;
    QSSGRenderCamera *camera = nullptr;
    std::unique_ptr<QSSGRenderCamera> debugCamera;
    QSSGRhiGraphicsPipelineState ps;
    QSSGBounds3 castingObjectsBox;
    QSSGBounds3 receivingObjectsBox;
    bool enabled = false;
};

class ReflectionMapPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Standalone; }
    void resetForFrame() final;

    std::shared_ptr<QSSGRenderReflectionMap> reflectionMapManager;
    QList<QSSGRenderReflectionProbe *> reflectionProbes;
    QSSGRenderableObjectList reflectionPassObjects;
    QSSGRhiGraphicsPipelineState ps;
};

class ZPrePassPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Main; }
    void resetForFrame() final;

    QSSGRenderableObjectList renderedDepthWriteObjects;
    QSSGRenderableObjectList renderedOpaqueDepthPrepassObjects;
    QSSGRhiGraphicsPipelineState ps;
    bool active = false;
};

class SSAOMapPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Standalone; }
    void resetForFrame() final;

    const QSSGRhiRenderableTexture *rhiDepthTexture = nullptr;
    const QSSGRenderCamera *camera = nullptr;
    QSSGAmbientOcclusionSettings aoSettings;
    QSSGRhiGraphicsPipelineState ps;
    QSSGRhiRenderableTexture *rhiAoTexture = nullptr;
    QSSGRhiShaderPipelinePtr ssaoShaderPipeline;
};

class Q_QUICK3DRUNTIMERENDER_EXPORT DepthMapPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Standalone; }
    void resetForFrame() final;

    QSSGRenderableObjectList sortedOpaqueObjects;
    QSSGRenderableObjectList sortedTransparentObjects;
    QSSGRhiGraphicsPipelineState ps;
    QSSGRhiRenderableTexture *rhiDepthTexture = nullptr;
};

class SkyboxPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Main; }
    void resetForFrame() final;

    QSSGRenderLayer *layer = nullptr;
    QRhiRenderPassDescriptor *rpDesc = nullptr;
    QSSGRhiGraphicsPipelineState ps;
    bool skipTonemapping = false;
    bool skipPrep = false;
};

class SkyboxCubeMapPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Main; }
    void resetForFrame() final;

    QSSGRhiShaderPipelinePtr skyBoxCubeShader;
    QSSGRenderLayer *layer = nullptr;
    QRhiRenderPassDescriptor *rpDesc = nullptr;
    QSSGRhiGraphicsPipelineState ps;
    bool skipTonemapping = false;
};

class ScreenMapPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Standalone; }
    void resetForFrame() final;

    QSSGRhiRenderableTexture *rhiScreenTexture = nullptr;
    std::optional<SkyboxPass> skyboxPass;
    std::optional<SkyboxCubeMapPass> skyboxCubeMapPass;
    QSSGShaderFeatures shaderFeatures;
    QSSGRenderableObjectList sortedOpaqueObjects;
    QSSGRhiGraphicsPipelineState ps;
    QColor clearColor{Qt::transparent};
    bool wantsMips = false;
};

class ScreenReflectionPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Main; }
    void resetForFrame() final;

    QSSGRenderableObjectList sortedScreenTextureObjects;
    const QSSGRhiRenderableTexture *rhiScreenTexture = nullptr;
    QSSGRhiGraphicsPipelineState ps {};
};

class OpaquePass : public QSSGRenderPass
{
public:
    static void prep(const QSSGRenderContextInterface &ctx,
                     QSSGLayerRenderData &data,
                     QSSGPassKey passKey,
                     QSSGRhiGraphicsPipelineState &ps,
                     QSSGShaderFeatures shaderFeatures,
                     QRhiRenderPassDescriptor *rpDesc,
                     const QSSGRenderableObjectList &sortedOpaqueObjects);

    static void render(const QSSGRenderContextInterface &ctx,
                       const QSSGRhiGraphicsPipelineState &ps,
                       const QSSGRenderableObjectList &sortedOpaqueObjects);

    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Main; }
    void resetForFrame() final;

    QSSGRenderableObjectList sortedOpaqueObjects;
    QSSGRhiGraphicsPipelineState ps;
    QSSGShaderFeatures shaderFeatures;
};

struct QSSGCameraRenderData;

class TransparentPass : public QSSGRenderPass
{
public:
    static void prep(const QSSGRenderContextInterface &ctx,
                     QSSGLayerRenderData &data,
                     QSSGPassKey passKey,
                     QSSGRhiGraphicsPipelineState &ps,
                     QSSGShaderFeatures shaderFeatures,
                     QRhiRenderPassDescriptor *rpDesc,
                     const QSSGRenderableObjectList &sortedTransparentObjects);

    static void render(const QSSGRenderContextInterface &ctx,
                       const QSSGRhiGraphicsPipelineState &ps,
                       const QSSGRenderableObjectList &sortedTransparentObjects);


    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Main; }
    void resetForFrame() final;

    QSSGRenderableObjectList sortedTransparentObjects;
    QSSGRhiGraphicsPipelineState ps;
    QSSGShaderFeatures shaderFeatures;
};

class Item2DPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Main; }
    void resetForFrame() final;

    QList<QSSGRenderItem2D *> item2Ds;
    QSSGRhiGraphicsPipelineState ps {};
};

class InfiniteGridPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Main; }
    void resetForFrame() final;

    QSSGRhiShaderPipelinePtr gridShader;
    QSSGRhiGraphicsPipelineState ps {};
    QSSGRenderLayer *layer = nullptr;
};

class DebugDrawPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Main; }
    void resetForFrame() final;

    QSSGRhiShaderPipelinePtr debugObjectShader;
    QSSGRhiGraphicsPipelineState ps;
};

class QSSGRenderExtension;

class UserPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Extension; }
    void resetForFrame() final;

    bool hasData() const { return extensions.size() != 0; }

    QList<QSSGRenderExtension *> extensions;
};

QT_END_NAMESPACE

#endif // QSSGRENDERPASS_H
