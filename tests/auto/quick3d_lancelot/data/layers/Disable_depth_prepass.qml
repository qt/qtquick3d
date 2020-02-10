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
    id: disable_depth_prepass
    width: 800
    height: 480
    color: Qt.rgba(1, 1, 1, 1)

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
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 0, 600)
            clipFar: 5000
        }

        DirectionalLight {
            id: light
            shadowFactor: 10
        }

        Model {
            id: sphere
            position: Qt.vector3d(-354.989, 135.238, 0)
            source: "#Sphere"
            
            

            DefaultMaterial {
                id: default_
                lighting: DefaultMaterial.FragmentLighting
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
            id: cone
            position: Qt.vector3d(-365.912, -248.222, 0)
            scale: Qt.vector3d(2.89542, 3.13161, 1)
            source: "#Cone"
            
            

            DefaultMaterial {
                id: default_002
                lighting: DefaultMaterial.FragmentLighting
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
            id: cube
            position: Qt.vector3d(349.297, -228.053, 0)
            rotation: Quaternion.fromEulerAngles(28.0299, 33.3145, 17.1637)
            scale: Qt.vector3d(2.00606, 1, 1)
            source: "#Cube"
            
            

            DefaultMaterial {
                id: default_003
                lighting: DefaultMaterial.FragmentLighting
                indexOfRefraction: 1.5
                specularAmount: 0
                specularRoughness: 0
                bumpAmount: 0.5
                translucentFalloff: 1
                displacementAmount: 20
            }
            materials: [default_003]
        }

        Node {
            id: barrel
            position: Qt.vector3d(-292.216, -304.023, -434)
            rotation: Quaternion.fromEulerAngles(0, 0, -41.5)
            scale: Qt.vector3d(10, 10, 10)

            Model {
                id: barrel_1
                rotation: Quaternion.fromEulerAngles(-90, 0, 0)
                scale: Qt.vector3d(100, 100, 100)
                source: "../shared/models/barrel/meshes/Barrel.mesh"
                
                

                DefaultMaterial {
                    id: barrel_001
                    lighting: DefaultMaterial.FragmentLighting
                    diffuseColor: Qt.rgba(0.639994, 0.639994, 0.639994, 1)
                    indexOfRefraction: 1.5
                    specularAmount: 0
                    specularRoughness: 9.607839584350586
                    bumpAmount: 0.5
                    translucentFalloff: 1
                    displacementAmount: 20
                }
                materials: [barrel_001]
            }
        }

        Model {
            id: cylinder
            position: Qt.vector3d(255.743, -27.1591, 185)
            scale: Qt.vector3d(1.5, 1.5, 1.5)
            source: "#Cylinder"
            
            

            DefaultMaterial {
                id: default_001
                lighting: DefaultMaterial.FragmentLighting
                indexOfRefraction: 1.5
                specularAmount: 0
                specularRoughness: 0
                opacity: 0.76
                bumpAmount: 0.5
                translucentFalloff: 1
                displacementAmount: 20
            }
            materials: [default_001]
        }
    }
}
