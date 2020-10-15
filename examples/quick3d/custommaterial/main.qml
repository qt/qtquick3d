/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick
import QtQuick3D
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: window
    width: 1280
    height: 720
    visible: true
    title: "Custom Materials Example"

    MouseArea {
        anchors.fill: parent
        hoverEnabled: screenSphere.visible
        onPositionChanged: {
            screenSphere.x = (mouse.x / window.width * 2.0 - 1.0) * 200
            screenSphere.y = (mouse.y / window.height * 2.0 - 1.0) * -200
        }
    }

    View3D {
        id: v3d
        anchors.fill: parent

        camera: camera

        environment: probeCb.checked ? probeEnv : env

        SceneEnvironment {
            id: env
            clearColor: "#444845"
            backgroundMode: SceneEnvironment.Color
        }

        SceneEnvironment {
            id: probeEnv
            clearColor: "#444845"
            backgroundMode: SceneEnvironment.Color
            probeExposure: 10
            lightProbe: Texture {
                source: "maps/OpenfootageNET_garage-1024.hdr"
            }
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 0, 600)
        }

        DirectionalLight {
            position: Qt.vector3d(-500, 500, -100)
            color: Qt.rgba(0.2, 0.2, 0.2, 1.0)
            ambientColor: Qt.rgba(0.1, 0.1, 0.1, 1.0)
            scope: probeCb.checked ? dummy : null
        }

        PointLight {
            position: Qt.vector3d(0, 500, 0)
            color: Qt.rgba(0.1, 1.0, 0.1, 1.0)
            ambientColor: Qt.rgba(0.2, 0.2, 0.2, 1.0)
            brightness: 5
            castsShadow: true
            shadowMapQuality: Light.ShadowMapQualityHigh
            scope: probeCb.checked ? dummy : null
        }

        Node {
            id: dummy
        }

        property url fragShaderSrc: probeCb.checked ? "material_lightprobe.frag" : (builtinSpecularCb.checked ? "material_builtinspecular.frag" : "material.frag")

        Model {
            source: "#Rectangle"
            y: -200
            scale: Qt.vector3d(5, 5, 5)
            eulerRotation.x: -90
            materials: [
                CustomMaterial {
                    shadingMode: CustomMaterial.Shaded
                    vertexShader: "material.vert"
                    fragmentShader: v3d.fragShaderSrc
                    property real uTime: 0.0
                    property real uAmplitude: 0.0
                    property color uDiffuse: "white"
                    property real uShininess: 50
                }
            ]
        }

        WeirdShape {
            customMaterial: CustomMaterial {
                shadingMode: CustomMaterial.Shaded
                vertexShader: "material.vert"
                fragmentShader: v3d.fragShaderSrc
                property real uTime: 0.0
                property real uAmplitude: 0.0
                property color uDiffuse: "purple"
                property real uShininess: 50
            }
            position: Qt.vector3d(150, 150, -100)
        }

        Model {
            position: Qt.vector3d(-100, 0, -50)
            eulerRotation.x: 30
            NumberAnimation on eulerRotation.y {
                from: 0; to: 360; duration: 5000; loops: -1
            }
            scale: Qt.vector3d(1.5, 1.5, 1.5)
            source: "#Cylinder"
            materials: [
                CustomMaterial {
                    shadingMode: CustomMaterial.Shaded
                    vertexShader: "material.vert"
                    fragmentShader: v3d.fragShaderSrc
                    property real uTime: 0.0
                    property real uAmplitude: 0.0
                    property color uDiffuse: "yellow"
                    property real uShininess: 50
                    SequentialAnimation on uAmplitude {
                        loops: -1
                        NumberAnimation { from: 0.0; to: 200.0; duration: 5000 }
                        NumberAnimation { from: 200.0; to: 0.0; duration: 5000 }
                    }
                }
            ]
        }

        Model {
            id: screenSphere
            source: "#Sphere"
            visible: screenTexCb.checked
            scale: Qt.vector3d(4, 4, 4)
            z: 100
            materials: [
                CustomMaterial {
                    shadingMode: CustomMaterial.Shaded
                    sourceBlend: uKeepAlpha ? CustomMaterial.SrcAlpha : CustomMaterial.NoBlend
                    destinationBlend: uKeepAlpha ? CustomMaterial.OneMinusSrcAlpha : CustomMaterial.NoBlend
                    fragmentShader: "screen.frag"
                    property bool uKeepAlpha: screenTexAlphaCb.checked
                }
            ]
        }
    }

    Rectangle {
        color: "lightGray"
        width: controls.implicitWidth
        height: controls.implicitHeight
        ColumnLayout {
            id: controls
            RadioButton {
                checked: true
                text: "Lights, custom diffuse and specular"
            }
            RadioButton {
                id: builtinSpecularCb
                checked: false
                text: "Light, custom diffuse, built-in specular"
            }
            RadioButton {
                id: probeCb
                checked: false
                text: "Light probe, metalness (disables directional/point lights)"
                onCheckedChanged: if (checked) screenTexCb.checked = false
            }
            RowLayout {
                CheckBox {
                    id: screenTexCb
                    enabled: !probeCb.checked
                    checked: false
                    text: "Emboss Sphere"
                }
                CheckBox {
                    id: screenTexAlphaCb
                    checked: false
                    text: "Keep alpha"
                }
            }
        }
    }
}
