/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

//! [import]
import QtQuick
import QtQuick3D
import QtQuick3D.Helpers
//! [import]

Window {
    id: window
    width: 1280
    height: 720
    visible: true

    View3D {
        id: view
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "black"
            backgroundMode: SceneEnvironment.Color
        }

        PerspectiveCamera {
            position: Qt.vector3d(0, 0, 300)
            clipNear: 1.0
            NumberAnimation on z {
                from: 300
                to: 0
                duration: 10 * 1000
            }
        }

        DirectionalLight {
            eulerRotation.x: -30
            eulerRotation.y: -70
            ambientColor: Qt.rgba(0.5, 0.5, 0.5, 1.0)
        }

        //! [randomInstancing]
        RandomInstancing {
            id: randomInstancing
            instanceCount: 1500

            position: InstanceRange {
                from: Qt.vector3d(-300, -200, -500)
                to: Qt.vector3d(300, 200, 200)
            }
            scale: InstanceRange {
                from: Qt.vector3d(1, 1, 1)
                to: Qt.vector3d(10, 10, 10)
                proportional: true
            }
            rotation: InstanceRange {
                from: Qt.vector3d(0, 0, 0)
                to: Qt.vector3d(360, 360, 360)
            }
            color: InstanceRange {
                from: "grey"
                to: "white"
                proportional: true
            }

            randomSeed: 2021
        }
        //! [randomInstancing]

        //! [manualInstancing]
        InstanceListEntry {
            id: redShip
            position: Qt.vector3d(50, 10, 100)
            eulerRotation: Qt.vector3d(0, 180, 0)
            color: "red"
            NumberAnimation on position.x {
                from: 50
                to: -70
                duration: 8000
            }
        }

        InstanceListEntry {
            id: greenShip
            position: Qt.vector3d(0, 0, -60)
            eulerRotation: Qt.vector3d(-10, 0, 30)
            color: "green"
        }

        InstanceListEntry {
            id: blueShip
            position: Qt.vector3d(-100, -100, 0)
            color: "blue"
        }

        InstanceList {
            id: manualInstancing
            instances: [ redShip, greenShip, blueShip ]
        }
        //! [manualInstancing]

        //! [objects]
        Asteroid {
            instancing: randomInstancing
            NumberAnimation on eulerRotation.x {
                from: 0
                to: 360
                duration: 11000
                loops: Animation.Infinite
            }
        }

        SimpleSpaceship {
            instancing: manualInstancing
        }
        //! [objects]
    }
}
