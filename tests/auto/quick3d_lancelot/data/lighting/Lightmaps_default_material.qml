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
    id: lightmaps_default_material
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
            position: Qt.vector3d(8.92529, -33.3906, 88.9147)
            rotation: Quaternion.fromEulerAngles(-38.717, -127.514, -31.7476)

            Model {
                id: cube
                rotation: Quaternion.fromEulerAngles(-90, 0, 0)
                scale: Qt.vector3d(100, 100, 100)
                source: "models/testCube/meshes/Cube.mesh"
                
                

                DefaultMaterial {
                    id: material
                    lighting: DefaultMaterial.FragmentLighting
                    diffuseColor: Qt.rgba(0.8, 0.8, 0.8, 1)
                    diffuseMap: material_diffusemap
                    indexOfRefraction: 1.5
                    specularAmount: 0
                    specularRoughness: 20
                    bumpAmount: 0.5
                    translucentFalloff: 1
                    lightmapIndirect: material_lightmapindirect
                    lightmapRadiosity: material_lightmapradiosity
                    lightmapShadow: material_lightmapshadow
                    displacementAmount: 20

                    Texture {
                        id: material_diffusemap
                        source: "maps/Cork1.jpg"
                    }

                    Texture {
                        id: material_lightmapindirect
                        source: "maps/cork_lightmap_indirect.jpg"
                    }

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
            id: noShadow
            position: Qt.vector3d(-270.428, 9.34266, 88.9069)
            rotation: Quaternion.fromEulerAngles(-38.717, -127.514, -31.7476)

            Model {
                id: cube_001
                rotation: Quaternion.fromEulerAngles(-90, 0, 0)
                scale: Qt.vector3d(100, 100, 100)
                source: "models/testCube/meshes/Cube.mesh"
                
                

                DefaultMaterial {
                    id: material_001
                    lighting: DefaultMaterial.FragmentLighting
                    diffuseColor: Qt.rgba(0.8, 0.8, 0.8, 1)
                    diffuseMap: material_001_diffusemap
                    indexOfRefraction: 1.5
                    specularAmount: 0
                    specularRoughness: 20
                    bumpAmount: 0.5
                    translucentFalloff: 1
                    lightmapIndirect: material_001_lightmapindirect
                    lightmapRadiosity: material_001_lightmapradiosity
                    displacementAmount: 20

                    Texture {
                        id: material_001_diffusemap
                        source: "maps/Cork1.jpg"
                    }

                    Texture {
                        id: material_001_lightmapindirect
                        source: "maps/cork_lightmap_indirect.jpg"
                    }

                    Texture {
                        id: material_001_lightmapradiosity
                        source: "maps/core_lightmap_radiosity.jpg"
                    }
                }
                materials: [material_001]
            }
        }

        Node {
            id: indirectOnly
            position: Qt.vector3d(266.005, 81.2868, 88.9395)
            rotation: Quaternion.fromEulerAngles(1.05041, 78.4043, -264.953)

            Model {
                id: cube_002
                rotation: Quaternion.fromEulerAngles(-90, 0, 0)
                scale: Qt.vector3d(100, 100, 100)
                source: "models/testCube/meshes/Cube.mesh"
                
                

                DefaultMaterial {
                    id: material_002
                    lighting: DefaultMaterial.FragmentLighting
                    diffuseColor: Qt.rgba(0.8, 0.8, 0.8, 1)
                    diffuseMap: material_002_diffusemap
                    indexOfRefraction: 1.5
                    specularAmount: 0
                    specularRoughness: 20
                    bumpAmount: 0.5
                    translucentFalloff: 1
                    lightmapIndirect: material_002_lightmapindirect
                    displacementAmount: 20

                    Texture {
                        id: material_002_diffusemap
                        source: "maps/Cork1.jpg"
                    }

                    Texture {
                        id: material_002_lightmapindirect
                        source: "maps/cork_lightmap_indirect.jpg"
                    }
                }
                materials: [material_002]
            }
        }

        Node {
            id: radiosityOnly
            position: Qt.vector3d(223.385, -186.454, 88.9396)
            rotation: Quaternion.fromEulerAngles(-38.717, -127.514, -31.7476)

            Model {
                id: cube_003
                rotation: Quaternion.fromEulerAngles(-90, 0, 0)
                scale: Qt.vector3d(100, 100, 100)
                source: "models/testCube/meshes/Cube.mesh"
                
                

                DefaultMaterial {
                    id: material_003
                    lighting: DefaultMaterial.FragmentLighting
                    diffuseColor: Qt.rgba(0.8, 0.8, 0.8, 1)
                    diffuseMap: material_003_diffusemap
                    indexOfRefraction: 1.5
                    specularAmount: 0
                    specularRoughness: 20
                    bumpAmount: 0.5
                    translucentFalloff: 1
                    lightmapRadiosity: material_003_lightmapradiosity
                    displacementAmount: 20

                    Texture {
                        id: material_003_diffusemap
                        source: "maps/Cork1.jpg"
                    }

                    Texture {
                        id: material_003_lightmapradiosity
                        source: "maps/core_lightmap_radiosity.jpg"
                    }
                }
                materials: [material_003]
            }
        }

        Node {
            id: unLit
            position: Qt.vector3d(-131.581, -186.634, 245.316)
            rotation: Quaternion.fromEulerAngles(-38.717, -127.514, -31.7476)

            Model {
                id: cube_004
                rotation: Quaternion.fromEulerAngles(-90, 0, 0)
                scale: Qt.vector3d(100, 100, 100)
                source: "models/testCube/meshes/Cube.mesh"
                
                

                DefaultMaterial {
                    id: material_004
                    lighting: DefaultMaterial.NoLighting
                    diffuseColor: Qt.rgba(0.8, 0.8, 0.8, 1)
                    diffuseMap: material_004_diffusemap
                    indexOfRefraction: 1.5
                    specularAmount: 0
                    specularRoughness: 20
                    bumpAmount: 0.5
                    translucentFalloff: 1
                    displacementAmount: 20

                    Texture {
                        id: material_004_diffusemap
                        source: "maps/Cork1.jpg"
                    }
                }
                materials: [material_004]
            }
        }
    }
}
