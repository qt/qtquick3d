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

import "./materials" as Materials

Rectangle {
    id: lightmaps_referenced_material
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

        Node {
            id: testCube2
            position: Qt.vector3d(-189.977, 8.13851, 88.9147)
            rotation: Quaternion.fromEulerAngles(-38.717, -127.514, -31.7476)

            Model {
                id: cube
                rotation: Quaternion.fromEulerAngles(-90, 0, 0)
                scale: Qt.vector3d(100, 100, 100)
                source: "models/testCube/meshes/Cube.mesh"
                
                

                DefaultMaterial {
                    id: material
                    lighting: DefaultMaterial.FragmentLighting
                    indexOfRefraction: 1.5
                    specularAmount: 0
                    specularRoughness: 0
                    bumpAmount: 0.5
                    translucentFalloff: 1
                    lightmapRadiosity: material_lightmapradiosity
                    lightmapShadow: material_lightmapshadow
                    displacementAmount: 20

                    Texture {
                        id: material_lightmapradiosity
                        source: "maps/core_lightmap_radiosity.jpg"
                    }

                    Texture {
                        id: material_lightmapshadow
                        source: "maps/core_lightmap_shadow.jpg"
                    }
                }
                materials: [material]
            }
        }

        Node {
            id: everything
            position: Qt.vector3d(157.556, -3.88307, 88.9147)
            rotation: Quaternion.fromEulerAngles(-38.717, -127.514, -31.7476)

            Model {
                id: cube_001
                rotation: Quaternion.fromEulerAngles(-90, 0, 0)
                scale: Qt.vector3d(100, 100, 100)
                source: "models/testCube/meshes/Cube.mesh"
                
                

                Materials.Material {
                    id: material_001
                    lightmapIndirect: material_001_lightmapindirect
                    lightmapRadiosity: material_001_lightmapradiosity
                    lightmapShadow: material_001_lightmapshadow

                    Texture {
                        id: material_001_lightmapradiosity
                        source: "maps/core_lightmap_radiosity.jpg"
                    }

                    Texture {
                        id: material_001_lightmapshadow
                        source: "maps/core_lightmap_shadow.jpg"
                    }

                    Texture {
                        id: material_001_lightmapindirect
                        source: "maps/cork_lightmap_indirect.jpg"
                    }
                }
                materials: [material_001]
            }
        }
    }
}
