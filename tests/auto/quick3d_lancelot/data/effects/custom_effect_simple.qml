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
import QtQuick3D.Effects

GridView {
    width: 600
    height: 400
    cellWidth: 200
    cellHeight: 200

    model: 6

    Effect {
        id: e0
        property TextureInput tex: TextureInput {
            texture: Texture { source: "../shared/maps/rgba.png" }
        }
        passes: Pass {
            shaders: Shader {
                stage: Shader.Fragment
                shader: "custom_effect_simple_0.frag"
            }
        }
    }

    Effect {
        id: e1
        property real uRed: 0.5
        Shader {
            id: e1_vs
            stage: Shader.Vertex
            shader: "custom_effect_simple_1.vert"
        }
        Shader {
            id: e1_fs
            stage: Shader.Fragment
            shader: "custom_effect_simple_1.frag"
        }
        passes: Pass {
            shaders: [ e1_vs, e1_fs ]
        }
    }

    Effect {
        id: e2
        passes: Pass {
            shaders: Shader {
                stage: Shader.Vertex
                shader: "custom_effect_simple_2.vert"
            }
        }
    }

    Effect {
        id: e3
        Shader {
            id: e3_fs_1
            stage: Shader.Fragment
            shader: "custom_effect_simple_3_1.frag"
        }
        Shader {
            id: e3_fs_2
            stage: Shader.Fragment
            shader: "custom_effect_simple_3_2.frag"
        }
        Buffer {
            id: tempBuffer
            name: "tempBuffer"
            sizeMultiplier: 0.5
        }
        passes: [
            Pass {
                shaders: [ e3_fs_1 ]
                output: tempBuffer
            },
            Pass {
                shaders: [ e3_fs_2 ]
                commands: BufferInput {
                    buffer: tempBuffer
                    sampler: "pass1Result"
                }
            }
        ]
        property TextureInput pass1Result: TextureInput { texture: Texture {} }
    }

    Effect {
        id: e4
        Buffer {
            id: tempBuffer2
            name: "tempBuffer"
        }
        passes: [
            Pass {
                shaders: [ e3_fs_1 ]
                output: tempBuffer2
            },
            Pass {
                shaders: [ e3_fs_2 ]
                commands: BufferInput {
                    buffer: tempBuffer2
                    sampler: "pass1Result"
                }
            }
        ]
        property TextureInput pass1Result: TextureInput { texture: Texture {} }
    }

    Effect {
        id: e5
        // referencing e0's TextureInput does not work for whatever reason, so have our own
        property TextureInput tex: TextureInput {
            texture: Texture { source: "../shared/maps/rgba.png" }
        }
        property TextureInput originalInput: TextureInput { texture: Texture {} }
        property real uRed: 1.0
        Buffer {
            id: tempBuffer3
            name: "tempBuffer"
        }
        Shader {
            id: e5_fs_1
            stage: Shader.Fragment
            shader: "custom_effect_simple_5_1.frag"
        }
        Shader {
            id: e5_fs_2
            stage: Shader.Fragment
            shader: "custom_effect_simple_5_2.frag"
        }
        passes: [
            Pass {
                shaders: [ e5_fs_1 ]
                output: tempBuffer3
            },
            Pass {
                shaders: [ e5_fs_2 ]
                commands: [
                    BufferInput {
                        buffer: tempBuffer3
                    },
                    BufferInput {
                        sampler: "originalInput"
                    },
                    SetUniformValue {
                        target: "uRed"
                        // the background with the original scene behind the
                        // RGBA text will have it's red channel removed,
                        // becoming more cyan than white, where the rgba image
                        // does not add back red
                        value: 0.0
                    }
                ]
            }
        ]
    }

    delegate: PlainView {
        effect: index == 0 ? e0 : index == 1 ? e1 : index == 2 ? e2 : index == 3 ? e3 : index == 4 ? e4 : e5
    }
}
