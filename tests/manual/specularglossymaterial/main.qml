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
import QtQuick3D.Helpers

Window {
    id: window
    width: 1280
    height: 720
    visible: true
    title: "SpecularGlossyMaterial Example"
    color: "#848895"

    View3D {
        anchors.fill: parent
        camera: camera

        // Rotate the light direction
        DirectionalLight {
            eulerRotation.y: -100
            SequentialAnimation on eulerRotation.y {
                loops: Animation.Infinite
                PropertyAnimation {
                    duration: 5000
                    to: 360
                    from: 0
                }
            }
        }

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.SkyBox
            probeExposure: 1.5
            lightProbe: Texture {
                textureData: ProceduralSkyTextureData {}
            }
            skyboxBlurAmount: 0.4
        }

        Node {
            scale: Qt.vector3d(100, 100, 100)
            Model {
                id: waterBottle_MR
                x: -0.08
                rotation: Qt.quaternion(0, 0, 1, 0)
                source: "meshes/waterBottle_MR.mesh"

                PrincipledMaterial {
                    id: bottleMat_MR_material
                    baseColorMap: Texture {
                        source: "maps/WaterBottle_baseColor.jpg"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    opacityChannel: Material.A

                    Texture {
                        id: metalRoughnessTexture
                        source: "maps/WaterBottle_roughnessMetallic.jpg"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    metalnessMap: metalRoughnessTexture
                    metalnessChannel: Material.B
                    roughnessMap: metalRoughnessTexture
                    roughnessChannel: Material.G
                    metalness: 1
                    roughness: 1
                    normalMap: Texture {
                        source: "maps/WaterBottle_normal.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    occlusionMap: Texture {
                        source: "maps/WaterBottle_occlusion.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    occlusionChannel: Material.R
                    emissiveMap: Texture {
                        source: "maps/WaterBottle_emissive.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    emissiveFactor: Qt.vector3d(1, 1, 1)
                    alphaMode: PrincipledMaterial.Opaque
                }
                materials: [
                    bottleMat_MR_material
                ]
            }
            Model {
                id: waterBottle_SpecGloss
                x: 0.08
                rotation: Qt.quaternion(0, 0, 1, 0)
                source: "meshes/waterBottle_SpecGloss.mesh"

                SpecularGlossyMaterial {
                    id: bottleMat_SpecGloss_material
                    albedoMap: Texture {
                        source: "maps/WaterBottle_diffuse.jpg"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }

                    normalMap: Texture {
                        source: "maps/WaterBottle_normal.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    occlusionMap: Texture {
                        source: "maps/WaterBottle_occlusion.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    occlusionChannel: Material.R
                    emissiveMap: Texture {
                        source: "maps/WaterBottle_emissive.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    emissiveFactor: Qt.vector3d(1, 1, 1)
                    alphaMode: PrincipledMaterial.Opaque

                    Texture {
                        id: specularGlossyTexture
                        source: "maps/WaterBottle_specularGlossiness.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }

                    specularMap: specularGlossyTexture
                    glossiness: 1.0
                    glossinessMap: specularGlossyTexture
                    glossinessChannel: Material.A
                }
                materials: [
                    bottleMat_SpecGloss_material
                ]
            }
        }

        Node {
            id: originNode
            PerspectiveCamera {
                id: cameraNode
                position: Qt.vector3d(0, 0, 30)
            }
        }

        OrbitCameraController {
            origin: originNode
            camera: cameraNode
        }
    }
}
