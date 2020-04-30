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

import QtQuick 2.15
import QtQuick3D 1.15
import QtQuick3D.Materials 1.15

Rectangle {
    height: 320
    width: 480
    color: "white"

    AluminumMaterial {id: m1}
    AluminumAnodizedMaterial {id: m2}
    AluminumAnodizedEmissiveMaterial {id: m3}
    AluminumBrushedMaterial {id: m4}
    AluminumEmissiveMaterial {id: m5}
    CopperMaterial {id: m6}
    FrostedGlassMaterial {id: m7}
    FrostedGlassSinglePassMaterial {id: m8}
    GlassMaterial {id: m9}
    GlassRefractiveMaterial {id: m10}
    PaperArtisticMaterial {id: m11}
    PaperOfficeMaterial {id: m12}
    PlasticStructuredRedMaterial {id: m13}
    PlasticStructuredRedEmissiveMaterial {id: m14}
    SteelMilledConcentricMaterial {id: m15}

    property var materialList: [ m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15]

    property int itemsWidth: 480
    property int itemsHeight: 300
    property int modelSeparation: itemsWidth / (materialList.length - 1)

    View3D {
        id: view
        anchors.fill: parent
        environment: SceneEnvironment {
            lightProbe: Texture {
                source: "../shared/maps/OpenfootageNET_lowerAustria01-1024.hdr"
            }
            probeBrightness: 500
        }

        PerspectiveCamera {
            z: 400
        }
        DirectionalLight {
            eulerRotation.x: -60
            brightness: 50
        }

       Repeater3D {
            model: materialList.length
            Node {
                property int rows: 3
                x: -itemsWidth / 2 + index * modelSeparation
                y: -itemsHeight / 2 + (index % rows) * itemsHeight/(rows - 1)

                Node {
                    Model {
                        y: -50
                        rotation: Quaternion.fromEulerAngles(-45, -30, 0)
                        scale: Qt.vector3d(70, 70, 70)
                        source: "../shared/models/barrel/meshes/Barrel.mesh"
                        materials: materialList[index]
                    }
                    Model {
                        y: 50
                        source: "#Sphere"
                        materials:  materialList[index]
                    }
                }
            }
       }

        //background objects

       Repeater3D {
           model: 100
           Model {
               x: (index/10 - 5)*150;
               y: (index%10 - 5)*150;
               z: -500
               source: "#Cube"
               scale: Qt.vector3d(0.5, 0.5, 0.5)
               materials: DefaultMaterial {
                   diffuseColor: "#ffbb99"
               }
           }
       }
    }
}
