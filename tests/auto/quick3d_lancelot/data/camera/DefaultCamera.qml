/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tests of the Qt Toolkit.
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

import QtQuick3D 1.15
import QtQuick 2.15

Rectangle {
    id: defaultCamera
    width: 800
    height: 480
    color: Qt.rgba(0, 0, 0, 1)

    View3D {
        id: layer
        anchors.left: parent.left
        anchors.leftMargin: parent.width * 0
        width: parent.width * 1
        anchors.top: parent.top
        anchors.topMargin: parent.height * 0
        height: parent.height * 1
        environment: SceneEnvironment {
            clearColor: Qt.rgba(0, 0, 0, 1)
            aoDither: true
            depthPrePassEnabled: true
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 0, 600)
            clipFar: 5000
        }

        DirectionalLight {
            id: light
            color: Qt.rgba(1, 1, 0.964706, 1)
            ambientColor: Qt.rgba(0.168627, 0.164706, 0.160784, 1)
            shadowFactor: 10
        }

        Node {
            id: arrowUp
            position: Qt.vector3d(-393.171, 121.883, 0)

            Model {
                id: cone
                source: "#Cone"
                
                

                DefaultMaterial {
                    id: default_
                    lighting: DefaultMaterial.FragmentLighting
                    diffuseColor: Qt.rgba(0, 1, 0, 1)
                    indexOfRefraction: 1.5
                    specularAmount: 0
                    specularRoughness: 0
                    bumpAmount: 0.5
                    translucentFalloff: 1
                    displacementAmount: 20
                }
                materials: [default_]
            }

            Model {
                id: cylinder
                position: Qt.vector3d(0, -97.5005, 0)
                scale: Qt.vector3d(0.5, 2, 0.5)
                source: "#Cylinder"
                
                

                DefaultMaterial {
                    id: default_001
                    lighting: DefaultMaterial.FragmentLighting
                    diffuseColor: Qt.rgba(0, 1, 0, 1)
                    indexOfRefraction: 1.5
                    specularAmount: 0
                    specularRoughness: 0
                    bumpAmount: 0.5
                    translucentFalloff: 1
                    displacementAmount: 20
                }
                materials: [default_001]
            }

            PerspectiveCamera {
                id: camera_001
                position: Qt.vector3d(0, 98.234, 0)
                rotation: Quaternion.fromEulerAngles(26, 90, 0)
                clipFar: 5000
            }
        }

        Node {
            id: arrowForward
            position: Qt.vector3d(-138.558, 0, 0)
            rotation: Quaternion.fromEulerAngles(-90, 0, 0)

            Model {
                id: cone_001
                source: "#Cone"
                
                

                DefaultMaterial {
                    id: default_002
                    lighting: DefaultMaterial.FragmentLighting
                    diffuseColor: Qt.rgba(1, 0, 0, 1)
                    indexOfRefraction: 1.5
                    specularAmount: 0
                    specularRoughness: 0
                    bumpAmount: 0.5
                    translucentFalloff: 1
                    displacementAmount: 20
                }
                materials: [default_002]
            }

            Model {
                id: cylinder_001
                position: Qt.vector3d(0, -97.5005, 0)
                scale: Qt.vector3d(0.5, 2, 0.5)
                source: "#Cylinder"
                
                

                DefaultMaterial {
                    id: default_003
                    lighting: DefaultMaterial.FragmentLighting
                    diffuseColor: Qt.rgba(1, 0, 0, 1)
                    indexOfRefraction: 1.5
                    specularAmount: 0
                    specularRoughness: 0
                    bumpAmount: 0.5
                    translucentFalloff: 1
                    displacementAmount: 20
                }
                materials: [default_003]
            }
        }

        Node {
            id: arrowDown
            position: Qt.vector3d(91.1513, 0, 0)
            rotation: Quaternion.fromEulerAngles(-180, 0, 0)

            Model {
                id: cone_002
                source: "#Cone"
                
                

                DefaultMaterial {
                    id: default_004
                    lighting: DefaultMaterial.FragmentLighting
                    diffuseColor: Qt.rgba(0.333333, 1, 1, 1)
                    indexOfRefraction: 1.5
                    specularAmount: 0
                    specularRoughness: 0
                    bumpAmount: 0.5
                    translucentFalloff: 1
                    displacementAmount: 20
                }
                materials: [default_004]
            }

            Model {
                id: cylinder_002
                position: Qt.vector3d(0, -97.5005, 0)
                scale: Qt.vector3d(0.5, 2, 0.5)
                source: "#Cylinder"
                
                

                DefaultMaterial {
                    id: default_005
                    lighting: DefaultMaterial.FragmentLighting
                    diffuseColor: Qt.rgba(0.333333, 1, 1, 1)
                    indexOfRefraction: 1.5
                    specularAmount: 0
                    specularRoughness: 0
                    bumpAmount: 0.5
                    translucentFalloff: 1
                    displacementAmount: 20
                }
                materials: [default_005]
            }
        }

        Node {
            id: arrowBackwards
            position: Qt.vector3d(312.117, 0, 0)
            rotation: Quaternion.fromEulerAngles(90, 0, 0)

            Model {
                id: cone_003
                source: "#Cone"
                
                

                DefaultMaterial {
                    id: default_006
                    lighting: DefaultMaterial.FragmentLighting
                    indexOfRefraction: 1.5
                    specularAmount: 0
                    specularRoughness: 0
                    bumpAmount: 0.5
                    translucentFalloff: 1
                    displacementAmount: 20
                }
                materials: [default_006]
            }

            Model {
                id: cylinder_003
                position: Qt.vector3d(0, -97.5005, 0)
                scale: Qt.vector3d(0.5, 2, 0.5)
                source: "#Cylinder"
                
                

                DefaultMaterial {
                    id: default_007
                    lighting: DefaultMaterial.FragmentLighting
                    indexOfRefraction: 1.5
                    specularAmount: 0
                    specularRoughness: 0
                    bumpAmount: 0.5
                    translucentFalloff: 1
                    displacementAmount: 20
                }
                materials: [default_007]
            }
        }

        DirectionalLight {
            id: light2
            rotation: Quaternion.fromEulerAngles(-180, -90, 0)
            color: Qt.rgba(1, 0.988235, 0.882353, 1)
            shadowFactor: 10
        }
    }
}
