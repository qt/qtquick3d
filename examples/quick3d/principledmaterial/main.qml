/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
import QtQuick3D.Effects
import QtQuick3D.Helpers
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: window
    width: 1280
    height: 720
    visible: true
    title: "Principled Materials Example"
    //color: "black"
    color: "#848895"

    Image {
        anchors.fill: parent
        source: "maps/grid.png"
        fillMode: Image.Tile
        horizontalAlignment: Image.AlignLeft
        verticalAlignment: Image.AlignTop
    }

    SplitView {
        id: splitView
        anchors.fill: parent
        Page {
            id: toolPage
            SplitView.fillHeight: true
            SplitView.preferredWidth: 420
            SplitView.minimumWidth: 300
            header: TabBar {
               id: tabBar
               TabButton {
                   text: "Basics"
               }
               TabButton {
                   text: "Alpha"
               }
               TabButton {
                   text: "Details"
               }
               TabButton {
                   text: "Clearcoat"
               }
               TabButton {
                   text: "Refraction"
               }
               TabButton {
                   text: "Special"
               }
            }

            StackLayout {
                id: toolPageSwipeView
                anchors.fill: parent
                anchors.margins: 10
                currentIndex: tabBar.currentIndex

                BasicsPane {
                    targetMaterial: basicMaterial
                }

                AlphaPane {
                    targetMaterial: basicMaterial
                }

                DetailsPane {
                    targetMaterial: basicMaterial
                    view3D: viewport
                }

                ClearcoatPane {
                    targetMaterial: basicMaterial
                }

                RefractionPane {
                    targetMaterial: basicMaterial
                }

                SpecialPane {
                    targetMaterial: basicMaterial
                    linesModel: linesLogo
                    pointsModel: pointsLogo
                }
            }
        }

        View3D {
            id: viewport
            SplitView.fillHeight: true
            SplitView.fillWidth: true
            SplitView.minimumWidth: splitView.width * 0.5
            environment: SceneEnvironment {
                property bool enableEffects: false
                antialiasingMode: SceneEnvironment.MSAA
                antialiasingQuality: SceneEnvironment.High
                lightProbe: Texture {
                    source: "maps/OpenfootageNET_garage-1024.hdr"
                }
                effects: enableEffects ? [bloom, scurveTonemap] : []
                backgroundMode: SceneEnvironment.SkyBox

                SCurveTonemap {
                    id: scurveTonemap
                }
                HDRBloomTonemap {
                    id: bloom
                }
            }
            Node {
                id: originNode
                PerspectiveCamera {
                    id: cameraNode
                    z: 600
                    clipNear: 1
                    clipFar: 10000
                }
            }

            PrincipledMaterial {
                id: basicMaterial
                baseColor: "red"
            }

            Model {
                id: cube
                source: "#Cube"
                materials: basicMaterial
                pickable: true
            }

            Model {
                id: sphereModel
                x: -200
                scale: Qt.vector3d(1.5, 1.5, 1.5)
                source: "#Sphere"
                materials: basicMaterial
                pickable: true
            }

            Model {
                id: monkeyMesh
                x: 250
                source: "meshes/suzanne.mesh"
                materials: basicMaterial
                pickable: true
            }

            Model {
                id: linesLogo
                y: 200
                x: -100
                source: "meshes/logo_lines.mesh"
                materials: basicMaterial
                pickable: true
                visible: false
            }

            Model {
                id: pointsLogo
                y: 200
                x: 100
                source: "meshes/logo_points.mesh"
                materials: basicMaterial
                pickable: true
                visible: false
            }
            //! [resource loader]
            ResourceLoader {
                meshSources: [
                    "meshes/logo_lines.mesh",
                    "meshes/logo_points.mesh"
                ]
            }
            //! [resource loader]

            BackgroundCurtain {
                visible: curtainToggleButton.checked
                y: -150
            }

            OrbitCameraController {
                origin: originNode
                camera: cameraNode
            }

            MouseArea {
                id: pickController
                anchors.fill: parent
                onClicked: function(mouse) {
                    let pickResult = viewport.pick(mouse.x, mouse.y);
                    if (pickResult.objectHit) {
                        let pickedObject = pickResult.objectHit;
                        // Move the camera orbit origin to be the clicked object
                        originNode.position = pickedObject.position
                    }
                }
            }

            Button {
                id: curtainToggleButton
                text: checked ? "Hide Curtain" : "Show Curtain"
                checkable: true
                checked: true
                anchors.top: parent.top
                anchors.right: parent.right
            }

            RowLayout {
                anchors.bottom: parent.bottom
                Label {
                    text: "Drag Background to Orbit Camera, Touch/Click a Model to Center Camera"
                    color: "#dddddd"
                }
            }
        }
    }
}
