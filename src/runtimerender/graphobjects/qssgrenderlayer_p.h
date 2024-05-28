// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSG_RENDER_LAYER_H
#define QSSG_RENDER_LAYER_H

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

#include <QtQuick3DRuntimeRender/private/qssgrendernode_p.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qlist.h>
#include <ssg/qssglightmapper.h>

QT_BEGIN_NAMESPACE
class QSSGRenderContextInterface;
struct QSSGRenderPresentation;
struct QSSGRenderEffect;
struct QSSGRenderImage;
class QSSGLayerRenderData;
struct QSSGRenderResourceLoader;

class QQuick3DObject;
class QSSGRenderExtension;

class QRhiShaderResourceBindings;

// A layer is a special node.  It *always* presents its global transform
// to children as the identity.  It also can optionally have a width or height
// different than the overlying context.  You can think of layers as the transformation
// between a 3d scene graph and a 2D texture.
struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderLayer : public QSSGRenderNode
{
    enum class AAMode : quint8
    {
        NoAA = 0,
        SSAA,
        MSAA,
        ProgressiveAA
    };

    enum class AAQuality : quint8
    {
        Normal = 2,
        High = 4,
        VeryHigh = 8
    };

    enum class HorizontalField : quint8
    {
        LeftWidth = 0,
        LeftRight,
        WidthRight
    };

    enum class VerticalField : quint8
    {
        TopHeight = 0,
        TopBottom,
        HeightBottom
    };

    enum class UnitType : quint8
    {
        Percent = 0,
        Pixels
    };

    enum class Background : quint8
    {
        Transparent = 0,
        Unspecified,
        Color,
        SkyBox,
        SkyBoxCubeMap
    };

    enum class TonemapMode : quint8
    {
        None = 0, // Bypass mode
        Linear,
        Aces,
        HejlDawson,
        Filmic,
        Custom
    };
    static size_t constexpr TonemapModeCount = 6;

    enum class LayerFlag
    {
        EnableDepthTest = 0x1,
        EnableDepthPrePass = 0x2, ///< True when we render a depth pass before
        RenderToTarget = 0x3 ///< Does this layer render to the normal render target,
    };
    Q_DECLARE_FLAGS(LayerFlags, LayerFlag)

    enum class MaterialDebugMode : quint8
    {
        None = 0, // Bypass
        BaseColor = 1,
        Roughness,
        Metalness,
        Diffuse,
        Specular,
        ShadowOcclusion,
        Emission,
        AmbientOcclusion,
        Normal,
        Tangent,
        Binormal,
        F0
    };

    // First effect in a list of effects.
    QSSGRenderEffect *firstEffect;
    QSSGLayerRenderData *renderData = nullptr;
    enum class RenderExtensionStage { Underlay, Overlay, Count };
    QList<QSSGRenderExtension *> renderExtensions[size_t(RenderExtensionStage::Count)];

    QSSGRenderLayer::AAMode antialiasingMode;
    QSSGRenderLayer::AAQuality antialiasingQuality;

    QSSGRenderLayer::Background background;
    QVector3D clearColor;

    // Ambient occlusion
    float aoStrength = 0.0f;
    float aoDistance = 5.0f;
    float aoSoftness = 50.0f;
    float aoBias = 0.0f;
    qint32 aoSamplerate = 2;
    bool aoDither = false;
    bool aoEnabled = false;

    constexpr bool ssaoEnabled() const { return aoEnabled && (aoStrength > 0.0f && aoDistance > 0.0f); }

    // IBL
    QSSGRenderImage *lightProbe { nullptr };
    struct LightProbeSettings {
        float probeExposure { 1.0f };
        float probeHorizon { -1.0f };
        QMatrix3x3 probeOrientation;
        QVector3D probeOrientationAngles;
    } lightProbeSettings;

    QSSGRenderImage *skyBoxCubeMap = nullptr;

    bool temporalAAEnabled;
    float temporalAAStrength;
    bool ssaaEnabled;
    float ssaaMultiplier;
    bool specularAAEnabled;

    //TODO: move render state somewhere more suitable
    bool temporalAAIsActive;
    bool progressiveAAIsActive;
    uint tempAAPassIndex;
    uint progAAPassIndex;

    // The camera explicitly set on the view by the user. (backend node can be null)
    QVarLengthArray<QSSGRenderCamera *, 2> explicitCameras;
    // The camera used for rendering, multiple ones with multiview.
    QVarLengthArray<QSSGRenderCamera *, 2> renderedCameras;

    // Tonemapping
    TonemapMode tonemapMode;

    LayerFlags layerFlags { LayerFlag::RenderToTarget,
                            LayerFlag::EnableDepthTest,
                            LayerFlag::EnableDepthPrePass };

    // references to objects owned by the QSSGRhiContext
    QRhiShaderResourceBindings *skyBoxSrb = nullptr;
    QVarLengthArray<QRhiShaderResourceBindings *, 4> item2DSrbs;
    bool skyBoxIsRgbe8 = false;

    // Skybox
    float skyboxBlurAmount = 0.0f;

    // Grid
    bool gridEnabled = false;
    float gridScale = 1.0f;
    quint32 gridFlags = 0;
    QRhiShaderResourceBindings *gridSrb = nullptr;

    // Lightmapper config
    QSSGLightmapperOptions lmOptions;

    // Scissor
    QRect scissorRect;

    // Fog
    struct FogOptions {
        bool enabled = false;
        QVector3D color = QVector3D(0.5f, 0.6f, 0.7f);
        float density = 1.0f;
        bool depthEnabled = false;
        float depthBegin = 10.0f;
        float depthEnd = 1000.0f;
        float depthCurve = 1.0f;
        bool heightEnabled = false;
        float heightMin = 10.0f;
        float heightMax = 0.0f;
        float heightCurve = 1.0f;
        bool transmitEnabled = false;
        float transmitCurve = 1.0f;
    } fog;

    QVector<QSSGRenderGraphObject *> resourceLoaders;

    MaterialDebugMode debugMode = MaterialDebugMode::None;

    bool wireframeMode = false;
    bool drawDirectionalLightShadowBoxes = false;
    bool drawShadowCastingBounds = false;
    bool drawShadowReceivingBounds = false;
    bool drawCascades = false;
    bool drawSceneCascadeIntersection = false;
    bool disableShadowCameraUpdate = false;

    QSSGRenderLayer();
    ~QSSGRenderLayer();

    void setProbeOrientation(const QVector3D &angles);

    void addEffect(QSSGRenderEffect &inEffect);
    bool hasEffect(QSSGRenderEffect *inEffect) const;

    QSSGRenderNode *importSceneNode = nullptr;

    // Special function(s) for importScene
    void setImportScene(QSSGRenderNode &rootNode);
    void removeImportScene(QSSGRenderNode &rootNode);

};
QT_END_NAMESPACE

#endif
