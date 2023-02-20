// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D
import QtQuick3D.Helpers

Rectangle {
    id: toolPage

    required property Camera camera
    required property var sceneEnvironment
    required property Texture lutTexture

    ScrollView {
        anchors.fill: parent
        anchors.margins: 10
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ColumnLayout {
            width: parent.width
            // AA
            SectionLayout {
                id: antialiasingSection
                title: "Antialiasing"
                RowLayout {
                    Label {
                        text: "AA Mode"
                        Layout.fillWidth: true
                    }

                    ComboBox {
                        id: antialiasingModeComboBox
                        textRole: "text"
                        valueRole: "value"
                        implicitContentWidthPolicy: ComboBox.WidestText
                        onActivated: sceneEnvironment.antialiasingMode = currentValue

                        Component.onCompleted: antialiasingModeComboBox.currentIndex = antialiasingModeComboBox.indexOfValue(sceneEnvironment.antialiasingMode)

                        model: [
                            { value: SceneEnvironment.NoAA, text: "No Antialiasing"},
                            { value: SceneEnvironment.SSAA, text: "Supersample AA"},
                            { value: SceneEnvironment.MSAA, text: "Multisample AA"},
                            { value: SceneEnvironment.ProgressiveAA, text: "Progressive AA"}
                        ]
                    }
                }
                RowLayout {
                    visible: sceneEnvironment.antialiasingMode !== SceneEnvironment.NoAA
                    Label {
                        text: "AA Quality"
                        Layout.fillWidth: true
                    }

                    ComboBox {
                        id: antialiasingQualityComboBox
                        textRole: "text"
                        valueRole: "value"
                        implicitContentWidthPolicy: ComboBox.WidestText
                        onActivated: sceneEnvironment.antialiasingQuality = currentValue
                        Component.onCompleted: antialiasingQualityComboBox.currentIndex = antialiasingQualityComboBox.indexOfValue(sceneEnvironment.antialiasingQuality)
                        model: [
                            { value: SceneEnvironment.Medium, text: "Medium"},
                            { value: SceneEnvironment.High, text: "High"},
                            { value: SceneEnvironment.VeryHigh, text: "VeryHigh"}
                        ]
                    }
                }
                CheckBox {
                    text: "Enable FXAA"
                    checked: sceneEnvironment.fxaaEnabled
                    onCheckedChanged: {
                        sceneEnvironment.fxaaEnabled = checked
                    }
                }

                CheckBox {
                    text: "Enable Temporal AA"
                    checked: sceneEnvironment.temporalAAEnabled
                    onCheckedChanged: {
                        sceneEnvironment.temporalAAEnabled = checked
                    }
                }

                RowLayout {
                    visible: sceneEnvironment.temporalAAEnabled
                    Label {
                        text: "Temporal AA Strength (" + sceneEnvironment.temporalAAStrength.toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 1.0
                        value: sceneEnvironment.temporalAAStrength
                        onValueChanged:
                            sceneEnvironment.temporalAAStrength = value
                    }
                }

                CheckBox {
                    text: "Enable Specular AA"
                    checked: sceneEnvironment.specularAAEnabled
                    onCheckedChanged: {
                        sceneEnvironment.specularAAEnabled = checked
                    }
                }
            }

            // AO
            SectionLayout {
                id: aoSection
                title: "Ambient Occlusion"

                CheckBox {
                    text: "Enable SSAO"
                    checked: sceneEnvironment.aoEnabled
                    onCheckedChanged: {
                        sceneEnvironment.aoEnabled = checked
                    }
                }
                RowLayout {
                    Label {
                        text: "Strength (" + (sceneEnvironment.aoStrength).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 100.0
                        value: sceneEnvironment.aoStrength
                        onValueChanged: {
                            sceneEnvironment.aoStrength = value
                        }
                    }
                }
                RowLayout {
                    Label {
                        text: "Softness (" + (sceneEnvironment.aoSoftness).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 50.0
                        value: sceneEnvironment.aoSoftness
                        onValueChanged:
                            sceneEnvironment.aoSoftness = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Distance (" + (sceneEnvironment.aoDistance).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 5.0
                        value: sceneEnvironment.aoDistance
                        onValueChanged:
                            sceneEnvironment.aoDistance = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Sample Rate"
                        Layout.fillWidth: true
                    }

                    ComboBox {
                        id: aoSampleRateComboBox
                        textRole: "text"
                        valueRole: "value"
                        implicitContentWidthPolicy: ComboBox.WidestText
                        onActivated: sceneEnvironment.aoSampleRate = currentValue

                        Component.onCompleted: aoSampleRateComboBox.currentIndex = aoSampleRateComboBox.indexOfValue(sceneEnvironment.aoSampleRate)

                        model: [
                            { value: 2, text: "2"},
                            { value: 3, text: "3"},
                            { value: 4, text: "4"}
                        ]
                    }
                }
                RowLayout {
                    Label {
                        text: "Bias (" + (sceneEnvironment.aoBias).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: -1.0
                        to: 1.0
                        stepSize: 0.01
                        value: sceneEnvironment.aoBias
                        onValueChanged:
                            sceneEnvironment.aoBias = value
                    }
                }
                CheckBox {
                    text: "Enable AO Dither"
                    checked: sceneEnvironment.aoDither
                    onCheckedChanged: {
                        sceneEnvironment.aoDither = checked
                    }
                }
            }

            // DOF
            SectionLayout {
                id: dofSection
                title: "Depth of Field"

                CheckBox {
                    text: "Enabled"
                    checked: sceneEnvironment.depthOfFieldEnabled
                    onCheckedChanged: {
                        sceneEnvironment.depthOfFieldEnabled = checked
                    }
                }

                Label {
                    text: "Focus Distance (" + (sceneEnvironment.depthOfFieldFocusDistance).toFixed(2) + ")"
                    Layout.fillWidth: true
                }

                Label {
                    text: "Focus Range (" + (sceneEnvironment.depthOfFieldFocusRange).toFixed(2) +")"
                    Layout.fillWidth: true
                }

                Label {
                    text: "Focus Near (" + dofFocusSlider.first.value.toFixed(2) + ")"
                    Layout.fillWidth: true
                }
                Label {
                    text: "Focus Far (" + dofFocusSlider.second.value.toFixed(2) + ")"
                    Layout.fillWidth: true
                }

                RangeSlider {
                    id: dofFocusSlider
                    from: camera.clipNear
                    to: camera.clipFar
                    Layout.fillWidth: true
                    Component.onCompleted: {
                        first.value = sceneEnvironment.depthOfFieldFocusDistance - sceneEnvironment.depthOfFieldFocusRange * 0.5
                        second.value = sceneEnvironment.depthOfFieldFocusDistance + sceneEnvironment.depthOfFieldFocusRange * 0.5
                    }
                    first.onMoved: {
                        sceneEnvironment.depthOfFieldFocusRange = second.value - first.value
                        sceneEnvironment.depthOfFieldFocusDistance = first.value + sceneEnvironment.depthOfFieldFocusRange * 0.5
                    }
                    second.onMoved: {
                        sceneEnvironment.depthOfFieldFocusRange = second.value - first.value
                        sceneEnvironment.depthOfFieldFocusDistance = first.value + sceneEnvironment.depthOfFieldFocusRange * 0.5
                    }
                }

                RowLayout {
                    Label {
                        text: "Blur Amount (" + (sceneEnvironment.depthOfFieldBlurAmount).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 25.0
                        value: sceneEnvironment.depthOfFieldBlurAmount
                        onValueChanged:
                            sceneEnvironment.depthOfFieldBlurAmount = value
                    }
                }

            }

            // FOG
            SectionLayout {
                id: fogSection
                title: "Fog"

                CheckBox {
                    text: "Enabled"
                    checked: sceneEnvironment.fog.enabled
                    onCheckedChanged: {
                        sceneEnvironment.fog.enabled = checked
                    }
                }
                RowLayout {
                    Label {
                        text: "Density (" + sceneEnvironment.fog.density.toFixed(2) + ")"
                    }
                    Slider {
                        id: valDensity
                        focusPolicy: Qt.NoFocus
                        Layout.fillWidth: true
                        from: 0.0
                        to: 1.0
                        value: sceneEnvironment.fog.density
                        onValueChanged: sceneEnvironment.fog.density = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Color"
                        Layout.fillWidth: true
                    }
                    ColorPicker {
                        color: sceneEnvironment.fog.color
                        onColorChanged: sceneEnvironment.fog.color = color
                    }
                }

                // DEPTH FOG
                CheckBox {
                    text: "Depth fog enabled"
                    checked: sceneEnvironment.fog.depthEnabled
                    onCheckedChanged: {
                        sceneEnvironment.fog.depthEnabled = checked
                    }
                }
                RowLayout {
                    Label {
                        text: "Near (" + sceneEnvironment.fog.depthNear.toFixed(2) + ") / Far (" + sceneEnvironment.fog.depthFar.toFixed(2) + ")"
                    }
                    RangeSlider {
                        id: valDepth
                        focusPolicy: Qt.NoFocus
                        Layout.fillWidth: true
                        from: -1000.0
                        to: 1000.0
                        first.value: sceneEnvironment.fog.depthNear
                        second.value: sceneEnvironment.fog.depthFar
                        first.onValueChanged: sceneEnvironment.fog.depthNear = first.value
                        second.onValueChanged: sceneEnvironment.fog.depthFar = second.value
                    }
                }
                RowLayout {
                    Label {
                        text: "Curve (" + sceneEnvironment.fog.depthCurve.toFixed(2) + ")"
                    }
                    Slider {
                        id: valDepthCurve
                        focusPolicy: Qt.NoFocus
                        Layout.fillWidth: true
                        from: 0.0
                        to: 1.0
                        value: sceneEnvironment.fog.depthCurve
                        onValueChanged: sceneEnvironment.fog.depthCurve = value
                    }
                }

                // HEIGHT FOG
                CheckBox {
                    text: "Height fog enabled"
                    checked: sceneEnvironment.fog.heightEnabled
                    onCheckedChanged: {
                        sceneEnvironment.fog.heightEnabled = checked
                    }
                }
                RowLayout {
                    Label {
                        text: "Least Intense Y (" + sceneEnvironment.fog.leastIntenseY.toFixed(2) + ")"
                    }
                    Slider {
                        id: valHeightMin
                        focusPolicy: Qt.NoFocus
                        Layout.fillWidth: true
                        from: -1000.0
                        to: 1000.0
                        value: sceneEnvironment.fog.leastIntenseY
                        onValueChanged: sceneEnvironment.fog.leastIntenseY = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Most Intense Y (" + sceneEnvironment.fog.mostIntenseY.toFixed(2) + ")"
                    }
                    Slider {
                        id: valHeightMax
                        focusPolicy: Qt.NoFocus
                        Layout.fillWidth: true
                        from: -1000.0
                        to: 1000.0
                        value: sceneEnvironment.fog.mostIntenseY
                        onValueChanged: sceneEnvironment.fog.mostIntenseY = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Curve (" + sceneEnvironment.fog.heightCurve.toFixed(2) + ")"
                    }
                    Slider {
                        id: valHeightCurve
                        focusPolicy: Qt.NoFocus
                        Layout.fillWidth: true
                        from: 0.0
                        to: 100.0
                        value: sceneEnvironment.fog.heightCurve
                        onValueChanged: sceneEnvironment.fog.heightCurve = value
                    }
                }

                // TRANSMISSION
                CheckBox {
                    text: "Light transmission enabled"
                    checked: sceneEnvironment.fog.transmitEnabled
                    onCheckedChanged: {
                        sceneEnvironment.fog.transmitEnabled = checked
                    }
                }
                RowLayout {
                    Label {
                        text: "Curve (" + sceneEnvironment.fog.transmitCurve.toFixed(2) + ")"
                    }
                    Slider {
                        id: valTransmitCurve
                        focusPolicy: Qt.NoFocus
                        Layout.fillWidth: true
                        from: 0.0
                        to: 100.0
                        value: sceneEnvironment.fog.transmitCurve
                        onValueChanged: sceneEnvironment.fog.transmitCurve = value
                    }
                }

                Button {
                    text: "Reset to Defaults"
                    onClicked: {
                        sceneEnvironment.fog.enabled = false
                        sceneEnvironment.fog.depthEnabled = false
                        sceneEnvironment.fog.heightEnabled = false
                        sceneEnvironment.fog.transmitEnabled = false

                        sceneEnvironment.fog.density = 1.0;
                        sceneEnvironment.fog.color = Qt.rgba(0.5, 0.6, 0.7, 1.0);
                        sceneEnvironment.fog.depthNear = 10.0;
                        sceneEnvironment.fog.depthFar = 1000.0;
                        sceneEnvironment.fog.depthCurve = 1.0;
                        sceneEnvironment.fog.leastIntenseY = 10.0;
                        sceneEnvironment.fog.mostIntenseY = 0.0;
                        sceneEnvironment.fog.heightCurve = 1.0;
                        sceneEnvironment.fog.transmitCurve = 1.0;
                    }
                }
            }

            // GLOW
            SectionLayout {
                id: glowSection
                title: "Glow"

                CheckBox {
                    text: "Enable Glow"
                    checked: sceneEnvironment.glowEnabled
                    onCheckedChanged: {
                        sceneEnvironment.glowEnabled = checked
                    }
                }
                CheckBox {
                    text: "High Quality Mode"
                    checked: sceneEnvironment.glowQualityHigh
                    onCheckedChanged: {
                        sceneEnvironment.glowQualityHigh = checked
                    }
                }
                CheckBox {
                    text: "Use Bicubic Upscale"
                    checked: sceneEnvironment.glowUseBicubicUpscale
                    onCheckedChanged: {
                        sceneEnvironment.glowUseBicubicUpscale = checked
                    }
                }
                RowLayout {
                    Label {
                        text: "Strength (" + (sceneEnvironment.glowStrength).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 2.0
                        value: sceneEnvironment.glowStrength
                        onValueChanged:
                            sceneEnvironment.glowStrength = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Intensity (" + (sceneEnvironment.glowIntensity).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 2.0
                        value: sceneEnvironment.glowIntensity
                        onValueChanged:
                            sceneEnvironment.glowIntensity = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Bloom (" + (sceneEnvironment.glowBloom).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 1.0
                        value: sceneEnvironment.glowBloom
                        onValueChanged:
                            sceneEnvironment.glowBloom = value
                    }
                }
                RowLayout {
                    Label {
                        text: "HDR Upper Threshold (" + (sceneEnvironment.glowHDRMaximumValue).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 256.0
                        value: sceneEnvironment.glowHDRMaximumValue
                        onValueChanged:
                            sceneEnvironment.glowHDRMaximumValue = value
                    }
                }
                RowLayout {
                    Label {
                        text: "HDR Lower Threshold (" + (sceneEnvironment.glowHDRMinimumValue).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 4.0
                        value: sceneEnvironment.glowHDRMinimumValue
                        onValueChanged:
                            sceneEnvironment.glowHDRMinimumValue = value
                    }
                }
                RowLayout {
                    Label {
                        text: "HDR Scale (" + (sceneEnvironment.glowHDRScale).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 4.0
                        value: sceneEnvironment.glowHDRScale
                        onValueChanged:
                            sceneEnvironment.glowHDRScale = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Blend Mode"
                        Layout.fillWidth: true
                    }

                    ComboBox {
                        id: glowBlendModeComboBox
                        textRole: "text"
                        valueRole: "value"
                        implicitContentWidthPolicy: ComboBox.WidestText
                        onActivated: sceneEnvironment.glowBlendMode = currentValue

                        Component.onCompleted: glowBlendModeComboBox.currentIndex = glowBlendModeComboBox.indexOfValue(sceneEnvironment.glowBlendMode)

                        model: [
                            { value: ExtendedSceneEnvironment.GlowBlendMode.Additive, text: "Additive"},
                            { value: ExtendedSceneEnvironment.GlowBlendMode.Screen, text: "Screen"},
                            { value: ExtendedSceneEnvironment.GlowBlendMode.SoftLight, text: "Softlight"},
                            { value: ExtendedSceneEnvironment.GlowBlendMode.Replace, text: "Replace"}
                        ]
                    }
                }

                RowLayout {
                    id: glowLevelSettings
                    Label {
                        text: "Glow Level"
                        Layout.fillWidth: true
                    }

                    function updateGlowLevels(value, enable) {
                        if (enable)
                           sceneEnvironment.glowLevel |= value
                        else
                           sceneEnvironment.glowLevel &= ~value
                    }

                    ColumnLayout {
                        id: glowLevelCheckBoxes
                        function isChecked(flag) { return ((sceneEnvironment.glowLevel & flag) === flag) }
                        CheckBox {
                            text: qsTr("One")
                            checked: glowLevelCheckBoxes.isChecked(ExtendedSceneEnvironment.GlowLevel.One)
                            onCheckStateChanged: {
                                glowLevelSettings.updateGlowLevels(ExtendedSceneEnvironment.GlowLevel.One, checkState === Qt.Checked)
                            }
                        }
                        CheckBox {
                            text: qsTr("Two")
                            checked: glowLevelCheckBoxes.isChecked(ExtendedSceneEnvironment.GlowLevel.Two)
                            onCheckStateChanged: {
                                glowLevelSettings.updateGlowLevels(ExtendedSceneEnvironment.GlowLevel.Two, checkState === Qt.Checked)
                            }
                        }
                        CheckBox {
                            text: qsTr("Three")
                            checked: glowLevelCheckBoxes.isChecked(ExtendedSceneEnvironment.GlowLevel.Three)
                            onCheckStateChanged: {
                                glowLevelSettings.updateGlowLevels(ExtendedSceneEnvironment.GlowLevel.Three, checkState === Qt.Checked)
                            }
                        }
                        CheckBox {
                            text: qsTr("Four")
                            checked: glowLevelCheckBoxes.isChecked(ExtendedSceneEnvironment.GlowLevel.Four)
                            onCheckStateChanged: {
                                glowLevelSettings.updateGlowLevels(ExtendedSceneEnvironment.GlowLevel.Four, checkState === Qt.Checked)
                            }
                        }
                        CheckBox {
                            text: qsTr("Five")
                            checked: glowLevelCheckBoxes.isChecked(ExtendedSceneEnvironment.GlowLevel.Five)
                            onCheckStateChanged: {
                                glowLevelSettings.updateGlowLevels(ExtendedSceneEnvironment.GlowLevel.Five, checkState === Qt.Checked)
                            }
                        }
                        CheckBox {
                            text: qsTr("Six")
                            checked: glowLevelCheckBoxes.isChecked(ExtendedSceneEnvironment.GlowLevel.Six)
                            onCheckStateChanged: {
                                glowLevelSettings.updateGlowLevels(ExtendedSceneEnvironment.GlowLevel.Six, checkState === Qt.Checked)
                            }
                        }
                        CheckBox {
                            text: qsTr("Seven")
                            checked: glowLevelCheckBoxes.isChecked(ExtendedSceneEnvironment.GlowLevel.Seven)
                            onCheckStateChanged: {
                                glowLevelSettings.updateGlowLevels(ExtendedSceneEnvironment.GlowLevel.Seven, checkState === Qt.Checked)
                            }
                        }
                    }
                }
                Button {
                    text: "Reset to Defaults"
                    onClicked: {
                        sceneEnvironment.glowQualityHigh = false
                        sceneEnvironment.glowUseBicubicUpscale = false
                        sceneEnvironment.glowStrength = 1.0
                        sceneEnvironment.glowIntensity = 0.8
                        sceneEnvironment.glowBloom = 0.0
                        sceneEnvironment.glowBlendMode = 2
                        sceneEnvironment.glowHDRMaximumValue = 12.0
                        sceneEnvironment.glowHDRScale = 2.0
                        sceneEnvironment.glowHDRMinimumValue = 1.0
                        sceneEnvironment.glowLevel = 1
                    }
                }

            }

            // TONEMAPPING
            SectionLayout {
                id: tonemappingSection
                title: "Tonemapping"
                RowLayout {
                    Label {
                        text: "Tonemapping Mode"
                        Layout.fillWidth: true
                    }

                    ComboBox {
                        id: tonemappingModeComboBox
                        textRole: "text"
                        valueRole: "value"
                        implicitContentWidthPolicy: ComboBox.WidestText
                        onActivated: sceneEnvironment.tonemapMode = currentValue
                        Component.onCompleted: tonemappingModeComboBox.currentIndex = tonemappingModeComboBox.indexOfValue(sceneEnvironment.tonemapMode)
                        model: [
                            { value: SceneEnvironment.TonemapModeNone, text: "None"},
                            { value: SceneEnvironment.TonemapModeAces, text: "ACES"},
                            { value: SceneEnvironment.TonemapModeFilmic, text: "Filmic"},
                            { value: SceneEnvironment.TonemapModeHejlDawson, text: "HejlDawson"},
                            { value: SceneEnvironment.TonemapModeLinear, text: "Linear"}
                        ]
                    }
                }

                RowLayout {
                    Label {
                        text: "Exposure (" + (sceneEnvironment.exposure).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 10.0
                        value: sceneEnvironment.exposure
                        onValueChanged:
                            sceneEnvironment.exposure = value
                    }
                }
                RowLayout {
                    Label {
                        text: "White Point (" + (sceneEnvironment.whitePoint).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 1.0
                        value: sceneEnvironment.whitePoint
                        onValueChanged:
                            sceneEnvironment.whitePoint = value
                    }
                }

                RowLayout {
                    Label {
                        text: "Sharpness Amount (" + (sceneEnvironment.sharpnessAmount).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 1.0
                        value: sceneEnvironment.sharpnessAmount
                        onValueChanged:
                            sceneEnvironment.sharpnessAmount = value
                    }
                }

                CheckBox {
                    text: "Enable Dithering"
                    checked: sceneEnvironment.ditheringEnabled
                    onCheckedChanged: {
                        sceneEnvironment.ditheringEnabled = checked
                    }
                }
                Button {
                    text: "Reset to Defaults"
                    onClicked: {
                        sceneEnvironment.tonemapMode = SceneEnvironment.TonemapModeLinear
                        sceneEnvironment.exposure = 1.0
                        sceneEnvironment.whitePoint = 1.0
                        sceneEnvironment.sharpnessAmount = 0.0
                        sceneEnvironment.ditheringEnabled = false
                    }
                }
            }

            // ADJUSTMENT
            SectionLayout {
                id: adjustmentsSection
                title: "Adjustments"
                CheckBox {
                    text: "Enable Color Adjustments"
                    checked: sceneEnvironment.colorAdjustmentsEnabled
                    onCheckedChanged: {
                        sceneEnvironment.colorAdjustmentsEnabled = checked
                    }
                }
                RowLayout {
                    Label {
                        text: "Brightness (" + (sceneEnvironment.adjustmentBrightness).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.01
                        to: 8.0
                        stepSize: 0.01
                        value: sceneEnvironment.adjustmentBrightness
                        onValueChanged:
                            sceneEnvironment.adjustmentBrightness = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Contrast (" + (sceneEnvironment.adjustmentContrast).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.01
                        to: 8.0
                        stepSize: 0.01
                        value: sceneEnvironment.adjustmentContrast
                        onValueChanged:
                            sceneEnvironment.adjustmentContrast = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Saturation (" + (sceneEnvironment.adjustmentSaturation).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.01
                        to: 8.0
                        stepSize: 0.01
                        value: sceneEnvironment.adjustmentSaturation
                        onValueChanged:
                            sceneEnvironment.adjustmentSaturation = value
                    }
                }
                Button {
                    text: "Reset to Defaults"
                    onClicked: {
                        //sceneEnvironment.bcsAdjustments = Qt.vector3d(1.0, 1.0, 1.0);
                        sceneEnvironment.adjustmentBrightness = 1.0
                        sceneEnvironment.adjustmentContrast = 1.0
                        sceneEnvironment.adjustmentSaturation = 1.0
                    }
                }
            }

            // COLORGRADING
            SectionLayout {
                id: colorGradingSection
                title: "Color Grading"
                CheckBox {
                    text: "Enable"
                    checked: sceneEnvironment.lutEnabled
                    onCheckedChanged: {
                        sceneEnvironment.lutEnabled = checked
                    }
                }
                RowLayout {
                    Label {
                        text: "Alpha Filtering (" + (sceneEnvironment.lutFilterAlpha).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 1.0
                        value: sceneEnvironment.lutFilterAlpha
                        onValueChanged:
                            sceneEnvironment.lutFilterAlpha = value
                    }
                }

                RowLayout {
                    Label {
                        text: "Look Up Texture (LUT)"
                        Layout.fillWidth: true
                    }

                    ComboBox {
                        id: lutSourceTextureComboBox
                        textRole: "text"
                        valueRole: "value"
                        implicitContentWidthPolicy: ComboBox.WidestText
                        onActivated: lutTexture.source = currentValue
                        Component.onCompleted: lutSourceTextureComboBox.currentIndex = lutSourceTextureComboBox.indexOfValue(lutTexture.source)
                        model: [
                            { value: Qt.url("qrc:/luts/grayscale.png"), text: "Greyscale"},
                            { value: Qt.url("qrc:/luts/identity.png"), text: "Identity"},
                            { value: Qt.url("qrc:/luts/inverted.png"), text: "Inverted"}
                        ]
                    }
                }
                Button {
                    text: "Reset to Defaults"
                    onClicked: {
                        sceneEnvironment.lutFilterAlpha = 1.0;
                        lutTexture.source = Qt.url("qrc:/luts/identity.png");
                    }
                }
            }

            // VIGNETTE
            SectionLayout {
                id: vignetteSection
                title: "Vignette"
                CheckBox {
                    text: "Enable"
                    checked: sceneEnvironment.vignetteEnabled
                    onCheckedChanged: {
                        sceneEnvironment.vignetteEnabled = checked
                    }
                }
                RowLayout {
                    Label {
                        text: "Vignette Strength(" + (sceneEnvironment.vignetteStrength).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 15.0
                        value: sceneEnvironment.vignetteStrength
                        onValueChanged:
                            sceneEnvironment.vignetteStrength = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Vignette Color"
                        Layout.fillWidth: true
                    }
                    ColorPicker {
                        color: sceneEnvironment.vignetteColor
                        onColorChanged:
                            sceneEnvironment.vignetteColor = color
                    }
                }
                RowLayout {
                    Label {
                        text: "Vignette Radius(" + (sceneEnvironment.vignetteRadius).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 5.0
                        value: sceneEnvironment.vignetteRadius
                        onValueChanged:
                            sceneEnvironment.vignetteRadius = value
                    }
                }
                Button {
                    text: "Reset to Defaults"
                    onClicked: {
                        sceneEnvironment.vignetteStrength = 15.0
                        sceneEnvironment.vignetteColor = Qt.rgba(0.5, 0.5, 0.5, 1.0)
                        sceneEnvironment.vignetteRadius = 0.35
                    }
                }
            }

            // LENSFLARE
            SectionLayout {
                id: lensFlareSection
                title: "Lens Flare"
                CheckBox {
                    text: "Enable"
                    checked: sceneEnvironment.lensFlareEnabled
                    onCheckedChanged: {
                        sceneEnvironment.lensFlareEnabled = checked
                    }
                }
                RowLayout {
                    Label {
                        text: "Bloom Scale(" + (sceneEnvironment.lensFlareBloomScale).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 20.0
                        value: sceneEnvironment.lensFlareBloomScale
                        onValueChanged:
                            sceneEnvironment.lensFlareBloomScale = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Bloom Bias(" + (sceneEnvironment.lensFlareBloomBias).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 10.0
                        value: sceneEnvironment.lensFlareBloomBias
                        onValueChanged:
                            sceneEnvironment.lensFlareBloomBias = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Ghost Dispersal(" + (sceneEnvironment.lensFlareGhostDispersal).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 1.0
                        value: sceneEnvironment.lensFlareGhostDispersal
                        onValueChanged:
                            sceneEnvironment.lensFlareGhostDispersal = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Ghost Count(" + (sceneEnvironment.lensFlareGhostCount) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0
                        to: 20
                        stepSize: 1
                        value: sceneEnvironment.lensFlareGhostCount
                        onValueChanged:
                            sceneEnvironment.lensFlareGhostCount = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Halo Width(" + (sceneEnvironment.lensFlareHaloWidth).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 1.0
                        value: sceneEnvironment.lensFlareHaloWidth
                        onValueChanged:
                            sceneEnvironment.lensFlareHaloWidth = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Stretch Aspect(" + (sceneEnvironment.lensFlareStretchToAspect).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 1.0
                        value: sceneEnvironment.lensFlareStretchToAspect
                        onValueChanged:
                            sceneEnvironment.lensFlareStretchToAspect = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Distortion(" + (sceneEnvironment.lensFlareDistortion).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 20.0
                        value: sceneEnvironment.lensFlareDistortion
                        onValueChanged:
                            sceneEnvironment.lensFlareDistortion = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Blur Amount(" + (sceneEnvironment.lensFlareBlurAmount).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 50.0
                        value: sceneEnvironment.lensFlareBlurAmount
                        onValueChanged:
                            sceneEnvironment.lensFlareBlurAmount = value
                    }
                }
                CheckBox {
                    text: "Apply Lens Dirt"
                    checked: sceneEnvironment.lensFlareApplyDirtTexture
                    onCheckedChanged: {
                        sceneEnvironment.lensFlareApplyDirtTexture = checked
                    }
                }
                CheckBox {
                    text: "Apply Starburst"
                    checked: sceneEnvironment.lensFlareApplyStarburstTexture
                    onCheckedChanged: {
                        sceneEnvironment.lensFlareApplyStarburstTexture = checked
                    }
                }
                Button {
                    text: "Reset to Defaults"
                    onClicked: {
                        sceneEnvironment.lensFlareBloomScale = 10
                        sceneEnvironment.lensFlareBloomBias = 2.75
                        sceneEnvironment.lensFlareGhostDispersal = 0.5
                        sceneEnvironment.lensFlareGhostCount = 4
                        sceneEnvironment.lensFlareHaloWidth = 0.25
                        sceneEnvironment.lensFlareStretchToAspect = 0.5
                        sceneEnvironment.lensFlareDistortion = 5
                        sceneEnvironment.lensFlareBlurAmount = 3
                        sceneEnvironment.lensFlareApplyDirtTexture = true
                        sceneEnvironment.lensFlareApplyStarburstTexture = true
                    }
                }
            }
        }
    }
}
