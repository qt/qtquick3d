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

import QtQuick3D 1.15
import QtQuick 2.15

Node {
    id: doorRoot
    property PerspectiveCamera activeCamera: camera

    PointLight {
        id: lamp
        x: -10
        y: 100
        z: -100
        color: "#ffffffff"
        linearFade: 1
        brightness: 300
    }

    PerspectiveCamera {
        id: camera

        x: 180.067
        y: 225.598
        z: -411.521
        eulerRotation.x: -15.4614

        eulerRotation.y: 171.605

        fieldOfViewOrientation: Camera.Horizontal
    }

    Model {
        id: door1
        pivot.x: 20
        x: 80
        y: 70
        scale.x: 2
        scale.y: 3.5
        scale.z: 0.5
        source: "meshes/door1.mesh"
        pickable: true

        //! [material]
        DefaultMaterial {
            id: material_001_material
            diffuseMap: Texture {
                sourceItem: object2d
            }
        }
        materials: [
            material_001_material
        ]
        //! [material]

        //! [state]
        states: State {
            name: "opened"
            PropertyChanges {
                target: door1
                eulerRotation.y: 90
            }
        }
        transitions: Transition {
            to: "opened"
            reversible: true
            SequentialAnimation {
                PropertyAnimation { property: "eulerRotation.y"; duration: 2000 }
            }
        }
        //! [state]
     }

    Model {
        id: wall
        y: 100
        scale.x: 400
        scale.y: 100
        scale.z: 10
        source: "meshes/wall.mesh"

        DefaultMaterial {
            id: material_material
            diffuseColor: "lightgreen"
        }
        materials: [
            material_material
        ]
    }

    Model {
        id: door2
        x: -80
        y: 70
        scale.x: 2
        scale.y: 3.5
        scale.z: 0.5
        pivot.x: -20
        source: "meshes/door2.mesh"
        pickable: true
        materials: [
            material_001_material
        ]
        states: State {
            name: "opened"
            PropertyChanges {
                target: door2
                eulerRotation.y: -90
            }
        }
        transitions: Transition {
            to: "opened"
            reversible: true
            SequentialAnimation {
                PropertyAnimation { property: "eulerRotation.y"; duration: 2000 }
            }
        }
    }
}
