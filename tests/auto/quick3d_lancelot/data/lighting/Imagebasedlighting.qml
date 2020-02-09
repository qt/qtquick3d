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
import QtQuick3D.Materials 1.15

import "./materials" as Materials

Rectangle {
    id: imagebasedlighting
    width: 800
    height: 480
    color: Qt.rgba(0, 0, 0, 1)

    View3D {
        id: layer
        anchors.left: parent.left
        anchors.leftMargin: parent.width * 0
        width: parent.width * 0.5
        anchors.top: parent.top
        anchors.topMargin: parent.height * 0
        height: parent.height * 1
        environment: SceneEnvironment {
            clearColor: Qt.rgba(0, 0, 0, 1)
            aoDither: true
            depthPrePassEnabled: true
            lightProbe: layer_lightprobe
            probeBrightness: 1000
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 0, 1000)
            clipFar: 5000
            fieldOfViewOrientation: PerspectiveCamera.Horizontal
        }

        Texture {
            id: layer_lightprobe
            source: "maps/OpenfootageNET_fieldairport-512.hdr"
            mappingMode: Texture.LightProbe
            tilingModeHorizontal: Texture.Repeat
        }

        Node {
            id: group
            position: Qt.vector3d(468.305, -129.677, 0)
            scale: Qt.vector3d(2, 2, 2)

            Model {
                id: cylinder
                position: Qt.vector3d(-448.399, 78.869, 0)
                source: "#Cylinder"
                
                

                Materials.Default_002 {
                    id: default_
                }
                materials: [default_]
            }

            Model {
                id: cone
                position: Qt.vector3d(-452.899, -147.545, 0)
                source: "#Cone"
                
                

                CopperMaterial {
                    id: default_001
                }
                materials: [default_001]
            }

            Model {
                id: sphere
                position: Qt.vector3d(-441.352, 255.317, 0)
                source: "#Sphere"
                
                

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
                id: defaultOverride
                position: Qt.vector3d(-233.489, 252.353, 0)
                source: "#Sphere"
                
                

                DefaultMaterial {
                    id: default_003
                    lighting: DefaultMaterial.FragmentLighting
                    indexOfRefraction: 1.5
                    specularAmount: 0
                    specularRoughness: 0
                    bumpAmount: 0.5
                    translucentFalloff: 1
                    lightProbe: default_003_lightProbe
                    displacementAmount: 20

                    Texture {
                        id: default_003_lightProbe
                        source: "maps/OpenfootageNET_lowerAustria01-512.hdr"
                        mappingMode: Texture.LightProbe
                        tilingModeHorizontal: Texture.Repeat
                    }
                }
                materials: [default_003]
            }

            Model {
                id: referenceMaterialOverride
                position: Qt.vector3d(-235.717, 78.869, 0)
                source: "#Cylinder"
                
                

                Materials.Default_002 {
                    id: default_004
                    lightProbe: default_004_lightProbe

                    Texture {
                        id: default_004_lightProbe
                        source: "maps/OpenfootageNET_lowerAustria01-512.hdr"
                        mappingMode: Texture.LightProbe
                        tilingModeHorizontal: Texture.Repeat
                    }
                }
                materials: [default_004]
            }

            Model {
                id: customMaterialOverride
                position: Qt.vector3d(-245.686, -147.545, 0)
                source: "#Cone"
                
                

                CopperMaterial {
                    id: default_005
                    lightProbe: default_005_lightProbe

                    Texture {
                        id: default_005_lightProbe
                        source: "maps/OpenfootageNET_lowerAustria01-512.hdr"
                        mappingMode: Texture.LightProbe
                        tilingModeHorizontal: Texture.Repeat
                    }
                }
                materials: [default_005]
            }

            Model {
                id: defaultMaterialSpecial
                position: Qt.vector3d(-18.5029, 255.317, 0)
                source: "#Sphere"
                
                

                DefaultMaterial {
                    id: default_006
                    lighting: DefaultMaterial.FragmentLighting
                    indexOfRefraction: 1.5
                    specularAmount: 0
                    specularRoughness: 0
                    bumpAmount: 0.5
                    translucentFalloff: 1
                    lightProbe: default_006_lightProbe
                    displacementAmount: 20

                    Texture {
                        id: default_006_lightProbe
                        source: "maps/OpenfootageNET_lowerAustria01-512.hdr"
                        scaleU: 20
                        scaleV: 0
                        mappingMode: Texture.LightProbe
                        tilingModeHorizontal: Texture.Repeat
                    }
                }
                materials: [default_006]
            }

            Model {
                id: referenceMaterialSpecial
                position: Qt.vector3d(-14.5576, 78.869, 0)
                source: "#Cylinder"
                
                

                Materials.Default_002 {
                    id: default_007
                    lightProbe: default_007_lightProbe

                    Texture {
                        id: default_007_lightProbe
                        source: "maps/OpenfootageNET_lowerAustria01-512.hdr"
                        scaleU: 20
                        mappingMode: Texture.LightProbe
                        tilingModeHorizontal: Texture.Repeat
                    }
                }
                materials: [default_007]
            }

            Model {
                id: customMaterialSpecial
                position: Qt.vector3d(-17.4744, -147.545, 0)
                source: "#Cone"
                
                

                CopperMaterial {
                    id: copper_001
                    lightProbe: copper_001_lightProbe

                    Texture {
                        id: copper_001_lightProbe
                        source: "maps/OpenfootageNET_lowerAustria01-512.hdr"
                        scaleU: 20
                        mappingMode: Texture.LightProbe
                        tilingModeHorizontal: Texture.Repeat
                    }
                }
                materials: [copper_001]
            }
        }
    }

    View3D {
        id: oneLightProbe
        anchors.left: parent.left
        anchors.leftMargin: parent.width * 0.5
        width: parent.width * 0.5
        anchors.top: parent.top
        anchors.topMargin: parent.height * 0
        height: parent.height * 1
        environment: SceneEnvironment {
            clearColor: Qt.rgba(0, 0, 0, 1)
            aoDither: true
            depthPrePassEnabled: true
            lightProbe: oneLightProbe_lightprobe
            probeBrightness: 1000
        }

        PerspectiveCamera {
            id: camera_001
            position: Qt.vector3d(0, 0, 1000)
            clipFar: 5000
            fieldOfViewOrientation: PerspectiveCamera.Horizontal
        }

        Texture {
            id: oneLightProbe_lightprobe
            source: "maps/OpenfootageNET_fieldairport-512.hdr"
            mappingMode: Texture.LightProbe
            tilingModeHorizontal: Texture.Repeat
        }

        Node {
            id: group_001
            position: Qt.vector3d(468.305, -129.677, 0)
            scale: Qt.vector3d(2, 2, 2)

            Model {
                id: referenceMaterial
                position: Qt.vector3d(-448.399, 78.869, 0)
                source: "#Cylinder"
                
                

                Materials.Default_009 {
                    id: default_008
                }
                materials: [default_008]
            }

            Model {
                id: customMaterial
                position: Qt.vector3d(-452.899, -147.545, 0)
                source: "#Cone"
                
                

                CopperMaterial {
                    id: copper_002
                }
                materials: [copper_002]
            }

            Model {
                id: defaultMaterial
                position: Qt.vector3d(-441.352, 255.317, 0)
                source: "#Sphere"
                
                

                DefaultMaterial {
                    id: default_009
                    lighting: DefaultMaterial.FragmentLighting
                    indexOfRefraction: 1.5
                    specularAmount: 0
                    specularRoughness: 0
                    bumpAmount: 0.5
                    translucentFalloff: 1
                    displacementAmount: 20
                }
                materials: [default_009]
            }

            Model {
                id: defaultOverride_001
                position: Qt.vector3d(-233.489, 252.353, 0)
                source: "#Sphere"
                
                

                DefaultMaterial {
                    id: default_010
                    lighting: DefaultMaterial.FragmentLighting
                    indexOfRefraction: 1.5
                    specularAmount: 0
                    specularRoughness: 0
                    bumpAmount: 0.5
                    translucentFalloff: 1
                    lightProbe: default_010_lightProbe
                    displacementAmount: 20

                    Texture {
                        id: default_010_lightProbe
                        source: "maps/OpenfootageNET_lowerAustria01-512.hdr"
                        mappingMode: Texture.LightProbe
                        tilingModeHorizontal: Texture.Repeat
                    }
                }
                materials: [default_010]
            }

            Model {
                id: referenceMaterialOverride_001
                position: Qt.vector3d(-235.717, 78.869, 0)
                source: "#Cylinder"
                
                

                Materials.Default_009 {
                    id: default_011
                    lightProbe: default_011_lightProbe

                    Texture {
                        id: default_011_lightProbe
                        source: "maps/OpenfootageNET_lowerAustria01-512.hdr"
                        mappingMode: Texture.LightProbe
                        tilingModeHorizontal: Texture.Repeat
                    }
                }
                materials: [default_011]
            }

            Model {
                id: customMaterialOverride_001
                position: Qt.vector3d(-245.686, -147.545, 0)
                source: "#Cone"
                
                

                CopperMaterial {
                    id: copper_003
                    lightProbe: copper_003_lightProbe

                    Texture {
                        id: copper_003_lightProbe
                        source: "maps/OpenfootageNET_lowerAustria01-512.hdr"
                        mappingMode: Texture.LightProbe
                        tilingModeHorizontal: Texture.Repeat
                    }
                }
                materials: [copper_003]
            }

            Model {
                id: defaultMaterialSpecial_001
                position: Qt.vector3d(-18.5029, 255.317, 0)
                source: "#Sphere"
                
                

                DefaultMaterial {
                    id: default_012
                    lighting: DefaultMaterial.FragmentLighting
                    indexOfRefraction: 1.5
                    specularAmount: 0
                    specularRoughness: 0
                    bumpAmount: 0.5
                    translucentFalloff: 1
                    lightProbe: default_012_lightProbe
                    displacementAmount: 20

                    Texture {
                        id: default_012_lightProbe
                        source: "maps/OpenfootageNET_lowerAustria01-512.hdr"
                        scaleU: 20
                        scaleV: 0
                        mappingMode: Texture.LightProbe
                        tilingModeHorizontal: Texture.Repeat
                    }
                }
                materials: [default_012]
            }

            Model {
                id: referenceMaterialSpecial_001
                position: Qt.vector3d(-14.5576, 78.869, 0)
                source: "#Cylinder"
                
                

                Materials.Default_009 {
                    id: default_013
                    lightProbe: default_013_lightProbe

                    Texture {
                        id: default_013_lightProbe
                        source: "maps/OpenfootageNET_lowerAustria01-512.hdr"
                        scaleU: 20
                        mappingMode: Texture.LightProbe
                        tilingModeHorizontal: Texture.Repeat
                    }
                }
                materials: [default_013]
            }

            Model {
                id: customMaterialSpecial_001
                position: Qt.vector3d(-17.4744, -147.545, 0)
                source: "#Cone"
                
                

                CopperMaterial {
                    id: copper_004
                    lightProbe: copper_004_lightProbe

                    Texture {
                        id: copper_004_lightProbe
                        source: "maps/OpenfootageNET_lowerAustria01-512.hdr"
                        scaleU: 20
                        mappingMode: Texture.LightProbe
                        tilingModeHorizontal: Texture.Repeat
                    }
                }
                materials: [copper_004]
            }
        }
    }
}
