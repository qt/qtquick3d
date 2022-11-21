// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers.impl

SceneEffectEnvironment {
    id: sceneEnvironment

    // Depth of Field Effect
    property alias depthOfFieldEnabled: dofBlurEffect.enabled
    property alias depthOfFieldFocusDistance: dofBlurEffect.focusDistance
    property alias depthOfFieldFocusRange: dofBlurEffect.focusRange
    property alias depthOfFieldBlurAmount: dofBlurEffect.blurAmount

    // Tonemapper
    property alias exposure: sceneEffect.exposure
    property alias whitePoint: sceneEffect.white
    property alias ditheringEnabled: sceneEffect.ditheringEnabled
    property alias sharpenIntensity: sceneEffect.sharpenIntensity

    // FXAA
    property alias fxaaEnabled: sceneEffect.applyFXAA

    // Adjustments
    property alias colorAdjustmentsEnabled: sceneEffect.colorAdjustmentsEnabled
    property alias adjustmentBrightness: sceneEffect.bcsAdjustments.x
    property alias adjustmentContrast: sceneEffect.bcsAdjustments.y
    property alias adjustmentSaturation: sceneEffect.bcsAdjustments.z

    // Color Grading Effect
    property alias lutEnabled: sceneEffect.enableLut
    property alias lutSize: sceneEffect.lutSize
    property alias lutFilterAlpha: sceneEffect.lutFilterAlpha
    property alias lutTextureSource: sceneEffect.lutTextureSource

    // Glow Effect
    property alias glowEnabled: sceneEffect.isGlowEnabled
    property alias glowQualityHigh: sceneEffect.glowQualityHigh
    property alias glowUseBicubicUpscale: sceneEffect.glowUseBicubicUpscale
    property alias glowStrength: sceneEffect.glowStrength
    property alias glowIntensity: sceneEffect.glowIntensity
    property alias glowBloom: sceneEffect.glowBloom
    property alias glowBlendMode: sceneEffect.glowBlendMode
    property alias glowHDRLuminanceCap: sceneEffect.glowHDRLuminanceCap
    property alias glowHDRScale: sceneEffect.glowHDRScale
    property alias glowHDRThreshold: sceneEffect.glowHDRThreshold
    property alias glowLevel1: sceneEffect.glowLevel1
    property alias glowLevel2: sceneEffect.glowLevel2
    property alias glowLevel3: sceneEffect.glowLevel3
    property alias glowLevel4: sceneEffect.glowLevel4
    property alias glowLevel5: sceneEffect.glowLevel5
    property alias glowLevel6: sceneEffect.glowLevel6
    property alias glowLevel7: sceneEffect.glowLevel7

    // Vignette
    property alias vignetteEnabled: sceneEffect.vignetteEnabled
    property alias vignetteStrength: sceneEffect.vignetteStrength
    property alias vignetteColor: sceneEffect.vignetteColor
    property alias vignetteRadius: sceneEffect.vignetteRadius

    // Lens Flare
    property alias lensFlareEnabled: sceneEffect.lensFlareEnabled
    property alias lensFlareBloomScale: sceneEffect.lensFlareBloomScale
    property alias lensFlareBloomBias: sceneEffect.lensFlareBloomBias
    property alias lensFlareGhostDispersal: sceneEffect.lensFlareGhostDispersal
    property alias lensFlareGhostCount: sceneEffect.lensFlareGhostCount
    property alias lensFlareHaloWidth: sceneEffect.lensFlareHaloWidth
    property alias lensFlareStretchToAspect: sceneEffect.lensFlareStretchToAspect
    property alias lensFlareDistortion: sceneEffect.lensFlareDistortion
    property alias lensFlareBlurAmount: sceneEffect.lensFlareBlurAmount
    property alias lensFlareApplyDirtTexture: sceneEffect.lensFlareApplyDirtTexture
    property alias lensFlareApplyStarburstTexture: sceneEffect.lensFlareApplyStarburstTexture
    property alias lensFlareCameraDirection: sceneEffect.lensFlareCameraDirection
    property alias lensFlareLensColorTexture: sceneEffect.lensColorTexture
    property alias lensFlareLensDirtTexture: sceneEffect.lensDirtTexture
    property alias lensFlareLensStarburstTexture: sceneEffect.starburstTexture

    DepthOfFieldBlur {
        id: dofBlurEffect
        environment: sceneEnvironment
    }

    SceneEffect {
        id: sceneEffect
        environment: sceneEnvironment
        tonemapMode: sceneEnvironment.tonemapMode
    }
}
