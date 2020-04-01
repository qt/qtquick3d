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

import QtQuick 2.14
import QtQuick3D 1.15
import QtQuick3D.Effects 1.15

View3D {
    id: view3d
    property var effect: ({})
    property bool animated: false

    height: 200
    width: 200
    renderMode: View3D.Offscreen

    environment: SceneEnvironment {
        clearColor: "skyblue"
        backgroundMode: view3d.animated ? SceneEnvironment.Transparent : SceneEnvironment.Color
        effects: effect
    }

    PerspectiveCamera {
        position: Qt.vector3d(0, 200, 300)
        eulerRotation.x: -20
        clipNear: 100
        clipFar: 800
    }

    DirectionalLight {
        eulerRotation.x: -20
        eulerRotation.y: 20
        ambientColor: Qt.rgba(0.8, 0.8, 0.8, 1.0);
    }

    Texture {
        id: checkers
        source: "../shared/maps/checkers2.png"
        scaleU: 20
        scaleV: 20
        tilingModeHorizontal: Texture.Repeat
        tilingModeVertical: Texture.Repeat
    }

    Model {
        source: "#Rectangle"
        scale.x: 10
        scale.y: 10
        eulerRotation.x: -90
        materials: [ DefaultMaterial { diffuseMap: checkers } ]
    }

    Model {
        source: "#Cone"
        position: Qt.vector3d(100, 0, -200)
        scale.y: 3
        materials: [ DefaultMaterial { diffuseColor: "green" } ]
    }

    Model {
        id: sphere
        source: "#Sphere"
        property int displacement: 0
        NumberAnimation on displacement {
            // Short-lasting animation that stops is acceptable to lancelot
            // It will grab the steady state when animation is finished
            duration: 150
            loops: 1
            running: view3d.animated
            from: 0
            to: 1
        }

        position: Qt.vector3d(displacement < 1 ? -150 : -100, 200, -200)
        materials: [ DefaultMaterial { diffuseColor: "#808000" } ]
    }

    Model {
        source: "#Cube"
        position.y: 50
        eulerRotation.y: 20
        materials: [ DefaultMaterial { diffuseColor: "gray" } ]
    }
}
