// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGRENDERPASS_H
#define QSSGRENDERPASS_H

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

using QSSGBoxPoints = std::array<QVector3D, 8>;

class QSSGRenderPass
{
public:
    // Input:

    virtual void renderPrep(const QSSGRef<QSSGRenderer> &renderer, QSSGLayerRenderData &data) = 0;
    virtual void renderPass(const QSSGRef<QSSGRenderer> &renderer) = 0;
    virtual void release() = 0;

    // Output:

    // Flags: Debug markers(?)

    // Dependency
};

class ShadowMapPass : public QSSGRenderPass
{
public:
    void renderPrep(const QSSGRef<QSSGRenderer> &renderer, QSSGLayerRenderData &data) final;
    void renderPass(const QSSGRef<QSSGRenderer> &renderer) final;
    void release() final;

    QSSGRef<QSSGRenderShadowMap> shadowMapManager;
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
    void renderPrep(const QSSGRef<QSSGRenderer> &renderer, QSSGLayerRenderData &data) final;
    void renderPass(const QSSGRef<QSSGRenderer> &renderer) final;
    void release() final;

    QSSGRef<QSSGRenderReflectionMap> reflectionMapManager;
    QVector<QSSGRenderReflectionProbe *> reflectionProbes;
    QSSGRenderableObjectList reflectionPassObjects;
    QSSGRhiGraphicsPipelineState ps;
};

class ZPrePassPass : public QSSGRenderPass
{
public:
    enum class State
    {
        Disabled,
        Active,
        Forced
    };

    // Note: prep phase, there's also the render phase... Should both be specified here?
    void renderPrep(const QSSGRef<QSSGRenderer> &renderer, QSSGLayerRenderData &data) final;
    void renderPass(const QSSGRef<QSSGRenderer> &renderer) final;
    void release() final;

    QSSGRenderableObjectList renderedDepthWriteObjects;
    QSSGRenderableObjectList renderedOpaqueDepthPrepassObjects;
    QSSGRhiGraphicsPipelineState ps;
    State state { State::Disabled };
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

    void renderPrep(const QSSGRef<QSSGRenderer> &renderer, QSSGLayerRenderData &data) final;
    void renderPass(const QSSGRef<QSSGRenderer> &renderer) final;
    void release() final;

    const QSSGRhiRenderableTexture *rhiDepthTexture = nullptr;
    const QSSGRenderCamera *camera = nullptr;
    AmbientOcclusion ao;
    QSSGRhiGraphicsPipelineState ps;
    QSSGRhiRenderableTexture rhiAoTexture;
    QSSGRef<QSSGRhiShaderPipeline> ssaoShaderPipeline;
};

class DepthMapPass : public QSSGRenderPass
{
public:
    void renderPrep(const QSSGRef<QSSGRenderer> &renderer, QSSGLayerRenderData &data) final;
    void renderPass(const QSSGRef<QSSGRenderer> &renderer) final;
    void release() final;

    QSSGRenderableObjectList sortedOpaqueObjects;
    QSSGRenderableObjectList sortedTransparentObjects;
    QSSGRhiGraphicsPipelineState ps;
    QSSGRhiRenderableTexture rhiDepthTexture;
};

class ScreenMapPass : public QSSGRenderPass
{
public:
    void renderPrep(const QSSGRef<QSSGRenderer> &renderer, QSSGLayerRenderData &data) final;
    void renderPass(const QSSGRef<QSSGRenderer> &renderer) final;
    void release() final;

    QSSGRhiRenderableTexture rhiScreenTexture;
    QSSGShaderFeatures shaderFeatures;
    QSSGRenderableObjectList sortedOpaqueObjects;
    QSSGRhiGraphicsPipelineState ps;
    QColor clearColor{Qt::transparent};
    bool wantsMips = false;
};

class MainPass : public QSSGRenderPass
{
public:
    void renderPrep(const QSSGRef<QSSGRenderer> &renderer, QSSGLayerRenderData &data) final;
    void renderPass(const QSSGRef<QSSGRenderer> &renderer) final;
    void release() final;

    QSSGRenderableObjectList sortedOpaqueObjects;
    QSSGRenderableObjectList sortedTransparentObjects;
    QSSGRenderableObjectList sortedScreenTextureObjects;
    QVector<QSSGRenderItem2D *> item2Ds;
    QSSGShaderFeatures shaderFeatures;
    QSSGRhiGraphicsPipelineState ps;
};

QT_END_NAMESPACE

#endif // QSSGRENDERPASS_H
