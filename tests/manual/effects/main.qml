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
    color: "black"

    View3D {
        id: view3D
        property real animationValue: 0.0
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
            duration: 4000
        }

        PerspectiveCamera {
            z: 500
        }

        DirectionalLight {
            eulerRotation.x: -30
            brightness: 200
        }



        environment: SceneEnvironment {
            id: sceneEnvironment
            clearColor: "#f0f0f0"
            backgroundMode: SceneEnvironment.Color

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
                x: 200 * Math.sin(view3D.animationValue/180 * Math.PI)
                y: 200 * Math.cos(view3D.animationValue/180 * Math.PI)
                eulerRotation.y: 5
                eulerRotation.x: 5
                scale: Qt.vector3d(0.75, 0.75, 0.75)
                materials: DefaultMaterial {
                    diffuseMap: textTexture
                }
            }

            Model {
                source: "#Sphere"
                x: 80
                y: -40
                z: 160
                scale: Qt.vector3d(1.5, 1.5, 1.5)
                materials: DefaultMaterial {
                    diffuseColor: "#2abe25"
                }
            }
        }
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

                Button {
                    id: animationButton
                    Layout.alignment: Qt.AlignHCenter
                    text: "Animate!"
                    checkable: true
                }

                EffectBox {
                    text: "AdditiveColorGradient"
                    effect: AdditiveColorGradient {}
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
                }
                EffectSlider {
                    visible: brushBox.checked
                    id: brushStrokesSize
                    from: 10.0
                    to: 200.0
                    value: 100
                    precision: 0
                }

                EffectBox {
                    text: "ChromaticAberration"
                    effect: ChromaticAberration {}
                }

                EffectBox {
                    text: "ColorMaster"
                    effect: ColorMaster {}
                }

                EffectBox {
                    text: "DepthOfFieldHQBlur"
                    effect: DepthOfFieldHQBlur {}
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
                    text: "DistortionRipple"
                    effect: DistortionRipple {}
                }

                EffectBox {
                    text: "DistortionSphere"
                    effect: DistortionSphere {}
                }

                EffectBox {
                    text: "DistortionSpiral"
                    effect: DistortionSpiral {}
                }

                EffectBox {
                    text: "EdgeDetect"
                    effect: EdgeDetect {}
                }


                EffectBox {
                    text: "Emboss"
                    effect: Emboss {}
                }

                EffectBox {
                    text: "Flip"
                    effect: Flip {}
                }

                EffectBox {
                    text: "HDRBloomTonemap"
                    effect: HDRBloomTonemap {}
                }

                EffectBox {
                    text: "Scatter"
                    effect: Scatter {}
                }

                EffectBox {
                    text: "SCurveTonemap"
                    effect: SCurveTonemap {}
                }

                EffectBox {
                    text: "TiltShift"
                    effect: TiltShift {}
                }

                EffectBox {
                    text: "Vignette"
                    effect: Vignette {}
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
