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

#include <QtQuick3DUtils/private/qtquick3dutilsglobal_p.h>
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

    virtual void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) = 0;
    virtual void renderPass(QSSGRenderer &renderer) = 0;
    virtual Type passType() const = 0;
    virtual void release() = 0;

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
    void release() final;

    std::shared_ptr<QSSGRenderShadowMap> shadowMapManager;
    QSSGRenderableObjectList shadowPassObjects;
    QSSGShaderLightList globalLights;
    QSSGRenderCamera *camera = nullptr;
    QSSGRhiGraphicsPipelineState ps;
    QSSGBoxPoints castingObjectsBox;
    QSSGBoxPoints receivingObjectsBox;
    bool enabled = false;
};

class ReflectionMapPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Standalone; }
    void release() final;

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
    void release() final;

    QSSGRenderableObjectList renderedDepthWriteObjects;
    QSSGRenderableObjectList renderedOpaqueDepthPrepassObjects;
    QSSGRhiGraphicsPipelineState ps;
    bool active = false;
};

class SSAOMapPass : public QSSGRenderPass
{
public:
    struct AmbientOcclusion
    {
        float aoStrength = 0.0f;
        float aoDistance = 5.0f;
        float aoSoftness = 50.0f;
        float aoBias = 0.0f;
        qint32 aoSamplerate = 2;
        bool aoDither = false;
    } ambientOcclusion;

    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Standalone; }
    void release() final;

    const QSSGRhiRenderableTexture *rhiDepthTexture = nullptr;
    const QSSGRenderCamera *camera = nullptr;
    AmbientOcclusion ao;
    QSSGRhiGraphicsPipelineState ps;
    QSSGRhiRenderableTexture *rhiAoTexture = nullptr;
    QSSGRhiShaderPipelinePtr ssaoShaderPipeline;
};

class Q_QUICK3DRUNTIMERENDER_PRIVATE_EXPORT DepthMapPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Standalone; }
    void release() final;

    QSSGRenderableObjectList sortedOpaqueObjects;
    QSSGRenderableObjectList sortedTransparentObjects;
    QSSGRhiGraphicsPipelineState ps;
    QSSGRhiRenderableTexture *rhiDepthTexture = nullptr;
};

class ScreenMapPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Standalone; }
    void release() final;

    QSSGRhiRenderableTexture *rhiScreenTexture = nullptr;
    QSSGRenderPass *skyboxPass = nullptr;
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
    void release() final;

    QSSGRenderableObjectList sortedScreenTextureObjects;
    const QSSGRhiRenderableTexture *rhiScreenTexture = nullptr;
    QSSGRhiGraphicsPipelineState ps {};
};

class OpaquePass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Main; }
    void release() final;

    QSSGRenderableObjectList sortedOpaqueObjects;
    QSSGRhiGraphicsPipelineState ps;
    QSSGShaderFeatures shaderFeatures;
};

class TransparentPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Main; }
    void release() final;

    QSSGRenderableObjectList sortedTransparentObjects;
    QSSGRhiGraphicsPipelineState ps;
    QSSGShaderFeatures shaderFeatures;
};

class SkyboxPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Main; }
    void release() final;

    QSSGRenderLayer *layer = nullptr;
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
    void release() final;

    QSSGRenderLayer *layer = nullptr;
    QSSGRhiGraphicsPipelineState ps;
    bool skipTonemapping = false;
};

class Item2DPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Main; }
    void release() final;

    QList<QSSGRenderItem2D *> item2Ds;
    QSSGRhiGraphicsPipelineState ps {};
};

class InfiniteGridPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Main; }
    void release() final;

    QSSGRhiGraphicsPipelineState ps {};
    QSSGRenderLayer *layer = nullptr;
};

class DebugDrawPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Main; }
    void release() final;

    QSSGRhiGraphicsPipelineState ps;
};

class QSSGRenderExtension;

class UserPass : public QSSGRenderPass
{
public:
    void renderPrep(QSSGRenderer &renderer, QSSGLayerRenderData &data) final;
    void renderPass(QSSGRenderer &renderer) final;
    Type passType() const final { return Type::Extension; }
    void release() final;

    bool hasData() const { return extensions.size() != 0; }

    QList<QSSGRenderExtension *> extensions;
};

QT_END_NAMESPACE

#endif // QSSGRENDERPASS_H
