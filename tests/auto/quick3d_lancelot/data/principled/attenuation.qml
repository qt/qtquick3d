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
            z: 15

        }

        Node {
            id: rOOT
            Model {
                id: r2_Block_1_0
                y: 3
                source: "../shared/models/attenuation/r2_Block_1_0.mesh"

                PrincipledMaterial {
                    id: r2_and_R4_ThicknessFac_1_0_material
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                    thicknessFactor: 1
                    attenuationDistance: 1
                    attenuationColor: "#ff1a80e6"
                }
                materials: [
                    r2_and_R4_ThicknessFac_1_0_material
                ]
            }
            Model {
                id: r2_Block_1_5
                x: 2.5
                y: 3
                source: "../shared/models/attenuation/r2_Block_1_5.mesh"

                PrincipledMaterial {
                    id: r2_ThicknessFac_1_5_material
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                    thicknessFactor: 1.5
                    attenuationDistance: 1
                    attenuationColor: "#ff1a80e6"
                }
                materials: [
                    r2_ThicknessFac_1_5_material
                ]
            }
            Model {
                id: r2_Block_2_0
                x: 6
                y: 3
                source: "../shared/models/attenuation/r2_Block_2_0.mesh"

                PrincipledMaterial {
                    id: r2_ThicknessFac_2_0_material
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                    thicknessFactor: 2
                    attenuationDistance: 1
                    attenuationColor: "#ff1a80e6"
                }
                materials: [
                    r2_ThicknessFac_2_0_material
                ]
            }
            Model {
                id: r2_Block_0_50
                x: -2
                y: 3
                source: "../shared/models/attenuation/r2_Block_0_50.mesh"

                PrincipledMaterial {
                    id: r2_ThicknessFac_0_50_material
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                    thicknessFactor: 0.5
                    attenuationDistance: 1
                    attenuationColor: "#ff1a80e6"
                }
                materials: [
                    r2_ThicknessFac_0_50_material
                ]
            }
            Model {
                id: r2_Block_0_25
                x: -3.5
                y: 3
                source: "../shared/models/attenuation/r2_Block_0_25.mesh"

                PrincipledMaterial {
                    id: r2_ThicknessFac_0_25_material
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                    thicknessFactor: 0.25
                    attenuationDistance: 1
                    attenuationColor: "#ff1a80e6"
                }
                materials: [
                    r2_ThicknessFac_0_25_material
                ]
            }
            Model {
                id: labels
                source: "../shared/models/attenuation/labels.mesh"

                PrincipledMaterial {
                    id: labelMaterial_material
                    baseColorMap: Texture {
                        source: "../shared/maps/AttenuationLabels.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    opacityChannel: Material.A
                    roughness: 0.8
                    alphaMode: PrincipledMaterial.Opaque
                }
                materials: [
                    labelMaterial_material
                ]
            }
            Model {
                id: r4_Block_1_0
                y: -3
                source: "../shared/models/attenuation/r4_Block_1_0.mesh"
                materials: [
                    r2_and_R4_ThicknessFac_1_0_material
                ]
            }
            Model {
                id: r4_Block_1_5
                x: 2.5
                y: -3
                scale.x: 1.5
                scale.y: 1.5
                scale.z: 1.5
                source: "../shared/models/attenuation/r4_Block_1_5.mesh"
                materials: [
                    r2_and_R4_ThicknessFac_1_0_material
                ]
            }
            Model {
                id: r4_Block_2_0
                x: 6
                y: -3
                scale.x: 2
                scale.y: 2
                scale.z: 2
                source: "../shared/models/attenuation/r4_Block_2_0.mesh"
                materials: [
                    r2_and_R4_ThicknessFac_1_0_material
                ]
            }
            Model {
                id: r4_Block_0_50
                x: -2
                y: -3
                scale.x: 0.5
                scale.y: 0.5
                scale.z: 0.5
                source: "../shared/models/attenuation/r4_Block_0_50.mesh"
                materials: [
                    r2_and_R4_ThicknessFac_1_0_material
                ]
            }
            Model {
                id: r4_Block_0_25
                x: -3.5
                y: -3
                scale.x: 0.25
                scale.y: 0.25
                scale.z: 0.25
                source: "../shared/models/attenuation/r4_Block_0_25.mesh"
                materials: [
                    r2_and_R4_ThicknessFac_1_0_material
                ]
            }
            Model {
                id: r3_Block_Row
                x: -3.5
                source: "../shared/models/attenuation/r3_Block_Row.mesh"

                PrincipledMaterial {
                    id: r3_ThicknessTex_Mat_material
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                    thicknessFactor: 2
                    thicknessMap: Texture {
                        source: "../shared/maps/ThicknessTexture.png"
                        generateMipmaps: true
                        mipFilter: Texture.Linear
                    }
                    attenuationDistance: 1
                    attenuationColor: "#ff1a80e6"
                }
                materials: [
                    r3_ThicknessTex_Mat_material
                ]
            }
            Model {
                id: r5_Block_1_0
                y: -6
                source: "../shared/models/attenuation/r5_Block_1_0.mesh"

                PrincipledMaterial {
                    id: r5_Attenuation_1_0_material
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                    thicknessFactor: 1
                    attenuationDistance: 1
                    attenuationColor: "#ff1a80e6"
                }
                materials: [
                    r5_Attenuation_1_0_material
                ]
            }
            Model {
                id: r5_Block_1_5
                x: 2.5
                y: -6
                source: "../shared/models/attenuation/r5_Block_1_5.mesh"

                PrincipledMaterial {
                    id: r5_Attenuation_1_5_material
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                    thicknessFactor: 1
                    attenuationDistance: 0.666667
                    attenuationColor: "#ff1a80e6"
                }
                materials: [
                    r5_Attenuation_1_5_material
                ]
            }
            Model {
                id: r5_Block_2_0
                x: 6
                y: -6
                source: "../shared/models/attenuation/r5_Block_2_0.mesh"

                PrincipledMaterial {
                    id: r5_Attenuation_2_0_material
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                    thicknessFactor: 1
                    attenuationDistance: 0.5
                    attenuationColor: "#ff1a80e6"
                }
                materials: [
                    r5_Attenuation_2_0_material
                ]
            }
            Model {
                id: r5_Block_0_50
                x: -2
                y: -6
                source: "../shared/models/attenuation/r5_Block_0_50.mesh"

                PrincipledMaterial {
                    id: r5_Attenuation_0_50_material
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                    thicknessFactor: 1
                    attenuationDistance: 2
                    attenuationColor: "#ff1a80e6"
                }
                materials: [
                    r5_Attenuation_0_50_material
                ]
            }
            Model {
                id: r5_Block_0_25
                x: -3.5
                y: -6
                source: "../shared/models/attenuation/r5_Block_0_25.mesh"

                PrincipledMaterial {
                    id: r5_Attenuation_0_25_material
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                    thicknessFactor: 1
                    attenuationDistance: 4
                    attenuationColor: "#ff1a80e6"
                }
                materials: [
                    r5_Attenuation_0_25_material
                ]
            }
            Model {
                id: meterGrid
                z: -2
                source: "../shared/models/attenuation/meterGrid.mesh"

                PrincipledMaterial {
                    id: flatBackdrop_material
                    baseColorMap: Texture {
                        source: "../shared/maps/PlainGrid.png"
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
                id: r1_Sample_1_0
                y: 6
                source: "../shared/models/attenuation/r1_Sample_1_0.mesh"

                PrincipledMaterial {
                    id: r1_Sample_1_0_material
                    baseColor: "#ff1a80e6"
                    cullMode: Material.NoCulling
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                }
                materials: [
                    r1_Sample_1_0_material
                ]
            }
            Model {
                id: r1_Sample_1_5
                x: 2.5
                y: 6
                source: "../shared/models/attenuation/r1_Sample_1_5.mesh"

                PrincipledMaterial {
                    id: r1_Sample_1_5_material
                    baseColor: "#ff085ada"
                    cullMode: Material.NoCulling
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                }
                materials: [
                    r1_Sample_1_5_material
                ]
            }
            Model {
                id: r1_Sample_2_0
                x: 6
                y: 6
                source: "../shared/models/attenuation/r1_Sample_2_0.mesh"

                PrincipledMaterial {
                    id: r1_Sample_2_0_material
                    baseColor: "#ff0340cf"
                    cullMode: Material.NoCulling
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                }
                materials: [
                    r1_Sample_2_0_material
                ]
            }
            Model {
                id: r1_Sample_0_50
                x: -2
                y: 6
                source: "../shared/models/attenuation/r1_Sample_0_50.mesh"

                PrincipledMaterial {
                    id: r1_Sample_0_50_material
                    baseColor: "#ff51b4f2"
                    cullMode: Material.NoCulling
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                }
                materials: [
                    r1_Sample_0_50_material
                ]
            }
            Model {
                id: r1_Sample_0_25
                x: -3.5
                y: 6
                source: "../shared/models/attenuation/r1_Sample_0_25.mesh"

                PrincipledMaterial {
                    id: r1_Sample_0_25_material
                    baseColor: "#ff8fd6f8"
                    cullMode: Material.NoCulling
                    alphaMode: PrincipledMaterial.Opaque
                    transmissionFactor: 1
                }
                materials: [
                    r1_Sample_0_25_material
                ]
            }
        }
    }
}
