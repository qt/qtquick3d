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
import QtQuick3D.Helpers
import QtQuick3D.Effects
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: window
    width: 1280
    height: 720
    visible: true
    title: "Custom Post-Processing Effect Example"
    color: "#848895"

    View3D {
        id: view3d

        //! [eff1]
        Effect {
            id: eff1
            property TextureInput tex: TextureInput {
                id: qtLogo
                texture: Texture { source: "qt_logo_rect.png" }
            }
            passes: Pass {
                shaders: Shader {
                    id: fs1
                    stage: Shader.Fragment
                    shader: "effect.frag"
                }
            }
        }
        //! [eff1]

        //! [eff2]
        Effect {
            id: eff2
            property real uRed: 0.0
            SequentialAnimation on uRed {
                running: radioEff2.checked || radioEff3.checked
                loops: -1
                NumberAnimation { from: 0; to: 1; duration: 2000 }
                NumberAnimation { from: 1; to: 0; duration: 2000 }
            }
            property real uGreen: 1.0
            Shader {
                id: vs2
                stage: Shader.Vertex
                shader: "effect2.vert"
            }
            Shader {
                id: fs2
                stage: Shader.Fragment
                shader: "effect2.frag"
            }
            passes: Pass {
                shaders: [ vs2, fs2 ]
            }
        }
        //! [eff2]

        Effect {
            id: eff3

            property TextureInput tex: qtLogo
            property real uRed: 1.0
            property real uGreen: 0.0
            SequentialAnimation on uGreen {
                running: radioEff4.checked
                loops: -1
                NumberAnimation { from: 0; to: 1; duration: 2000 }
                NumberAnimation { from: 1; to: 0; duration: 2000 }
            }

            Buffer {
                id: intermediateTexture
                name: "intermediateTexture"
                format: Buffer.RGBA8
                textureFilterOperation: Buffer.Linear
                textureCoordOperation: Buffer.ClampToEdge
                sizeMultiplier: 2 // just for fun upscale and then downscale
            }

            passes: [
                Pass {
                    shaders: [ fs1 ]
                    output: intermediateTexture
                },
                Pass {
                    shaders: [ vs2, fs2 ]
                    commands: [
                        BufferInput {
                            buffer: intermediateTexture
                        }
                    ]
                }
            ]
        }

        anchors.fill: parent
        renderMode: View3D.Offscreen

        environment: SceneEnvironment {
            id: env
            clearColor: "skyblue"
            backgroundMode: SceneEnvironment.Color
            effects: [ eff1 ]
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 200, 300)
            eulerRotation.x: -20
        }

        DirectionalLight {
            eulerRotation.x: -20
            eulerRotation.y: 20
            ambientColor: Qt.rgba(0.8, 0.8, 0.8, 1.0);
        }

        Texture {
            id: checkers
            source: "checkers2.png"
            scaleU: 20
            scaleV: 20
            tilingModeHorizontal: Texture.Repeat
            tilingModeVertical: Texture.Repeat
        }

        Model {
            source: "#Rectangle"
            scale.x: 10
            scale.y: 10
            eulerRotation.x: -90
            materials: [ DefaultMaterial { diffuseMap: checkers } ]
        }

        Model {
            source: "#Cone"
            position: Qt.vector3d(100, 0, -200)
            scale.y: 3
            materials: [ DefaultMaterial { diffuseColor: "green" } ]
        }

        Model {
            id: sphere
            source: "#Sphere"
            position: Qt.vector3d(-100, 200, -200)
            materials: [ DefaultMaterial { diffuseColor: "#808000" } ]
        }

        Model {
            source: "#Cube"
            position.y: 50
            eulerRotation.y: 20
            materials: [ DefaultMaterial { diffuseColor: "gray" } ]
        }
    }

    WasdController {
        controlledObject: camera
    }

    ColumnLayout {
        Label {
            text: "Use WASD and mouse to navigate"
            font.bold: true
        }
        ButtonGroup {
            buttons: [ radioEff1, radioEff2, radioEff3, radioEff4, radioEff5 ]
        }
        RadioButton {
            id: radioEff1
            text: "Custom effect with fragment shader only"
            checked: true
            focusPolicy: Qt.NoFocus
            onCheckedChanged: {
                if (checked)
                    env.effects = [ eff1 ];
            }
        }
        RadioButton {
            id: radioEff2
            text: "Custom effect with vertex and fragment shaders"
            checked: false
            focusPolicy: Qt.NoFocus
            onCheckedChanged: {
                if (checked)
                    env.effects = [ eff2 ];
            }
        }
        RadioButton {
            id: radioEff3
            text: "Both effects chained"
            checked: false
            focusPolicy: Qt.NoFocus
            onCheckedChanged: {
                if (checked)
                    env.effects = [ eff1, eff2 ];
            }
        }
        RadioButton {
            id: radioEff4
            text: "As one single, multi-pass effect"
            checked: false
            focusPolicy: Qt.NoFocus
            onCheckedChanged: {
                if (checked)
                    env.effects = [ eff3 ];
            }
        }
        RadioButton {
            id: radioEff5
            text: "No effects"
            checked: false
            focusPolicy: Qt.NoFocus
            onCheckedChanged: {
                if (checked)
                    env.effects = [];
            }
        }
    }
}
