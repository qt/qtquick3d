/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick3D 1.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick3D.Effects 1.15

Window {
    visible: true
    width: 800
    height: 600
    title: qsTr("Quick3D Effects Test")
    color: "#f0f0f0"

    View3D {
        id: view3D
        property real animationValue: 0.0
        property real fastAnimationValue: animationValue * 10
        anchors.left: settings.right
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        NumberAnimation on animationValue {
            id: modelAnimation
            running: true
            paused: !animationButton.checked
            loops: Animation.Infinite
            from: 0.0
            to: 360.0
            duration: 40000
        }

        PerspectiveCamera {
            z: 500
            clipNear: 200
            clipFar: 1000
        }

        DirectionalLight {
            eulerRotation.x: -30
            brightness: 200
        }

        environment: SceneEnvironment {
            id: sceneEnvironment
            clearColor: "#f0f0f0"
            backgroundMode: motionBox.checked ? SceneEnvironment.Transparent : SceneEnvironment.Color

            antialiasingMode: modeButton1.checked ? SceneEnvironment.NoAA : modeButton2.checked
                                                    ? SceneEnvironment.SSAA : modeButton3.checked
                                                      ? SceneEnvironment.MSAA : SceneEnvironment.ProgressiveAA

            antialiasingQuality: qualityButton1.checked ? SceneEnvironment.Medium : qualityButton2.checked
                                                          ? SceneEnvironment.High : SceneEnvironment.VeryHigh
            temporalAAEnabled: temporalModeButton.checked
            temporalAAStrength: temporalStrengthSlider.value

            effects: [  ]
        }

        Texture {
            id: corkTexture
            source: "cork.jpg"
            scaleU: 0.5
            scaleV: 0.5
        }

        Texture {
            id: textTexture
            source: "texture.png"
        }

        Node {
            id: scene

            Model {
                source: "#Cube"
                x: -100
                eulerRotation.y: 45
                eulerRotation.x: 30 + view3D.animationValue
                scale: Qt.vector3d(2, 2, 2)
                materials: DefaultMaterial {
                    diffuseMap: corkTexture
                }
            }

            Model {
                source: "#Cube"
                x: 350 * Math.sin(view3D.fastAnimationValue/180 * Math.PI)
                y: 350 * Math.cos(view3D.fastAnimationValue/180 * Math.PI)
                z: -300
                eulerRotation.y: 5
                eulerRotation.x: 5
                scale: Qt.vector3d(1.2, 1.2, 1.2)
                materials: DefaultMaterial {
                    diffuseMap: textTexture
                }
            }

            Model {
                source: "#Sphere"
                x: 80
                y: -40
                z: 200
                scale: Qt.vector3d(1.4, 1.4, 1.4)
                materials: DefaultMaterial {
                    diffuseColor: "#2abe25"
                }
            }
        }
    }
    Button {
        id: animationButton
        anchors.top: settings.top
        anchors.horizontalCenter: settings.horizontalCenter
        text: "Animate!"
        checkable: true
        z: 1
    }
    ScrollView {
        id: settings
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: settingsRect.width

        Rectangle {
            id: settingsRect

            implicitHeight: settingsArea.height
            width: settingsArea.width + 20



            ColumnLayout {
                id: settingsArea

                spacing: 2

                Item {
                    id: animationButtonSpace
                    Layout.preferredHeight: animationButton.height
                }

                EffectBox {
                    id: customBox
                    text: "Custom effect"
                    effect: CustomEffect {
                        amount: customAmount.value
                    }
                }
                EffectSlider {
                    visible: customBox.checked
                    id: customAmount
                    from: 0.0
                    to: 0.05
                    value: 0.01
                    precision: 4
                }

                EffectBox {
                    id: gradientBox
                    text: "AdditiveColorGradient"
                    effect: AdditiveColorGradient {
                        topColor: gradientTop.colorVector
                        bottomColor: gradientBottom.colorVector
                    }
                }
                EffectColor {
                    visible: gradientBox.checked
                    id: gradientTop
                    colorVector: Qt.vector3d(0.7, 1.0, 0.7)
                    description: "top"
                    buddy: gradientBottom
                }
                EffectColor {
                    visible: gradientBox.checked
                    id: gradientBottom
                    colorVector: Qt.vector3d(0.0, 0.0, 0.0)
                    description: "bottom"
                    buddy: gradientTop
                }
                EffectBox {
                    id: brushBox
                    text: "BrushStrokes"
                    effect: BrushStrokes {
                        brushAngle: brushStrokesAngle.value
                        brushLength: brushStrokesLength.value
                        brushSize: brushStrokesSize.value
                    }
                }
                EffectSlider {
                    visible: brushBox.checked
                    id: brushStrokesAngle
                    from: 0.0
                    to: 360.0
                    precision: 0
                    value: 45
                    description: "brush angle"
                }
                EffectSlider {
                    visible: brushBox.checked
                    id: brushStrokesLength
                    from: 0.0
                    to: 3.0
                    value: 1
                    description: "stroke length"
                }
                EffectSlider {
                    visible: brushBox.checked
                    id: brushStrokesSize
                    from: 10.0
                    to: 200.0
                    value: 100
                    precision: 0
                    description: "stroke size"
                }

                EffectBox {
                    id: chromaticBox
                    text: "ChromaticAberration"
                    effect: ChromaticAberration {
                        aberrationAmount: chromaticAmount.value
                        focusDepth: chromaticDepth.value
                    }
                }
                EffectSlider {
                    visible: chromaticBox.checked
                    id: chromaticAmount
                    from: -200.0
                    to: 200.0
                    value: 50
                    precision: 0
                    description: "aberration amount"
                }
                EffectSlider {
                    visible: chromaticBox.checked
                    id: chromaticDepth
                    from: 0.0
                    to: 1000.0
                    value: 600
                    precision: 0
                    description: "focus depth"
                }
                EffectBox {
                    id: colorMasterBox
                    text: "ColorMaster"
                    effect: ColorMaster {
                        redStrength: colorMasterRed.value
                        greenStrength: colorMasterGreen.value
                        blueStrength: colorMasterBlue.value
                        saturation: colorMasterSaturation.value
                    }
                }
                EffectSlider {
                    visible: colorMasterBox.checked
                    id: colorMasterRed
                    from: 0.0
                    to: 2.0
                    value: 1
                    precision: 2
                    description: "red strength"
                }
                EffectSlider {
                    visible: colorMasterBox.checked
                    id: colorMasterGreen
                    from: 0.0
                    to: 2.0
                    value: 1.5
                    precision: 2
                    description: "green strength"
                }
                EffectSlider {
                    visible: colorMasterBox.checked
                    id: colorMasterBlue
                    from: 0.0
                    to: 2.0
                    value: 1
                    precision: 2
                    description: "blue strength"
                }
                EffectSlider {
                    visible: colorMasterBox.checked
                    id: colorMasterSaturation
                    from: -1.0
                    to: 1.0
                    value: 0
                    precision: 2
                    description: "saturation"
                }

                EffectBox {
                    id: dofBox
                    text: "DepthOfFieldHQBlur"
                    effect: DepthOfFieldHQBlur {
                        focusDistance: dofFocusDistance.value
                        focusRange: dofFocusRange.value
                        blurAmount: dofBlurAmount.value
                    }
                }
                EffectSlider {
                    visible: dofBox.checked
                    id: dofFocusDistance
                    from: 0.0
                    to: 1000.0
                    value: 400
                    precision: 0
                    description: "focus distance"
                }
                EffectSlider {
                    visible: dofBox.checked
                    id: dofFocusRange
                    from: 0.0
                    to: 400.0
                    value: 100
                    precision: 0
                    description: "focus range"
                }
                EffectSlider {
                    visible: dofBox.checked
                    id: dofBlurAmount
                    from: 0.0
                    to: 10.0
                    value: 4
                    precision: 1
                    description: "blur amount"
                }

                EffectBox {
                    id: desaturateBox
                    text: "Desaturate"
                    effect: Desaturate {
                        amount: desaturateAmount.value
                    }
                }
                EffectSlider {
                    visible: desaturateBox.checked
                    id: desaturateAmount
                    from: 0.0
                    to: 1.0
                    value: 0.7
                }

                EffectBox {
                    id: rippleBox
                    text: "DistortionRipple"
                    effect: DistortionRipple {
                        radius: rippleRadius.value
                        distortionWidth: rippleWidth.value
                        distortionHeight: rippleHeight.value
                        distortionPhase: ripplePhase.value
                        center: Qt.vector2d(0.5, 0.5)
                    }
                }
                EffectSlider {
                    visible: rippleBox.checked
                    id: rippleRadius
                    from: 0.0
                    to: 100
                    value: 100
                    description: "radius"
                    precision: 1
                }
                EffectSlider {
                    visible: rippleBox.checked
                    id: rippleWidth
                    from: 2.0
                    to: 100
                    value: 10
                    description: "width"
                    precision: 1
                }
                EffectSlider {
                    visible: rippleBox.checked
                    id: rippleHeight
                    from: 0.0
                    to: 100
                    value: 10
                    description: "height"
                    precision: 1
                }
                EffectSlider {
                    visible: rippleBox.checked
                    id: ripplePhase
                    from: 0.0
                    to: 360
                    value: 100
                    description: "phase"
                    precision: 1
                }

                EffectBox {
                    id: sphereBox
                    text: "DistortionSphere"
                    effect: DistortionSphere {
                        radius: sphereRadius.value
                        distortionHeight: sphereHeight.value
                        center: Qt.vector2d(0.5, 0.5)
                    }
                }
                EffectSlider {
                    visible: sphereBox.checked
                    id: sphereRadius
                    from: 0.0
                    to: 1.0
                    value: 0.25
                    description: "radius"
                }
                EffectSlider {
                    visible: sphereBox.checked
                    id: sphereHeight
                    from: -1.0
                    to: 1.0
                    value: 0.5
                    description: "height"
                }

                EffectBox {
                    id: spiralBox
                    text: "DistortionSpiral"
                    effect: DistortionSpiral {
                        radius: spiralRadius.value
                        distortionStrength: spiralStrength.value
                        center: Qt.vector2d(0.5, 0.5)
                    }
                }
                EffectSlider {
                    visible: spiralBox.checked
                    id: spiralRadius
                    from: 0.0
                    to: 1.0
                    value: 0.25
                    description: "radius"
                }
                EffectSlider {
                    visible: spiralBox.checked
                    id: spiralStrength
                    from: -10.0
                    to: 10.0
                    value: 1
                    precision: 1
                    description: "strength"
                }

                EffectBox {
                    id: edgeBox
                    text: "EdgeDetect"
                    effect: EdgeDetect {
                        edgeStrength: edgeS.value
                    }
                }
                EffectSlider {
                    visible: edgeBox.checked
                    id: edgeS
                    from: 0.0
                    to: 1.0
                    value: 0.5
                    precision: 2
                }

                EffectBox {
                    id: embossBox
                    text: "Emboss"
                    effect: Emboss {
                        amount: embossAmount.value
                    }
                }
                EffectSlider {
                    visible: embossBox.checked
                    id: embossAmount
                    from: 0.0
                    to: 0.01
                    value: 0.003
                    precision: 4
                }

                EffectBox {
                    id: flipBox
                    text: "Flip"
                    effect: Flip {
                        flipHorizontally: flipH.checked
                        flipVertically: flipV.checked
                    }
                }
                CheckBox {
                    visible: flipBox.checked
                    id: flipH
                    checked: true
                    text: "horizontal"
                }
                CheckBox {
                    visible: flipBox.checked
                    id: flipV
                    checked: true
                    text: "vertical"
                }

                EffectBox {
                    id: hdrBox
                    text: "HDRBloomTonemap"
                    effect: HDRBloomTonemap {
                        bloomThreshold: hdrBloomThreshold.value
                        blurFalloff: hdrBlurFalloff.value
                        exposure: hdrExposure.value
                        gamma: hdrGamma.value
                    }
                }
                EffectSlider {
                    visible: hdrBox.checked
                    id: hdrBloomThreshold
                    from: 0.0
                    to: 1.0
                    precision: 2
                    value: 1
                    description: "bloomThreshold"
                }
                EffectSlider {
                    visible: hdrBox.checked
                    id: hdrBlurFalloff
                    from: 0.0
                    to: 10.0
                    precision: 1
                    value: 0
                    description: "blurFalloff"
                }
                EffectSlider {
                    visible: hdrBox.checked
                    id: hdrExposure
                    from: -9
                    to: 9
                    precision: 1
                    value: 0
                    description: "exposure"
                }
                EffectSlider {
                    visible: hdrBox.checked
                    id: hdrGamma
                    from: 0.1
                    to: 4
                    precision: 2
                    value: 1
                    description: "gamma"
                }

                EffectBox {
                    id: motionBox
                    text: "MotionBlur"
                    effect: MotionBlur {
                        fadeAmount: motionAmount.value
                        blurQuality: motionQuality.value
                    }
                }
                EffectSlider {
                    visible: motionBox.checked
                    id: motionAmount
                    from: 0.0
                    to: 1.0
                    precision: 2
                    value: 0.25
                    description: "fadeAmount"
                }
                EffectSlider {
                    visible: motionBox.checked
                    id: motionQuality
                    from: 0.1
                    to: 1.0
                    precision: 2
                    value: 0.25
                    description: "blurQuality"
                }

                EffectBox {
                    id: scatterBox
                    text: "Scatter"
                    effect: Scatter {
                        amount: scatterAmount.value
                        direction: scatterDirection.currentIndex
                        randomize: scatterRandomize.checked
                    }
                }
                EffectSlider {
                    visible: scatterBox.checked
                    id: scatterAmount
                    from: 0
                    to: 127
                    precision: 0
                    value: 10
                    description: "amount"
                }
                ComboBox {
                    visible: scatterBox.checked
                    id: scatterDirection
                    currentIndex: 0
                    displayText: "Dir: " + currentText
                    //0 = both, 1 = horizontal, 2 = vertical
                    model: ["both", "horizontal", "vertical"]
                }
                CheckBox {
                    visible: scatterBox.checked
                    id: scatterRandomize
                    checked: true
                    text: "randomize"
                }

                EffectBox {
                    text: "SCurveTonemap"
                    effect: SCurveTonemap {
                        // TODO
                        shoulderSlope: 1.0   // 0.0 - 3.0
                        shoulderEmphasis: 0  // -1.0 - 1.0
                        toeSlope: 1.0        // 0.0 - 3.0
                        toeEmphasis: 0       // -1.0 - 1.0
                        contrastBoost: 0     // -1.0 - 2.0
                        saturationLevel: 1   // 0.0 - 2.0
                        gammaValue: 2.2      // 0.1 - 8.0
                        useExposure: false
                        whitePoint: 1.0      // 0.01 - 128.0
                        exposureValue: 1.0   // 0.01 - 16.0
                    }
                }

                EffectBox {
                    id: tiltBox
                    text: "TiltShift"
                    effect: TiltShift {
                         focusPosition: tiltPosition.value
                         focusWidth: tiltWidth.value
                         blurAmount: tiltBlur.value
                         isVertical: tiltVertical.checked
                         isInverted: tiltInverted.checked
                    }
                }
                EffectSlider {
                    visible: tiltBox.checked
                    id: tiltBlur
                    from: 0.0
                    to: 10.0
                    value: 4
                    precision: 1
                    description: "blur amount"
                }
                EffectSlider {
                    visible: tiltBox.checked
                    id: tiltPosition
                    from: 0.0
                    to: 1.0
                    value: 0.5
                    precision: 2
                    description: "focus position"
                }
                EffectSlider {
                    visible: tiltBox.checked
                    id: tiltWidth
                    from: 0.0
                    to: 1.0
                    value: 0.2
                    precision: 2
                    description: "focus width"
                }
                CheckBox {
                    visible: tiltBox.checked
                    id: tiltInverted
                    text: "inverted"
                    checked: false
                }
                CheckBox {
                    visible: tiltBox.checked
                    id: tiltVertical
                    text: "vertical"
                    checked: false
                }

                EffectBox {
                    id: vignetteBox
                    text: "Vignette"
                    effect: Vignette {
                        vignetteColor: vignetteCol.colorVector
                        vignetteRadius: vignetteR.value
                        vignetteStrength: vignetteS.value
                    }
                }
                EffectColor {
                    visible: vignetteBox.checked
                    id: vignetteCol
                    colorVector:  Qt.vector3d(0.5, 0.5, 0.5)
                    description: "color"
                }
                EffectSlider {
                    visible: vignetteBox.checked
                    id: vignetteR
                    from: 0.0
                    to: 5.0
                    value: 0.35
                    precision: 2
                    description: "radius"
                }
                EffectSlider {
                    visible: vignetteBox.checked
                    id: vignetteS
                    from: 0.0
                    to: 15.0
                    value: 15
                    precision: 1
                    description: "strength"
                }

                EffectBox {
                    id: blurBox
                    text: "Blur effect"
                    effect : Blur {
                        amount: blurEffectAmount.value
                    }
                }
                EffectSlider {
                    visible: blurBox.checked
                    id: blurEffectAmount
                    from: 0.0
                    to: 0.01
                    value: 0.003
                    precision: 4
                }

                EffectBox {
                    id: gaussBox
                    text: "Gaussian blur"
                    effect: GaussianBlur {
                        amount: gaussAmount.value
                    }
                }
                EffectSlider {
                    visible: gaussBox.checked
                    id: gaussAmount
                    from: 0.0
                    to: 10.0
                    value: 2
                    precision: 1
                }

                EffectBox {
                    id: fxaaCheckBox
                    text: "FXAA effect"
                    effect : Fxaa {}
                }

                ColumnLayout {
                    id: antialiasingSettings
//                    visible: aaCheckBox.checked

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#909090"
                    }
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        font.bold: true
                        text: "Antialiasing mode"
                    }
                    RadioButton {
                        id: modeButton1
                        checked: true
                        text: qsTr("NoAA")
                    }
                    RadioButton {
                        id: modeButton2
                        text: qsTr("SSAA")
                    }
                    RadioButton {
                        id: modeButton3
                        text: qsTr("MSAA")
                    }
                    RadioButton {
                        id: modeButton4
                        text: qsTr("ProgressiveAA")
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#909090"
                    }
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        font.bold: true
                        text: "Antialiasing quality"
                    }
                    ButtonGroup {
                        buttons: antialiasingQualityColumn.children
                    }
                    ColumnLayout {
                        id: antialiasingQualityColumn
                        RadioButton {
                            id: qualityButton1
                            text: qsTr("Normal")
                        }
                        RadioButton {
                            id: qualityButton2
                            checked: true
                            text: qsTr("High")
                        }
                        RadioButton {
                            id: qualityButton3
                            text: qsTr("VeryHigh")
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#909090"
                    }
                    CheckBox {
                        id: temporalModeButton
                        text: "temporalAAEnabled"
                    }
                    Item { width: 1; height: 10 }
                    Slider {
                        id: temporalStrengthSlider
                        from: 0.0
                        to: 2.0
                        value: 0.3
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.bottom: parent.verticalCenter
                            anchors.bottomMargin: 16
                            text: "temporalAAStrength: " + temporalStrengthSlider.value.toFixed(2);
                            z: 10
                        }
                    }
                }
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#909090"
                }


                function recalcEffects()
                {
                    sceneEnvironment.effects = []
                    for (var i = 0; i < settingsArea.children.length; i++) {
                        let obj = settingsArea.children[i]
                        if (obj.effect) {
//                            console.log("item "+i);
//                            console.log("     obj " + obj);
//                            console.log("   check " + obj.checked)
//                            console.log("  effect " + obj.effect)
                            if (obj.checked)
                                sceneEnvironment.effects.push(obj.effect)
                        }
                    }
                }

            } // ColumnLayout settingsArea
        } // Rectangle contentsRect

        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    } // ScrollView


}
