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
        PreMain,
        Main
    };
    // Input:

    virtual void renderPrep(const QSSGRef<QSSGRenderer> &renderer, QSSGLayerRenderData &data) = 0;
    virtual void renderPass(const QSSGRef<QSSGRenderer> &renderer) = 0;
    virtual Type passType() const = 0;
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
    Type passType() const final { return Type::PreMain; }
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
    Type passType() const final { return Type::PreMain; }
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
    Type passType() const final { return Type::Main; }
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
    Type passType() const final { return Type::PreMain; }
    void release() final;

    const QSSGRhiRenderableTexture *rhiDepthTexture = nullptr;
    const QSSGRenderCamera *camera = nullptr;
    AmbientOcclusion ao;
    QSSGRhiGraphicsPipelineState ps;
    QSSGRhiRenderableTexture rhiAoTexture;
    QSSGRef<QSSGRhiShaderPipeline> ssaoShaderPipeline;
};

class Q_QUICK3DRUNTIMERENDER_PRIVATE_EXPORT DepthMapPass : public QSSGRenderPass
{
public:
    void renderPrep(const QSSGRef<QSSGRenderer> &renderer, QSSGLayerRenderData &data) final;
    void renderPass(const QSSGRef<QSSGRenderer> &renderer) final;
    Type passType() const final { return Type::PreMain; }
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
    Type passType() const final { return Type::PreMain; }
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
    Type passType() const final { return Type::Main; }
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
