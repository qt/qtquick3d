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
import QtQuick.Layouts
import QtQuick3D

Rectangle {
    width: 500
    height: 400
    color: "lightgray"

    View3D {
        anchors.fill: parent
        renderMode: View3D.Offscreen

        OrthographicCamera {
            z: 500
        }

        DirectionalLight {
            eulerRotation.y: 180
            ambientColor: Qt.rgba(1.0, 1.0, 1.0, 1.0);
        }

        Text {
            text: "Default   |   autoOrientation: false   |   flipV: true   |   autoOrientation: false; flipV: true"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        // Row 1

        // A #Rectangle model textured with a Texture sourced from a .png file.
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(0.7, 0.7, 1)
            position: Qt.vector3d(-150, 100, 0)
            materials: [ DefaultMaterial {
                    diffuseMap: Texture { source: "../shared/maps/t2.png" }
                } ]
        }

        // autoOrientation is false but has no visible effect.
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(0.7, 0.7, 1)
            position: Qt.vector3d(-50, 100, 0)
            materials: [ DefaultMaterial {
                    diffuseMap: Texture {
                        source: "../shared/maps/t2.png"
                        autoOrientation: false
                    }
                } ]
        }

        // flipV is set to true (appears upside down).
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(0.7, 0.7, 1)
            position: Qt.vector3d(50, 100, 0)
            materials: [ DefaultMaterial {
                    diffuseMap: Texture {
                        source: "../shared/maps/t2.png"
                        flipV: true
                    }
                } ]
        }

        // autoOrientation=false and flipV=true (same as just setting flipV=true)
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(0.7, 0.7, 1)
            position: Qt.vector3d(150, 100, 0)
            materials: [ DefaultMaterial {
                    diffuseMap: Texture {
                        source: "../shared/maps/t2.png"
                        autoOrientation: false
                        flipV: true
                    }
                } ]
        }

        // Row 2

        // A #Rectangle model textured with a Texture sourced from a .ktx file.
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(0.7, 0.7, 1)
            position: Qt.vector3d(-150, 0, 0)
            materials: [ DefaultMaterial {
                    diffuseMap: Texture { source: "../shared/maps/o1_bc1.ktx" }
                } ]
        }

        // autoOrientation is false so it should show up as upside down (as if flipV was set to true).
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(0.7, 0.7, 1)
            position: Qt.vector3d(-50, 0, 0)
            materials: [ DefaultMaterial {
                    diffuseMap: Texture {
                        source: "../shared/maps/o1_bc1.ktx"
                        autoOrientation: false
                    }
                } ]
        }

        // flipV is set to true (appears upside down).
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(0.7, 0.7, 1)
            position: Qt.vector3d(50, 0, 0)
            materials: [ DefaultMaterial {
                    diffuseMap: Texture {
                        source: "../shared/maps/o1_bc1.ktx"
                        flipV: true
                    }
                } ]
        }

        // autoOrientation=false and flipV=true (same as column #1 because the effective flipV is false).
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(0.7, 0.7, 1)
            position: Qt.vector3d(150, 0, 0)
            materials: [ DefaultMaterial {
                    diffuseMap: Texture {
                        source: "../shared/maps/o1_bc1.ktx"
                        autoOrientation: false
                        flipV: true
                    }
                } ]
        }

        // Row 3

        // A #Rectangle model textured with a Texture with a sourceItem.
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(0.7, 0.7, 1)
            position: Qt.vector3d(-150, -100, 0)
            materials: [ DefaultMaterial {
                    diffuseMap: Texture {
                        sourceItem: Rectangle {
                            id: miniQuickScene
                            width: 128
                            height: 128
                            color: "green"
                            Image {
                                source: "../shared/maps/t2.png"
                                anchors.centerIn: parent
                            }
                        }
                    }
                } ]
        }

        // autoOrientation is false so it should show up as upside down (as if flipV was set to true).
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(0.7, 0.7, 1)
            position: Qt.vector3d(-50, -100, 0)
            materials: [ DefaultMaterial {
                    diffuseMap: Texture {
                        autoOrientation: false
                        sourceItem: miniQuickScene
                    }
                } ]
        }

        // flipV is set to true (appears upside down).
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(0.7, 0.7, 1)
            position: Qt.vector3d(50, -100, 0)
            materials: [ DefaultMaterial {
                    diffuseMap: Texture {
                        flipV: true
                        sourceItem: miniQuickScene
                    }
                } ]
        }

        // autoOrientation=false and flipV=true (same as column #1 because the effective flipV is false).
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(0.7, 0.7, 1)
            position: Qt.vector3d(150, -100, 0)
            materials: [ DefaultMaterial {
                    diffuseMap: Texture {
                        autoOrientation: false
                        flipV: true
                        sourceItem: miniQuickScene
                    }
                } ]
        }
    }
}
