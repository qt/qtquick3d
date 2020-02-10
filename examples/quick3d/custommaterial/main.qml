/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.15
import QtQuick.Window 2.14
import QtQuick3D 1.15
import QtQuick3D.Materials 1.15

Window {
    width: 1280
    height: 720
    visible: true
    title: "Custom Materials Example"

    View3D {
        anchors.fill: parent
        camera: camera

        //! [environment]
        environment: SceneEnvironment {
            clearColor: "#848895"
            backgroundMode: SceneEnvironment.Color
            probeBrightness: 1000
            lightProbe: Texture {
                source: "maps/OpenfootageNET_garage-1024.hdr"
            }
            antialiasingMode: SceneEnvironment.SSAA
            antialiasingQuality: SceneEnvironment.VeryHigh
        }
        //! [environment]

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 0, 600)
        }

        //! [bumpy aluminum]
        WeirdShape {
            customMaterial: AluminumMaterial {
                bump_amount: 5.0
            }
            position: Qt.vector3d(150, 150, -100)
        }
        //! [bumpy aluminum]

        //! [copper]
        WeirdShape {
            customMaterial: CopperMaterial {}
            position: Qt.vector3d(-150, -150, -100)
        }
        //! [copper]

        //! [frosted glass]
        Model {
            position: Qt.vector3d(-300, 0, 100)
            scale: Qt.vector3d(2.5, 2.5, 2.5)
            source: "#Sphere"
            materials: [ FrostedGlassSinglePassMaterial {
                    roughness: 0.1
                    reflectivity_amount: 0.9
                    glass_ior: 1.9
                    glass_color: Qt.vector3d(0.85, 0.85, 0.9)
                }
            ]
        }
        //! [frosted glass]

        //! [plastic]
        Model {
            position: Qt.vector3d(300, 0, 100)
            scale: Qt.vector3d(2.5, 2.5, 2.5)
            source: "#Sphere"
            materials: [ PlasticStructuredRedMaterial {
                    material_ior: 1.55
                    bump_factor: 0.1
                }
            ]
        }
        //! [plastic]
    }
}
