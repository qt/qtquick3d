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

import QtQuick
import QtQuick3D

Rectangle {
    width: 500
    height: 500

    View3D {
        anchors.fill: parent

        PointLight {
            id: light
            position: Qt.vector3d(5, 5, 5)
            color: "#ffffffff"
        }

        PerspectiveCamera {
            id: camera
            z: 10
            clipNear: 0.1
            clipFar: 100
        }

        MorphTarget {
            id: morphtarget
            weight: 0
            attributes: MorphTarget.Position | MorphTarget.Normal
        }
        MorphTarget {
            id: morphtarget1
            weight: 0.33
            attributes: MorphTarget.Position | MorphTarget.Normal
        }
        MorphTarget {
            id: morphtarget2
            weight: 0.67
            attributes: MorphTarget.Position | MorphTarget.Normal
        }
        MorphTarget {
            id: morphtarget3
            weight: 1
            attributes: MorphTarget.Position | MorphTarget.Normal
        }
        Model {
            id: suzanne
            source: "../shared/models/suzanne.mesh"
            x: -3
            y: 3

            morphTargets: [
                morphtarget,
                morphtarget3
            ]

            PrincipledMaterial {
                id: _material
                metalness: 1
                roughness: 1
                alphaMode: PrincipledMaterial.Opaque
            }
            materials: [
                _material
            ]
        }

        Model {
            id: suzanne1
            source: "../shared/models/suzanne.mesh"
            y: 3
            morphTargets: [
                morphtarget,
                morphtarget2
            ]
            materials: [
                _material
            ]
        }

        Model {
            id: suzanne2
            source: "../shared/models/suzanne.mesh"
            x: 3
            y: 3
            morphTargets: [
                morphtarget,
                morphtarget1
            ]
            materials: [
                _material
            ]
        }

        Model {
            id: suzanne3
            source: "../shared/models/suzanne.mesh"
            x: -3
            morphTargets: [
                morphtarget,
                morphtarget
            ]
            materials: [
                _material
            ]
        }

        Model {
            id: suzanne4
            source: "../shared/models/suzanne.mesh"
            morphTargets: [
                morphtarget1,
                morphtarget
            ]
            materials: [
                _material
            ]
        }

        Model {
            id: suzanne5
            source: "../shared/models/suzanne.mesh"
            x: 3
            morphTargets: [
                morphtarget2,
                morphtarget
            ]
            materials: [
                _material
            ]
        }

        Model {
            id: suzanne6
            source: "../shared/models/suzanne.mesh"
            x: -3
            y: -3
            morphTargets: [
                morphtarget3,
                morphtarget
            ]
            materials: [
                _material
            ]
        }

        Model {
            id: suzanne7
            source: "../shared/models/suzanne.mesh"
            y: -3
            morphTargets: [
                morphtarget3,
                morphtarget2
            ]
            materials: [
                _material
            ]
        }

        Model {
            id: suzanne8
            source: "../shared/models/suzanne.mesh"
            x: 3
            y: -3
            morphTargets: [
                morphtarget3,
                morphtarget3
            ]
            materials: [
                _material
            ]
        }
    }
}
