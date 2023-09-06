/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

import QtQuick3D
import QtQuick

Rectangle {
    id: directionallight
    width: 800
    height: 480
    color: Qt.rgba(0, 0, 0, 1)

    View3D {
        id: view
        anchors.fill: parent
        camera: camera1

        DirectionalLight {
            castsShadow: true
            shadowFactor: 25
            eulerRotation: Qt.vector3d(-60, -20, 0)
        }

	// The ground must have shadows from the cube, the cylinder, and the big sphere above the cube.
	// The ground must not show shadowing from the smaller sphere in the center.
	// The cube must not show shadowing from from the big sphere.

        Model {
            id: ground
            source: "#Cube"
            scale: Qt.vector3d(10, 0.01, 10)
            materials: DefaultMaterial {
                diffuseColor: Qt.rgba(1.0, 1.0, 0.0, 1.0)
            }
            castsShadows: false
        }

        Model {
            source: "#Sphere"
            y: 50
            materials: DefaultMaterial {
            }
            castsShadows: false // no shadow should be visible on the ground for this
        }

        Model {
            source: "#Cylinder"
            y: 200
            x: -250
            scale: Qt.vector3d(1, 5, 1)
            materials: DefaultMaterial {
            }
        }

        Model {
            source: "#Sphere"
            x: -250
            y: 200 // above the cube
            z: 250
            materials: DefaultMaterial {
            }
        }

        Model {
            source: "#Cube"
            x: -250
            z: 250
            y: 50
            materials: DefaultMaterial {
            }
            receivesShadows: false // the sphere above would shadow it otherwise
        }

        PerspectiveCamera {
            id: camera1
            z: 600
            y: 300
            clipFar: 1000
            eulerRotation: Qt.vector3d(-20, 0, 0)
        }
    }
}
