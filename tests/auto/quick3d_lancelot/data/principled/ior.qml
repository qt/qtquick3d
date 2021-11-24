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
            probeExposure: 3.0
        }

        PerspectiveCamera {
            id: camera1
            z: 0.75
            clipNear: 0.01
            clipFar: 100
        }

        Node {
            id: rOOT
            Model {
                id: labels
                x: -0.55
                source: "../shared/models/ior/labels.mesh"

                PrincipledMaterial {
                    id: labelMat_material
                    baseColorMap: Texture {
                        source: "../shared/maps/IOR_Labels.png"
                        minFilter: Texture.Nearest
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    opacityChannel: Material.A
                    roughness: 0.8
                    alphaMode: PrincipledMaterial.Opaque
                }
                materials: [
                    labelMat_material
                ]
            }
            Node {
                id: sampleRoot
                x: 0.2
                Model {
                    id: iOR_1_0
                    y: -0.22
                    source: "../shared/models/ior/opaque.mesh"

                    PrincipledMaterial {
                        id: mat_IOR_1_0_material
                        baseColor: "#ff80b3e6"
                        metalnessMap: Texture {
                            source: "../shared/maps/RoughnessGrid.png"
                            minFilter: Texture.Nearest
                            generateMipmaps: true
                            mipFilter: Texture.Linear
                        }
                        metalnessChannel: Material.B
                        roughnessMap: Texture {
                            source: "../shared/maps/RoughnessGrid.png"
                            minFilter: Texture.Nearest
                            generateMipmaps: true
                            mipFilter: Texture.Linear
                        }
                        roughnessChannel: Material.G
                        metalness: 1
                        roughness: 1
                        alphaMode: PrincipledMaterial.Opaque
                        transmissionFactor: 1
                        thicknessFactor: 0.005
                        indexOfRefraction: 1
                    }
                    materials: [
                        mat_IOR_1_0_material
                    ]
                }
                Model {
                    id: iOR_1_33
                    y: -0.11
                    source: "../shared/models/ior/opaque.mesh"

                    PrincipledMaterial {
                        id: mat_IOR_1_33_material
                        baseColor: "#ff80b3e6"
                        metalnessMap: Texture {
                            source: "../shared/maps/RoughnessGrid.png"
                            minFilter: Texture.Nearest
                            generateMipmaps: true
                            mipFilter: Texture.Linear
                        }
                        metalnessChannel: Material.B
                        roughnessMap: Texture {
                            source: "../shared/maps/RoughnessGrid.png"
                            minFilter: Texture.Nearest
                            generateMipmaps: true
                            mipFilter: Texture.Linear
                        }
                        roughnessChannel: Material.G
                        metalness: 1
                        roughness: 1
                        alphaMode: PrincipledMaterial.Opaque
                        transmissionFactor: 1
                        thicknessFactor: 0.005
                        indexOfRefraction: 1.33
                    }
                    materials: [
                        mat_IOR_1_33_material
                    ]
                }
                Model {
                    id: iOR_1_50
                    source: "../shared/models/ior/opaque.mesh"

                    PrincipledMaterial {
                        id: mat_IOR_1_50_material
                        baseColor: "#ff80b3e6"
                        metalnessMap: Texture {
                            source: "../shared/maps/RoughnessGrid.png"
                            minFilter: Texture.Nearest
                            generateMipmaps: true
                            mipFilter: Texture.Linear
                        }
                        metalnessChannel: Material.B
                        roughnessMap: Texture {
                            source: "../shared/maps/RoughnessGrid.png"
                            minFilter: Texture.Nearest
                            generateMipmaps: true
                            mipFilter: Texture.Linear
                        }
                        roughnessChannel: Material.G
                        metalness: 1
                        roughness: 1
                        alphaMode: PrincipledMaterial.Opaque
                        transmissionFactor: 1
                        thicknessFactor: 0.005
                    }
                    materials: [
                        mat_IOR_1_50_material
                    ]
                }
                Model {
                    id: iOR_1_76
                    y: 0.11
                    source: "../shared/models/ior/opaque.mesh"

                    PrincipledMaterial {
                        id: mat_IOR_1_76_material
                        baseColor: "#ff80b3e6"
                        metalnessMap: Texture {
                            source: "../shared/maps/RoughnessGrid.png"
                            minFilter: Texture.Nearest
                            generateMipmaps: true
                            mipFilter: Texture.Linear
                        }
                        metalnessChannel: Material.B
                        roughnessMap: Texture {
                            source: "../shared/maps/RoughnessGrid.png"
                            minFilter: Texture.Nearest
                            generateMipmaps: true
                            mipFilter: Texture.Linear
                        }
                        roughnessChannel: Material.G
                        metalness: 1
                        roughness: 1
                        alphaMode: PrincipledMaterial.Opaque
                        transmissionFactor: 1
                        thicknessFactor: 0.005
                        indexOfRefraction: 1.76
                    }
                    materials: [
                        mat_IOR_1_76_material
                    ]
                }
                Model {
                    id: iOR_2_42
                    y: 0.22
                    source: "../shared/models/ior/opaque.mesh"

                    PrincipledMaterial {
                        id: mat_IOR_2_42_material
                        baseColor: "#ff80b3e6"
                        metalnessMap: Texture {
                            source: "../shared/maps/RoughnessGrid.png"
                            minFilter: Texture.Nearest
                            generateMipmaps: true
                            mipFilter: Texture.Linear
                        }
                        metalnessChannel: Material.B
                        roughnessMap: Texture {
                            source: "../shared/maps/RoughnessGrid.png"
                            minFilter: Texture.Nearest
                            generateMipmaps: true
                            mipFilter: Texture.Linear
                        }
                        roughnessChannel: Material.G
                        metalness: 1
                        roughness: 1
                        alphaMode: PrincipledMaterial.Opaque
                        transmissionFactor: 1
                        thicknessFactor: 0.005
                        indexOfRefraction: 2.42
                    }
                    materials: [
                        mat_IOR_2_42_material
                    ]
                }
                Model {
                    id: opaque
                    y: -0.33
                    source: "../shared/models/ior/opaque.mesh"

                    PrincipledMaterial {
                        id: mat_Opaque_material
                        baseColor: "#ffcccccc"
                        metalnessMap: Texture {
                            source: "../shared/maps/RoughnessGrid-1.png"
                            minFilter: Texture.Nearest
                            generateMipmaps: true
                            mipFilter: Texture.Linear
                        }
                        metalnessChannel: Material.B
                        roughnessMap: Texture {
                            source: "../shared/maps/RoughnessGrid-1.png"
                            minFilter: Texture.Nearest
                            generateMipmaps: true
                            mipFilter: Texture.Linear
                        }
                        roughnessChannel: Material.G
                        metalness: 1
                        roughness: 1
                        alphaMode: PrincipledMaterial.Opaque
                    }
                    materials: [
                        mat_Opaque_material
                    ]
                }
            }
            Model {
                id: flat_Backdrop
                z: 0.1
                source: "../shared/models/ior/flat_Backdrop.mesh"

                PrincipledMaterial {
                    id: flatBackdrop_material
                    baseColorMap: Texture {
                        source: "../shared/maps/GridWithDetails.png"
                        minFilter: Texture.Nearest
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    opacityChannel: Material.A
                    roughness: 0.5
                    alphaMode: PrincipledMaterial.Opaque
                }
                materials: [
                    flatBackdrop_material
                ]
            }
            Model {
                id: smoothRoughLabels
                source: "../shared/models/ior/smoothRoughLabels.mesh"

                PrincipledMaterial {
                    id: smoothVsRough_material
                    baseColorMap: Texture {
                        source: "../shared/maps/SmoothVsRough.png"
                        minFilter: Texture.Nearest
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    opacityChannel: Material.A
                    roughness: 0.8
                    cullMode: Material.NoCulling
                    alphaMode: PrincipledMaterial.Opaque
                }
                materials: [
                    smoothVsRough_material
                ]
            }
        }
    }
}
