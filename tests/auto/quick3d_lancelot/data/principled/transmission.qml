/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

Rectangle {
    width: 900
    height: 450
    color: "lightgray"

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.SkyBox
            lightProbe: Texture {
                source: "../shared/maps/OpenfootageNET_lowerAustria01-1024.hdr"
            }
        }

        PerspectiveCamera {
            id: camera1
            z: 150
        }

        Node {
            id: rOOT
            scale: Qt.vector3d(200, 200, 200)
            Model {
                id: cloth_Backdrop_01
                x: -0.154701
                y: -0.401584
                z: -0.0603687
                source: "../shared/models/cloth_Backdrop_01.mesh"

                PrincipledMaterial {
                    id: cloth_Backdrop_material
                    baseColorMap: Texture {
                        source: "../shared/maps/texture28577.jpg"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    opacityChannel: Material.A
                    roughness: 0.4935
                    alphaMode: PrincipledMaterial.Opaque
                }
                materials: [
                    cloth_Backdrop_material
                ]
            }
            Model {
                id: redTransTexture
                x: 0.101354
                y: 0.132672
                z: 0.0168398
                rotation: Qt.quaternion(0.026177, 0, 0.999657, 0)
                source: "../shared/models/ball.mesh"

                PrincipledMaterial {
                    id: redTransTexture_material
                    baseColor: "#fff42d18"
                    cullMode: Material.NoCulling
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                    transmissionMap: Texture {
                        source: "../shared/maps/texture14184.jpg"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                }
                materials: [
                    redTransTexture_material
                ]
            }
            Model {
                id: yellowTrans
                x: -0.367091
                y: 0.126122
                z: -0.0105196
                rotation: Qt.quaternion(0.026177, 0, 0.999657, 0)
                source: "../shared/models/ball.mesh"

                PrincipledMaterial {
                    id: yellowTrans_material
                    baseColor: "#ffd8de18"
                    cullMode: Material.NoCulling
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                }
                materials: [
                    yellowTrans_material
                ]
            }
            Model {
                id: blueTransWithMask
                x: 0.335576
                y: 0.135947
                z: 0.0305195
                rotation: Qt.quaternion(0.026177, 0, 0.999657, 0)
                source: "../shared/models/ball.mesh"

                PrincipledMaterial {
                    id: blueTransWithMask_material
                    baseColor: "#ff1676d8"
                    baseColorMap: Texture {
                        source: "../shared/maps/texture214190.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    opacityChannel: Material.A
                    cullMode: Material.NoCulling
                    alphaMode: PrincipledMaterial.Mask
                    transmissionFactor: 1
                    transmissionMap: Texture {
                        source: "../shared/maps/texture14184.jpg"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                }
                materials: [
                    blueTransWithMask_material
                ]
            }
            Model {
                id: greenMask
                x: -0.132868
                y: 0.129397
                z: 0.00316013
                rotation: Qt.quaternion(0.026177, 0, 0.999657, 0)
                source: "../shared/models/ball.mesh"

                PrincipledMaterial {
                    id: greenMask_material
                    baseColor: "#ff26d431"
                    baseColorMap: Texture {
                        source: "../shared/maps/texture4086.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    opacityChannel: Material.A
                    cullMode: Material.NoCulling
                    alphaMode: PrincipledMaterial.Mask
                    transmissionFactor: 1
                }
                materials: [
                    greenMask_material
                ]
            }
            Model {
                id: redTransTextureRough
                x: 0.101354
                y: -0.0934502
                z: 0.0168398
                rotation: Qt.quaternion(0.026177, 0, 0.999657, 0)
                source: "../shared/models/ball.mesh"

                PrincipledMaterial {
                    id: redTransTextureRough_material
                    baseColor: "#fff42d18"
                    metalnessMap: Texture {
                        source: "../shared/maps/texture177328.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    metalnessChannel: Material.B
                    roughnessMap: Texture {
                        source: "../shared/maps/texture177328.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    roughnessChannel: Material.G
                    roughness: 1
                    cullMode: Material.NoCulling
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                    transmissionMap: Texture {
                        source: "../shared/maps/texture14184.jpg"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                }
                materials: [
                    redTransTextureRough_material
                ]
            }
            Model {
                id: yellowTransRough
                x: -0.367091
                y: -0.1
                z: -0.0105196
                rotation: Qt.quaternion(0.026177, 0, 0.999657, 0)
                source: "../shared/models/ball.mesh"

                PrincipledMaterial {
                    id: yellowTransRough_material
                    baseColor: "#ffd8de18"
                    metalnessMap: Texture {
                        source: "../shared/maps/texture6807.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    metalnessChannel: Material.B
                    roughnessMap: Texture {
                        source: "../shared/maps/texture6807.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    roughnessChannel: Material.G
                    roughness: 1
                    cullMode: Material.NoCulling
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                }
                materials: [
                    yellowTransRough_material
                ]
            }
            Model {
                id: blueTransWithMask_1
                x: 0.335576
                y: -0.0901753
                z: 0.0305195
                rotation: Qt.quaternion(0.026177, 0, 0.999657, 0)
                source: "../shared/models/ball.mesh"

                PrincipledMaterial {
                    id: blueTransWithMask_material_1
                    baseColor: "#ff1676d8"
                    baseColorMap: Texture {
                        source: "../shared/maps/texture214190.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    opacityChannel: Material.A
                    metalnessMap: Texture {
                        source: "../shared/maps/texture177328.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    metalnessChannel: Material.B
                    roughnessMap: Texture {
                        source: "../shared/maps/texture177328.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    roughnessChannel: Material.G
                    roughness: 1
                    cullMode: Material.NoCulling
                    alphaMode: PrincipledMaterial.Mask
                    transmissionFactor: 1
                    transmissionMap: Texture {
                        source: "../shared/maps/texture14184.jpg"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                }
                materials: [
                    blueTransWithMask_material_1
                ]
            }
            Model {
                id: greenMaskRough
                x: -0.132868
                y: -0.0967251
                z: 0.00316013
                rotation: Qt.quaternion(0.026177, 0, 0.999657, 0)
                source: "../shared/models/ball.mesh"

                PrincipledMaterial {
                    id: greenMaskRough_material
                    baseColor: "#ff26d431"
                    baseColorMap: Texture {
                        source: "../shared/maps/texture4086.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    opacityChannel: Material.A
                    roughness: 0.32
                    cullMode: Material.NoCulling
                    alphaMode: PrincipledMaterial.Mask
                    transmissionFactor: 1
                }
                materials: [
                    greenMaskRough_material
                ]
            }
            Model {
                id: redTransTextureMetal
                x: 0.101354
                y: -0.327465
                z: 0.0168398
                rotation: Qt.quaternion(0.026177, 0, 0.999657, 0)
                source: "../shared/models/ball.mesh"

                PrincipledMaterial {
                    id: redTransTextureMetal_material
                    baseColor: "#fff42d18"
                    metalnessMap: Texture {
                        source: "../shared/maps/texture175763.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    metalnessChannel: Material.B
                    roughnessMap: Texture {
                        source: "../shared/maps/texture175763.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    roughnessChannel: Material.G
                    metalness: 1
                    cullMode: Material.NoCulling
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                    transmissionMap: Texture {
                        source: "../shared/maps/texture14184.jpg"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                }
                materials: [
                    redTransTextureMetal_material
                ]
            }
            Model {
                id: yellowTransMetal
                x: -0.367091
                y: -0.334015
                z: -0.0105196
                rotation: Qt.quaternion(0.026177, 0, 0.999657, 0)
                source: "../shared/models/ball.mesh"

                PrincipledMaterial {
                    id: yellowTransMetal_material
                    baseColor: "#ffd8de18"
                    metalnessMap: Texture {
                        source: "../shared/maps/texture10487.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    metalnessChannel: Material.B
                    roughnessMap: Texture {
                        source: "../shared/maps/texture10487.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    roughnessChannel: Material.G
                    metalness: 1
                    cullMode: Material.NoCulling
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                }
                materials: [
                    yellowTransMetal_material
                ]
            }
            Model {
                id: greenMaskMetal
                x: -0.132868
                y: -0.33074
                z: 0.00316013
                rotation: Qt.quaternion(0.026177, 0, 0.999657, 0)
                source: "../shared/models/ball.mesh"

                PrincipledMaterial {
                    id: greenMaskMetal_material
                    baseColor: "#ff26d431"
                    baseColorMap: Texture {
                        source: "../shared/maps/texture4086.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    opacityChannel: Material.A
                    metalnessMap: Texture {
                        source: "../shared/maps/texture175763.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    metalnessChannel: Material.B
                    roughnessMap: Texture {
                        source: "../shared/maps/texture175763.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    roughnessChannel: Material.G
                    metalness: 1
                    cullMode: Material.NoCulling
                    alphaMode: PrincipledMaterial.Mask
                    transmissionFactor: 1
                }
                materials: [
                    greenMaskMetal_material
                ]
            }
            Model {
                id: blueTransWithMask_2
                x: 0.335576
                y: -0.32419
                z: 0.0305195
                rotation: Qt.quaternion(0.026177, 0, 0.999657, 0)
                source: "../shared/models/ball.mesh"

                PrincipledMaterial {
                    id: blueTransWithMask_material_2
                    baseColor: "#ff1676d8"
                    baseColorMap: Texture {
                        source: "../shared/maps/texture214190.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    opacityChannel: Material.A
                    metalnessMap: Texture {
                        source: "../shared/maps/texture175763.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    metalnessChannel: Material.B
                    roughnessMap: Texture {
                        source: "../shared/maps/texture175763.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    roughnessChannel: Material.G
                    metalness: 1
                    cullMode: Material.NoCulling
                    alphaMode: PrincipledMaterial.Mask
                    transmissionFactor: 1
                    transmissionMap: Texture {
                        source: "../shared/maps/texture14184.jpg"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                }
                materials: [
                    blueTransWithMask_material_2
                ]
            }
        }
    }
}
