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
#include <QtQuick3DRuntimeRender/private/qssglightmapper_p.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE
class QSSGRenderContextInterface;
struct QSSGRenderPresentation;
struct QSSGRenderEffect;
struct QSSGRenderImage;
class QSSGLayerRenderData;
struct QSSGRenderResourceLoader;

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
        Filmic
    };

    enum class LayerFlag
    {
        EnableDepthTest = 0x1,
        EnableDepthPrePass = 0x2, ///< True when we render a depth pass before
        RenderToTarget = 0x3 ///< Does this layer render to the normal render target,
    };
    Q_DECLARE_FLAGS(LayerFlags, LayerFlag)

    // First effect in a list of effects.
    QSSGRenderEffect *firstEffect;
    QSSGLayerRenderData *renderData = nullptr;

    // If a layer has a valid texture path (one that resolves to either a
    // an on-disk image or a offscreen renderer), then it does not render its
    // own source path.  Instead, it renders the offscreen renderer.  Used in this manner,
    // offscreen renderer's also have the option (if they support it) to render directly to the
    // render target given a specific viewport (that is also scissored if necessary).
    QString texturePath;

    QSSGRenderLayer::AAMode antialiasingMode;
    QSSGRenderLayer::AAQuality antialiasingQuality;

    QSSGRenderLayer::Background background;
    QVector3D clearColor;

    // TODO: pack
    HorizontalField horizontalFieldValues;
    float m_left;
    UnitType leftUnits;
    float m_width;
    UnitType widthUnits;
    float m_right;
    UnitType rightUnits;

    VerticalField verticalFieldValues;
    float m_top;
    UnitType topUnits;
    float m_height;
    UnitType heightUnits;
    float m_bottom;
    UnitType bottomUnits;

    // Ambient occlusion
    float aoStrength;
    float aoDistance;
    float aoSoftness;
    float aoBias;
    qint32 aoSamplerate;
    bool aoDither;

    // IBL
    QSSGRenderImage *lightProbe;
    float probeExposure;
    float probeHorizon;
    QMatrix3x3 probeOrientation;
    QVector3D probeOrientationAngles;

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

    // The camera explicitly set on the view by the user.
    QSSGRenderCamera *explicitCamera;
    // The camera used for rendering (explicitCamera, nullptr or first usable camera).
    QSSGRenderCamera *renderedCamera;

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

    // Lightmapper config
    QSSGLightmapperOptions lmOptions;

    QVector<QSSGRenderGraphObject *> resourceLoaders;

    QSSGRenderLayer();
    ~QSSGRenderLayer();

    void setProbeOrientation(const QVector3D &angles);

    void addEffect(QSSGRenderEffect &inEffect);

    QSSGRenderEffect *getLastEffect();

    QSSGRenderNode *importSceneNode = nullptr;

    // Special function(s) for importScene
    void setImportScene(QSSGRenderNode &rootNode);
    void removeImportScene(QSSGRenderNode &rootNode);

};
QT_END_NAMESPACE

#endif
