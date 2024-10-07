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
    width: 400
    height: 400
    color: "black"

    View3D {
        id: v3d
        anchors.fill: parent
        camera: camera

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 0, 600)
        }

        DirectionalLight {
            position: Qt.vector3d(-500, 500, -100)
            ambientColor: Qt.rgba(0.1, 0.1, 0.1, 1.0)
        }

        property variant cubeScale: Qt.vector3d(1.5, 1.5, 1.5)

        // The default wrap mode is Repeat. Try with a shader that samples so that
        // it relies on the repeat wrap mode.
        Model {
            scale: v3d.cubeScale
            x: -200
            source: "#Cube"
            materials: [
                CustomMaterial {
                    TextureInput {
                        id: texInput
                        texture: Texture {
                            source: "../shared/maps/oulu_2.jpeg"
                        }
                    }
                    fragmentShader: "customtexturefiltertiling_repeat.frag"
                    property TextureInput tex: texInput
                }
            ]
        }

        // Try ClampToEdge with the same shader, should sample zeroes, resulting in a grayish cube.
        Model {
            scale: v3d.cubeScale
            x: 0
            source: "#Cube"
            materials: [
                CustomMaterial {
                    fragmentShader: "customtexturefiltertiling_repeat.frag"
                    property TextureInput tex: TextureInput {
                        texture: Texture {
                            source: "../shared/maps/oulu_2.jpeg"
                            tilingModeHorizontal: Texture.ClampToEdge
                            tilingModeVertical: Texture.ClampToEdge
                        }
                    }
                }
            ]
        }

        // ClampToEdge with in-range texture coordinates, should show up normally
        Model {
            scale: v3d.cubeScale
            x: 200
            source: "#Cube"
            materials: [
                CustomMaterial {
                    fragmentShader: "customtexturefiltertiling.frag"
                    property TextureInput tex: TextureInput {
                        texture: Texture {
                            source: "../shared/maps/oulu_2.jpeg"
                            tilingModeHorizontal: Texture.ClampToEdge
                            tilingModeVertical: Texture.ClampToEdge
                        }
                    }
                }
            ]
        }

        // min/mag filter
        Model {
            scale: v3d.cubeScale
            x: -200
            y: -200
            source: "#Cube"
            materials: [
                CustomMaterial {
                    fragmentShader: "customtexturefiltertiling.frag"
                    property TextureInput tex: TextureInput {
                        texture: Texture {
                            source: "../shared/maps/oulu_2.jpeg"
                            minFilter: Texture.Nearest
                            magFilter: Texture.Nearest
                        }
                    }
                }
            ]
        }

        Model {
            scale: v3d.cubeScale
            x: 0
            y: -200
            source: "#Cube"
            materials: [
                CustomMaterial {
                    fragmentShader: "customtexturefiltertiling.frag"
                    property TextureInput tex: TextureInput {
                        texture: Texture {
                            source: "../shared/maps/oulu_2.jpeg"
                            minFilter: Texture.Linear
                            magFilter: Texture.Linear
                        }
                    }
                }
            ]
        }

        Model {
            scale: v3d.cubeScale
            x: 200
            y: -200
            source: "#Cube"
            materials: [
                CustomMaterial {
                    fragmentShader: "customtexturefiltertiling.frag"
                    property TextureInput tex: TextureInput {
                        texture: Texture {
                            source: "../shared/maps/oulu_2.jpeg"
                            minFilter: Texture.None // effectively linear
                            magFilter: Texture.None
                        }
                    }
                }
            ]
        }

        // mipmap, common case
        Model {
            scale: v3d.cubeScale
            x: -200
            y: 200
            source: "#Cube"
            materials: [
                CustomMaterial {
                    fragmentShader: "customtexturefiltertiling.frag"
                    property TextureInput tex: TextureInput {
                        texture: Texture {
                            source: "../shared/maps/oulu_2.jpeg"
                            minFilter: Texture.Linear
                            magFilter: Texture.Linear
                            mipFilter: Texture.Linear
                            generateMipmaps: true
                        }
                    }
                }
            ]
        }

        // missing generateMipmaps, not mipmapped
        Model {
            scale: v3d.cubeScale
            x: 0
            y: 200
            source: "#Cube"
            materials: [
                CustomMaterial {
                    fragmentShader: "customtexturefiltertiling.frag"
                    property TextureInput tex: TextureInput {
                        texture: Texture {
                            source: "../shared/maps/oulu_2.jpeg"
                            minFilter: Texture.Nearest
                            magFilter: Texture.Nearest
                            mipFilter: Texture.Linear
                        }
                    }
                }
            ]
        }

        // Now actually sample a mip level in the shader. (textureLod)
        Model {
            scale: v3d.cubeScale
            x: 200
            y: 200
            source: "#Cube"
            materials: [
                CustomMaterial {
                    fragmentShader: "customtexturefiltertiling_mip.frag"
                    property TextureInput tex: TextureInput {
                        texture: Texture {
                            source: "../shared/maps/oulu_2.jpeg"
                            minFilter: Texture.Nearest
                            magFilter: Texture.Nearest
                            mipFilter: Texture.Nearest
                            generateMipmaps: true
                        }
                    }
                }
            ]
        }
    }
}
