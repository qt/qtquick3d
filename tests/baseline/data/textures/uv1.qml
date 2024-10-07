/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

Item {
    width: 400
    height: 400

    View3D {
        id: v3d
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "lightGray"
            backgroundMode: SceneEnvironment.Color
        }

        OrthographicCamera { // no need to confuse the output with depth, orthographic is good here
            id: camera
            position: Qt.vector3d(0, 0, 600)
        }

        camera: camera

        DirectionalLight {
        }

        // Show the same model with the same material, just switch the UV
        // channel from 0 to 1 in the second one. The mesh contains both UV0 and
        // UV1, with a different mapping, so the difference should be visually
        // obvious. If UV1 is not picked up correctly the second model (bottom
        // right corner) will be identical to the first one, which is wrong.

        Model {
            source: "../shared/models/animal_with_lightmapuv1.mesh"
            scale: Qt.vector3d(50, 50, 50)
            eulerRotation.y: -80
            x: -80
            y: 100
            materials: [
                DefaultMaterial {
                    diffuseMap: Texture {
                        source: "../shared/maps/oulu_2.jpeg"
                    }
                }
            ]
        }

        Model {
            property real modelScale: 100
            source: "../shared/models/animal_with_lightmapuv1.mesh"
            scale: Qt.vector3d(50, 50, 50)
            eulerRotation.y: -80
            x: 80
            y: -70
            materials: [
                DefaultMaterial {
                    diffuseMap: Texture {
                        source: "../shared/maps/oulu_2.jpeg"
                        indexUV: 1
                    }
                }
            ]
        }
    }
}
