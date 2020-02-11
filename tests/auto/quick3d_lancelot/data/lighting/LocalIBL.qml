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

import QtQuick 2.15
import QtQuick3D 1.15
import QtQuick3D.Materials 1.15

Rectangle {
    width: 400
    height: 400
    color: "black"

    View3D {
        anchors.fill: parent

        PerspectiveCamera {
            id: cam
            position: Qt.vector3d(50, 50, 150)
        }

        DirectionalLight {
            rotation: Quaternion.fromEulerAngles(-30, -70, 0)
            ambientColor: Qt.rgba(0.8, 0.8, 0.8, 1.0);
            brightness: 100
        }

        environment: SceneEnvironment {
            probeBrightness: 750
        }

        Texture {
            id: lightprobe_texture
            source: "maps/OpenfootageNET_Gerlos-512.hdr"
            mappingMode: Texture.LightProbe
            tilingModeHorizontal: Texture.ClampToEdge
        }

        Texture {
            id: lightmap_radiosity
            source: "maps/core_lightmap_radiosity.jpg"
        }

        Model {
            id: no_ibl
            position: Qt.vector3d(0, 100, -100)
            source: "#Sphere"
            materials: [
                PrincipledMaterial {
                    baseColor: "#ffd777"
                    metalness: 0.7
                    roughness: 0.3
                    specularAmount: 0.2
                    indexOfRefraction: 1.8
                    specularTint: 0.0
                    opacity: 1.0
                    lighting: DefaultMaterial.FragmentLighting
                }
            ]
        }

        Model {
            id: local_lightprobe
            position: Qt.vector3d(100, 100, -100)
            source: "#Sphere"
            materials: [
                PrincipledMaterial {
                    baseColor: "#e7e7f7"
                    metalness: 1.0
                    roughness: 0.3
                    specularAmount: 0.2
                    indexOfRefraction: 1.8
                    specularTint: 0.0
                    opacity: 1.0
                    lighting: DefaultMaterial.FragmentLighting

                    lightProbe: lightprobe_texture
                }
            ]
        }

        Model {
            id: radiosity_lightmap
            position: Qt.vector3d(0, 0, -100)
            rotation: Quaternion.fromEulerAngles(30, -40, 0)
            scale: Qt.vector3d(30, 30, 30)
            source: "models/testCube/meshes/Cube.mesh"

            materials: [
                DefaultMaterial {
                    diffuseColor: "blue"
                    lighting: DefaultMaterial.FragmentLighting

                    lightmapRadiosity: lightmap_radiosity
                }
            ]
        }

        Model {
            id: specular_reflection_map
            position: Qt.vector3d(100, 0, -100)
            source: "#Sphere"
            materials: [
                DefaultMaterial {
                    diffuseColor: "red"
                    lighting: DefaultMaterial.FragmentLighting

                    specularReflectionMap: lightprobe_texture
                }
            ]
        }
    }
}
