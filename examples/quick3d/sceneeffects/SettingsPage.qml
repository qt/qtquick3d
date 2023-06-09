// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D
import QtQuick3D.Helpers

Rectangle {
    id: toolPage

    required property PerspectiveCamera camera
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
                        onActivated: toolPage.sceneEnvironment.antialiasingMode = currentValue

                        Component.onCompleted: antialiasingModeComboBox.currentIndex = antialiasingModeComboBox.indexOfValue(toolPage.sceneEnvironment.antialiasingMode)

                        model: [
                            { value: SceneEnvironment.NoAA, text: "No Antialiasing"},
                            { value: SceneEnvironment.SSAA, text: "Supersample AA"},
                            { value: SceneEnvironment.MSAA, text: "Multisample AA"},
                            { value: SceneEnvironment.ProgressiveAA, text: "Progressive AA"}
                        ]
                    }
                }
                RowLayout {
                    visible: toolPage.sceneEnvironment.antialiasingMode !== SceneEnvironment.NoAA
                    Label {
                        text: "AA Quality"
                        Layout.fillWidth: true
                    }

                    ComboBox {
                        id: antialiasingQualityComboBox
                        textRole: "text"
                        valueRole: "value"
                        implicitContentWidthPolicy: ComboBox.WidestText
                        onActivated: toolPage.sceneEnvironment.antialiasingQuality = currentValue
                        Component.onCompleted: antialiasingQualityComboBox.currentIndex = antialiasingQualityComboBox.indexOfValue(toolPage.sceneEnvironment.antialiasingQuality)
                        model: [
                            { value: SceneEnvironment.Medium, text: "Medium"},
                            { value: SceneEnvironment.High, text: "High"},
                            { value: SceneEnvironment.VeryHigh, text: "VeryHigh"}
                        ]
                    }
                }
                CheckBox {
                    text: "Enable FXAA"
                    checked: toolPage.sceneEnvironment.fxaaEnabled
                    onCheckedChanged: {
                        toolPage.sceneEnvironment.fxaaEnabled = checked
                    }
                }

                CheckBox {
                    text: "Enable Temporal AA"
                    checked: toolPage.sceneEnvironment.temporalAAEnabled
                    onCheckedChanged: {
                        toolPage.sceneEnvironment.temporalAAEnabled = checked
                    }
                }

                RowLayout {
                    visible: toolPage.sceneEnvironment.temporalAAEnabled
                    Label {
                        text: "Temporal AA Strength (" + toolPage.sceneEnvironment.temporalAAStrength.toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 1.0
                        value: toolPage.sceneEnvironment.temporalAAStrength
                        onValueChanged:
                            toolPage.sceneEnvironment.temporalAAStrength = value
                    }
                }

                CheckBox {
                    text: "Enable Specular AA"
                    checked: toolPage.sceneEnvironment.specularAAEnabled
                    onCheckedChanged: {
                        toolPage.sceneEnvironment.specularAAEnabled = checked
                    }
                }
            }

            // AO
            SectionLayout {
                id: aoSection
                title: "Ambient Occlusion"

                CheckBox {
                    text: "Enable SSAO"
                    checked: toolPage.sceneEnvironment.aoEnabled
                    onCheckedChanged: {
                        toolPage.sceneEnvironment.aoEnabled = checked
                    }
                }
                RowLayout {
                    Label {
                        text: "Strength (" + (toolPage.sceneEnvironment.aoStrength).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 100.0
                        value: toolPage.sceneEnvironment.aoStrength
                        onValueChanged: {
                            toolPage.sceneEnvironment.aoStrength = value
                        }
                    }
                }
                RowLayout {
                    Label {
                        text: "Softness (" + (toolPage.sceneEnvironment.aoSoftness).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 50.0
                        value: toolPage.sceneEnvironment.aoSoftness
                        onValueChanged:
                            toolPage.sceneEnvironment.aoSoftness = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Distance (" + (toolPage.sceneEnvironment.aoDistance).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 5.0
                        value: toolPage.sceneEnvironment.aoDistance
                        onValueChanged:
                            toolPage.sceneEnvironment.aoDistance = value
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
                        onActivated: toolPage.sceneEnvironment.aoSampleRate = currentValue

                        Component.onCompleted: aoSampleRateComboBox.currentIndex = aoSampleRateComboBox.indexOfValue(toolPage.sceneEnvironment.aoSampleRate)

                        model: [
                            { value: 2, text: "2"},
                            { value: 3, text: "3"},
                            { value: 4, text: "4"}
                        ]
                    }
                }
                RowLayout {
                    Label {
                        text: "Bias (" + (toolPage.sceneEnvironment.aoBias).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: -1.0
                        to: 1.0
                        stepSize: 0.01
                        value: toolPage.sceneEnvironment.aoBias
                        onValueChanged:
                            toolPage.sceneEnvironment.aoBias = value
                    }
                }
                CheckBox {
                    text: "Enable AO Dither"
                    checked: toolPage.sceneEnvironment.aoDither
                    onCheckedChanged: {
                        toolPage.sceneEnvironment.aoDither = checked
                    }
                }
            }

            // DOF
            SectionLayout {
                id: dofSection
                title: "Depth of Field"

                CheckBox {
                    text: "Enabled"
                    checked: toolPage.sceneEnvironment.depthOfFieldEnabled
                    onCheckedChanged: {
                        toolPage.sceneEnvironment.depthOfFieldEnabled = checked
                    }
                }

                Label {
                    text: "Focus Distance (" + (toolPage.sceneEnvironment.depthOfFieldFocusDistance).toFixed(2) + ")"
                    Layout.fillWidth: true
                }

                Label {
                    text: "Focus Range (" + (toolPage.sceneEnvironment.depthOfFieldFocusRange).toFixed(2) +")"
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
                    from: toolPage.camera.clipNear
                    to: toolPage.camera.clipFar
                    Layout.fillWidth: true
                    Component.onCompleted: {
                        first.value = toolPage.sceneEnvironment.depthOfFieldFocusDistance - toolPage.sceneEnvironment.depthOfFieldFocusRange * 0.5
                        second.value = toolPage.sceneEnvironment.depthOfFieldFocusDistance + toolPage.sceneEnvironment.depthOfFieldFocusRange * 0.5
                    }
                    first.onMoved: {
                        toolPage.sceneEnvironment.depthOfFieldFocusRange = second.value - first.value
                        toolPage.sceneEnvironment.depthOfFieldFocusDistance = first.value + toolPage.sceneEnvironment.depthOfFieldFocusRange * 0.5
                    }
                    second.onMoved: {
                        toolPage.sceneEnvironment.depthOfFieldFocusRange = second.value - first.value
                        toolPage.sceneEnvironment.depthOfFieldFocusDistance = first.value + toolPage.sceneEnvironment.depthOfFieldFocusRange * 0.5
                    }
                }

                RowLayout {
                    Label {
                        text: "Blur Amount (" + (toolPage.sceneEnvironment.depthOfFieldBlurAmount).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 25.0
                        value: toolPage.sceneEnvironment.depthOfFieldBlurAmount
                        onValueChanged:
                            toolPage.sceneEnvironment.depthOfFieldBlurAmount = value
                    }
                }

            }

            // FOG
            SectionLayout {
                id: fogSection
                title: "Fog"

                CheckBox {
                    text: "Enabled"
                    checked: toolPage.sceneEnvironment.fog.enabled
                    onCheckedChanged: {
                        toolPage.sceneEnvironment.fog.enabled = checked
                    }
                }
                RowLayout {
                    Label {
                        text: "Density (" + toolPage.sceneEnvironment.fog.density.toFixed(2) + ")"
                    }
                    Slider {
                        id: valDensity
                        focusPolicy: Qt.NoFocus
                        Layout.fillWidth: true
                        from: 0.0
                        to: 1.0
                        value: toolPage.sceneEnvironment.fog.density
                        onValueChanged: toolPage.sceneEnvironment.fog.density = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Color"
                        Layout.fillWidth: true
                    }
                    ColorPicker {
                        color: toolPage.sceneEnvironment.fog.color
                        onColorChanged: toolPage.sceneEnvironment.fog.color = color
                    }
                }

                // DEPTH FOG
                CheckBox {
                    text: "Depth fog enabled"
                    checked: toolPage.sceneEnvironment.fog.depthEnabled
                    onCheckedChanged: {
                        toolPage.sceneEnvironment.fog.depthEnabled = checked
                    }
                }
                RowLayout {
                    Label {
                        text: "Near (" + toolPage.sceneEnvironment.fog.depthNear.toFixed(2) + ") / Far (" + toolPage.sceneEnvironment.fog.depthFar.toFixed(2) + ")"
                    }
                    RangeSlider {
                        id: valDepth
                        focusPolicy: Qt.NoFocus
                        Layout.fillWidth: true
                        from: -1000.0
                        to: 1000.0
                        first.value: toolPage.sceneEnvironment.fog.depthNear
                        second.value: toolPage.sceneEnvironment.fog.depthFar
                        first.onValueChanged: toolPage.sceneEnvironment.fog.depthNear = first.value
                        second.onValueChanged: toolPage.sceneEnvironment.fog.depthFar = second.value
                    }
                }
                RowLayout {
                    Label {
                        text: "Curve (" + toolPage.sceneEnvironment.fog.depthCurve.toFixed(2) + ")"
                    }
                    Slider {
                        id: valDepthCurve
                        focusPolicy: Qt.NoFocus
                        Layout.fillWidth: true
                        from: 0.0
                        to: 1.0
                        value: toolPage.sceneEnvironment.fog.depthCurve
                        onValueChanged: toolPage.sceneEnvironment.fog.depthCurve = value
                    }
                }

                // HEIGHT FOG
                CheckBox {
                    text: "Height fog enabled"
                    checked: toolPage.sceneEnvironment.fog.heightEnabled
                    onCheckedChanged: {
                        toolPage.sceneEnvironment.fog.heightEnabled = checked
                    }
                }
                RowLayout {
                    Label {
                        text: "Least Intense Y (" + toolPage.sceneEnvironment.fog.leastIntenseY.toFixed(2) + ")"
                    }
                    Slider {
                        id: valHeightMin
                        focusPolicy: Qt.NoFocus
                        Layout.fillWidth: true
                        from: -1000.0
                        to: 1000.0
                        value: toolPage.sceneEnvironment.fog.leastIntenseY
                        onValueChanged: toolPage.sceneEnvironment.fog.leastIntenseY = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Most Intense Y (" + toolPage.sceneEnvironment.fog.mostIntenseY.toFixed(2) + ")"
                    }
                    Slider {
                        id: valHeightMax
                        focusPolicy: Qt.NoFocus
                        Layout.fillWidth: true
                        from: -1000.0
                        to: 1000.0
                        value: toolPage.sceneEnvironment.fog.mostIntenseY
                        onValueChanged: toolPage.sceneEnvironment.fog.mostIntenseY = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Curve (" + toolPage.sceneEnvironment.fog.heightCurve.toFixed(2) + ")"
                    }
                    Slider {
                        id: valHeightCurve
                        focusPolicy: Qt.NoFocus
                        Layout.fillWidth: true
                        from: 0.0
                        to: 100.0
                        value: toolPage.sceneEnvironment.fog.heightCurve
                        onValueChanged: toolPage.sceneEnvironment.fog.heightCurve = value
                    }
                }

                // TRANSMISSION
                CheckBox {
                    text: "Light transmission enabled"
                    checked: toolPage.sceneEnvironment.fog.transmitEnabled
                    onCheckedChanged: {
                        toolPage.sceneEnvironment.fog.transmitEnabled = checked
                    }
                }
                RowLayout {
                    Label {
                        text: "Curve (" + toolPage.sceneEnvironment.fog.transmitCurve.toFixed(2) + ")"
                    }
                    Slider {
                        id: valTransmitCurve
                        focusPolicy: Qt.NoFocus
                        Layout.fillWidth: true
                        from: 0.0
                        to: 100.0
                        value: toolPage.sceneEnvironment.fog.transmitCurve
                        onValueChanged: toolPage.sceneEnvironment.fog.transmitCurve = value
                    }
                }

                Button {
                    text: "Reset to Defaults"
                    onClicked: {
                        toolPage.sceneEnvironment.fog.enabled = false
                        toolPage.sceneEnvironment.fog.depthEnabled = false
                        toolPage.sceneEnvironment.fog.heightEnabled = false
                        toolPage.sceneEnvironment.fog.transmitEnabled = false

                        toolPage.sceneEnvironment.fog.density = 1.0;
                        toolPage.sceneEnvironment.fog.color = Qt.rgba(0.5, 0.6, 0.7, 1.0);
                        toolPage.sceneEnvironment.fog.depthNear = 10.0;
                        toolPage.sceneEnvironment.fog.depthFar = 1000.0;
                        toolPage.sceneEnvironment.fog.depthCurve = 1.0;
                        toolPage.sceneEnvironment.fog.leastIntenseY = 10.0;
                        toolPage.sceneEnvironment.fog.mostIntenseY = 0.0;
                        toolPage.sceneEnvironment.fog.heightCurve = 1.0;
                        toolPage.sceneEnvironment.fog.transmitCurve = 1.0;
                    }
                }
            }

            // GLOW
            SectionLayout {
                id: glowSection
                title: "Glow"

                CheckBox {
                    text: "Enable Glow"
                    checked: toolPage.sceneEnvironment.glowEnabled
                    onCheckedChanged: {
                        toolPage.sceneEnvironment.glowEnabled = checked
                    }
                }
                CheckBox {
                    text: "High Quality Mode"
                    checked: toolPage.sceneEnvironment.glowQualityHigh
                    onCheckedChanged: {
                        toolPage.sceneEnvironment.glowQualityHigh = checked
                    }
                }
                CheckBox {
                    text: "Use Bicubic Upscale"
                    checked: toolPage.sceneEnvironment.glowUseBicubicUpscale
                    onCheckedChanged: {
                        toolPage.sceneEnvironment.glowUseBicubicUpscale = checked
                    }
                }
                RowLayout {
                    Label {
                        text: "Strength (" + (toolPage.sceneEnvironment.glowStrength).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 2.0
                        value: toolPage.sceneEnvironment.glowStrength
                        onValueChanged:
                            toolPage.sceneEnvironment.glowStrength = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Intensity (" + (toolPage.sceneEnvironment.glowIntensity).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 2.0
                        value: toolPage.sceneEnvironment.glowIntensity
                        onValueChanged:
                            toolPage.sceneEnvironment.glowIntensity = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Bloom (" + (toolPage.sceneEnvironment.glowBloom).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 1.0
                        value: toolPage.sceneEnvironment.glowBloom
                        onValueChanged:
                            toolPage.sceneEnvironment.glowBloom = value
                    }
                }
                RowLayout {
                    Label {
                        text: "HDR Upper Threshold (" + (toolPage.sceneEnvironment.glowHDRMaximumValue).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 256.0
                        value: toolPage.sceneEnvironment.glowHDRMaximumValue
                        onValueChanged:
                            toolPage.sceneEnvironment.glowHDRMaximumValue = value
                    }
                }
                RowLayout {
                    Label {
                        text: "HDR Lower Threshold (" + (toolPage.sceneEnvironment.glowHDRMinimumValue).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 4.0
                        value: toolPage.sceneEnvironment.glowHDRMinimumValue
                        onValueChanged:
                            toolPage.sceneEnvironment.glowHDRMinimumValue = value
                    }
                }
                RowLayout {
                    Label {
                        text: "HDR Scale (" + (toolPage.sceneEnvironment.glowHDRScale).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 4.0
                        value: toolPage.sceneEnvironment.glowHDRScale
                        onValueChanged:
                            toolPage.sceneEnvironment.glowHDRScale = value
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
                        onActivated: toolPage.sceneEnvironment.glowBlendMode = currentValue

                        Component.onCompleted: glowBlendModeComboBox.currentIndex = glowBlendModeComboBox.indexOfValue(toolPage.sceneEnvironment.glowBlendMode)

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
                           toolPage.sceneEnvironment.glowLevel |= value
                        else
                           toolPage.sceneEnvironment.glowLevel &= ~value
                    }

                    ColumnLayout {
                        id: glowLevelCheckBoxes
                        function isChecked(flag) { return ((toolPage.sceneEnvironment.glowLevel & flag) === flag) }
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
                        toolPage.sceneEnvironment.glowQualityHigh = false
                        toolPage.sceneEnvironment.glowUseBicubicUpscale = false
                        toolPage.sceneEnvironment.glowStrength = 1.0
                        toolPage.sceneEnvironment.glowIntensity = 0.8
                        toolPage.sceneEnvironment.glowBloom = 0.0
                        toolPage.sceneEnvironment.glowBlendMode = 2
                        toolPage.sceneEnvironment.glowHDRMaximumValue = 12.0
                        toolPage.sceneEnvironment.glowHDRScale = 2.0
                        toolPage.sceneEnvironment.glowHDRMinimumValue = 1.0
                        toolPage.sceneEnvironment.glowLevel = 1
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
                        onActivated: toolPage.sceneEnvironment.tonemapMode = currentValue
                        Component.onCompleted: tonemappingModeComboBox.currentIndex = tonemappingModeComboBox.indexOfValue(toolPage.sceneEnvironment.tonemapMode)
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
                        text: "Exposure (" + (toolPage.sceneEnvironment.exposure).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 10.0
                        value: toolPage.sceneEnvironment.exposure
                        onValueChanged:
                            toolPage.sceneEnvironment.exposure = value
                    }
                }
                RowLayout {
                    Label {
                        text: "White Point (" + (toolPage.sceneEnvironment.whitePoint).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 1.0
                        value: toolPage.sceneEnvironment.whitePoint
                        onValueChanged:
                            toolPage.sceneEnvironment.whitePoint = value
                    }
                }

                RowLayout {
                    Label {
                        text: "Sharpness Amount (" + (toolPage.sceneEnvironment.sharpnessAmount).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 1.0
                        value: toolPage.sceneEnvironment.sharpnessAmount
                        onValueChanged:
                            toolPage.sceneEnvironment.sharpnessAmount = value
                    }
                }

                CheckBox {
                    text: "Enable Dithering"
                    checked: toolPage.sceneEnvironment.ditheringEnabled
                    onCheckedChanged: {
                        toolPage.sceneEnvironment.ditheringEnabled = checked
                    }
                }
                Button {
                    text: "Reset to Defaults"
                    onClicked: {
                        toolPage.sceneEnvironment.tonemapMode = SceneEnvironment.TonemapModeLinear
                        toolPage.sceneEnvironment.exposure = 1.0
                        toolPage.sceneEnvironment.whitePoint = 1.0
                        toolPage.sceneEnvironment.sharpnessAmount = 0.0
                        toolPage.sceneEnvironment.ditheringEnabled = false
                    }
                }
            }

            // ADJUSTMENT
            SectionLayout {
                id: adjustmentsSection
                title: "Adjustments"
                CheckBox {
                    text: "Enable Color Adjustments"
                    checked: toolPage.sceneEnvironment.colorAdjustmentsEnabled
                    onCheckedChanged: {
                        toolPage.sceneEnvironment.colorAdjustmentsEnabled = checked
                    }
                }
                RowLayout {
                    Label {
                        text: "Brightness (" + (toolPage.sceneEnvironment.adjustmentBrightness).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.01
                        to: 8.0
                        stepSize: 0.01
                        value: toolPage.sceneEnvironment.adjustmentBrightness
                        onValueChanged:
                            toolPage.sceneEnvironment.adjustmentBrightness = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Contrast (" + (toolPage.sceneEnvironment.adjustmentContrast).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.01
                        to: 8.0
                        stepSize: 0.01
                        value: toolPage.sceneEnvironment.adjustmentContrast
                        onValueChanged:
                            toolPage.sceneEnvironment.adjustmentContrast = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Saturation (" + (toolPage.sceneEnvironment.adjustmentSaturation).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.01
                        to: 8.0
                        stepSize: 0.01
                        value: toolPage.sceneEnvironment.adjustmentSaturation
                        onValueChanged:
                            toolPage.sceneEnvironment.adjustmentSaturation = value
                    }
                }
                Button {
                    text: "Reset to Defaults"
                    onClicked: {
                        //sceneEnvironment.bcsAdjustments = Qt.vector3d(1.0, 1.0, 1.0);
                        toolPage.sceneEnvironment.adjustmentBrightness = 1.0
                        toolPage.sceneEnvironment.adjustmentContrast = 1.0
                        toolPage.sceneEnvironment.adjustmentSaturation = 1.0
                    }
                }
            }

            // COLORGRADING
            SectionLayout {
                id: colorGradingSection
                title: "Color Grading"
                CheckBox {
                    text: "Enable"
                    checked: toolPage.sceneEnvironment.lutEnabled
                    onCheckedChanged: {
                        toolPage.sceneEnvironment.lutEnabled = checked
                    }
                }
                RowLayout {
                    Label {
                        text: "Alpha Filtering (" + (toolPage.sceneEnvironment.lutFilterAlpha).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 1.0
                        value: toolPage.sceneEnvironment.lutFilterAlpha
                        onValueChanged:
                            toolPage.sceneEnvironment.lutFilterAlpha = value
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
                        onActivated: toolPage.lutTexture.source = currentValue
                        Component.onCompleted: lutSourceTextureComboBox.currentIndex = lutSourceTextureComboBox.indexOfValue(toolPage.lutTexture.source)
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
                        toolPage.sceneEnvironment.lutFilterAlpha = 1.0;
                        toolPage.lutTexture.source = Qt.url("qrc:/luts/identity.png");
                    }
                }
            }

            // VIGNETTE
            SectionLayout {
                id: vignetteSection
                title: "Vignette"
                CheckBox {
                    text: "Enable"
                    checked: toolPage.sceneEnvironment.vignetteEnabled
                    onCheckedChanged: {
                        toolPage.sceneEnvironment.vignetteEnabled = checked
                    }
                }
                RowLayout {
                    Label {
                        text: "Vignette Strength(" + (toolPage.sceneEnvironment.vignetteStrength).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 15.0
                        value: toolPage.sceneEnvironment.vignetteStrength
                        onValueChanged:
                            toolPage.sceneEnvironment.vignetteStrength = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Vignette Color"
                        Layout.fillWidth: true
                    }
                    ColorPicker {
                        color: toolPage.sceneEnvironment.vignetteColor
                        onColorChanged:
                            toolPage.sceneEnvironment.vignetteColor = color
                    }
                }
                RowLayout {
                    Label {
                        text: "Vignette Radius(" + (toolPage.sceneEnvironment.vignetteRadius).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 5.0
                        value: toolPage.sceneEnvironment.vignetteRadius
                        onValueChanged:
                            toolPage.sceneEnvironment.vignetteRadius = value
                    }
                }
                Button {
                    text: "Reset to Defaults"
                    onClicked: {
                        toolPage.sceneEnvironment.vignetteStrength = 15.0
                        toolPage.sceneEnvironment.vignetteColor = Qt.rgba(0.5, 0.5, 0.5, 1.0)
                        toolPage.sceneEnvironment.vignetteRadius = 0.35
                    }
                }
            }

            // LENSFLARE
            SectionLayout {
                id: lensFlareSection
                title: "Lens Flare"
                CheckBox {
                    text: "Enable"
                    checked: toolPage.sceneEnvironment.lensFlareEnabled
                    onCheckedChanged: {
                        toolPage.sceneEnvironment.lensFlareEnabled = checked
                    }
                }
                RowLayout {
                    Label {
                        text: "Bloom Scale(" + (toolPage.sceneEnvironment.lensFlareBloomScale).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 20.0
                        value: toolPage.sceneEnvironment.lensFlareBloomScale
                        onValueChanged:
                            toolPage.sceneEnvironment.lensFlareBloomScale = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Bloom Bias(" + (toolPage.sceneEnvironment.lensFlareBloomBias).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 10.0
                        value: toolPage.sceneEnvironment.lensFlareBloomBias
                        onValueChanged:
                            toolPage.sceneEnvironment.lensFlareBloomBias = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Ghost Dispersal(" + (toolPage.sceneEnvironment.lensFlareGhostDispersal).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 1.0
                        value: toolPage.sceneEnvironment.lensFlareGhostDispersal
                        onValueChanged:
                            toolPage.sceneEnvironment.lensFlareGhostDispersal = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Ghost Count(" + (toolPage.sceneEnvironment.lensFlareGhostCount) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0
                        to: 20
                        stepSize: 1
                        value: toolPage.sceneEnvironment.lensFlareGhostCount
                        onValueChanged:
                            toolPage.sceneEnvironment.lensFlareGhostCount = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Halo Width(" + (toolPage.sceneEnvironment.lensFlareHaloWidth).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 1.0
                        value: toolPage.sceneEnvironment.lensFlareHaloWidth
                        onValueChanged:
                            toolPage.sceneEnvironment.lensFlareHaloWidth = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Stretch Aspect(" + (toolPage.sceneEnvironment.lensFlareStretchToAspect).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 1.0
                        value: toolPage.sceneEnvironment.lensFlareStretchToAspect
                        onValueChanged:
                            toolPage.sceneEnvironment.lensFlareStretchToAspect = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Distortion(" + (toolPage.sceneEnvironment.lensFlareDistortion).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 20.0
                        value: toolPage.sceneEnvironment.lensFlareDistortion
                        onValueChanged:
                            toolPage.sceneEnvironment.lensFlareDistortion = value
                    }
                }
                RowLayout {
                    Label {
                        text: "Blur Amount(" + (toolPage.sceneEnvironment.lensFlareBlurAmount).toFixed(2) + ")"
                        Layout.fillWidth: true
                    }
                    Slider {
                        from: 0.0
                        to: 50.0
                        value: toolPage.sceneEnvironment.lensFlareBlurAmount
                        onValueChanged:
                            toolPage.sceneEnvironment.lensFlareBlurAmount = value
                    }
                }
                CheckBox {
                    text: "Apply Lens Dirt"
                    checked: toolPage.sceneEnvironment.lensFlareApplyDirtTexture
                    onCheckedChanged: {
                        toolPage.sceneEnvironment.lensFlareApplyDirtTexture = checked
                    }
                }
                CheckBox {
                    text: "Apply Starburst"
                    checked: toolPage.sceneEnvironment.lensFlareApplyStarburstTexture
                    onCheckedChanged: {
                        toolPage.sceneEnvironment.lensFlareApplyStarburstTexture = checked
                    }
                }
                Button {
                    text: "Reset to Defaults"
                    onClicked: {
                        toolPage.sceneEnvironment.lensFlareBloomScale = 10
                        toolPage.sceneEnvironment.lensFlareBloomBias = 2.75
                        toolPage.sceneEnvironment.lensFlareGhostDispersal = 0.5
                        toolPage.sceneEnvironment.lensFlareGhostCount = 4
                        toolPage.sceneEnvironment.lensFlareHaloWidth = 0.25
                        toolPage.sceneEnvironment.lensFlareStretchToAspect = 0.5
                        toolPage.sceneEnvironment.lensFlareDistortion = 5
                        toolPage.sceneEnvironment.lensFlareBlurAmount = 3
                        toolPage.sceneEnvironment.lensFlareApplyDirtTexture = true
                        toolPage.sceneEnvironment.lensFlareApplyStarburstTexture = true
                    }
                }
            }
        }
    }
}
