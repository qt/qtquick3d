/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D

ScrollView {
    id: rootView
    required property PrincipledMaterial targetMaterial
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    width: availableWidth

    ColumnLayout {
        width: rootView.availableWidth
        MarkdownLabel {
            text: "# The Basics
PrincipledMaterial is a Physically Based Rendering (PBR) material that follows
the Metal/Roughness workflow. This section describes the basics of using the
fundamental properties of the PrincipledMaterial.
"
        }
        MarkdownLabel {
            text: "## Base Color
The Base Color property of the material in the Metal/Roughness workflow defines
both a diffuse reflected color for dielectric (non-metals) as well as the
reflectance value for metals. The PrincipledMaterial enables the Base Color to
be defined either as a single color, or as a texture.
### Base Color (Scalar)
"
        }
        RowLayout {
            Label {
                text: "Red (" + targetMaterial.baseColor.r.toFixed(2) + ")"
                Layout.fillWidth: true
            }
            Slider {
                from: 0
                to: 1
                value: targetMaterial.baseColor.r
                onValueChanged: targetMaterial.baseColor.r = value
            }
        }
        RowLayout {
            Label {
                text: "Green (" + targetMaterial.baseColor.g.toFixed(2) + ")"
                Layout.fillWidth: true
            }

            Slider {
                from: 0
                to: 1
                value: targetMaterial.baseColor.g
                onValueChanged: targetMaterial.baseColor.g = value
            }
        }
        RowLayout {
            Label {
                text: "Blue  (" + targetMaterial.baseColor.b.toFixed(2) + ")"
                Layout.fillWidth: true
            }

            Slider {
                from: 0
                to: 1
                value: targetMaterial.baseColor.b
                onValueChanged: targetMaterial.baseColor.b = value
            }
        }

        MarkdownLabel {
            text: "### Base Color (Map)
When using a texture for Base Color, it is important to remember that the final
Base Color value will be the combination of both the Base Color (Scalar) and the
Base Color Map (Texture). Setting the Base Color (Scaler) to White, or in other
words 1.0 for all color channels, the Colors in the Base Color Map will be exactly
as intended.
"
        }
        Button {
            text: "Reset Base Color"
            onClicked: {
                targetMaterial.baseColor = "white"
            }
        }

        TextureSourceControl {
            defaultTexture: "maps/metallic/basecolor.jpg"
            onTargetTextureChanged: {
                targetMaterial.baseColorMap = targetTexture
            }
        }

        VerticalSectionSeparator {}

        MarkdownLabel {
            text: "## Metalness
The Metalness property of the PrincipledMaterial defines how to interpret the
Base Color of the material.  A value of 0 means that the material is dielectric
(non-metal) and Base Color should be interpreted as diffuse reflected color. If
the Metalness value is 1 then the material is a raw metal and Base Color should
be interpreted as reflectance values. For the the most realistic results, this
value should be either 0 or 1, but it is possible to set values in between to
get mixed of both for artistic reasons."
        }
        RowLayout {
            Label {
                text: "Metalness (" + targetMaterial.metalness.toFixed(2) + ")"
                Layout.fillWidth: true
            }
            Slider {
                from: 0
                to: 1
                value: targetMaterial.metalness
                onValueChanged: targetMaterial.metalness = value
            }
        }
        MarkdownLabel {
            text: "### Metalness (Map)
The Metalness Map property specifies a texture to sample the Metalness value
from. Since the Metalness property is only a single floating point value between
0.0 and 1.0, it's only necessary to use a single color channel of the image, or
a greyscale image. By default PrincipledMaterial will use the value in the blue
channel of the texture, but it's possible to change which color channel is used.
"
        }
        ComboBox {
            id: metalnessChannelComboBox
            textRole: "text"
            valueRole: "value"
            implicitContentWidthPolicy: ComboBox.WidestText
            onActivated: targetMaterial.metalnessChannel = currentValue
            Component.onCompleted: currentIndex = indexOfValue(targetMaterial.metalnessChannel)
            model: [
                { value: PrincipledMaterial.R, text: "Red Channel"},
                { value: PrincipledMaterial.G, text: "Green Channel"},
                { value: PrincipledMaterial.B, text: "Blue Channel"},
                { value: PrincipledMaterial.A, text: "Alpha Channel"}
            ]
        }
        MarkdownLabel {
            text: "
When using a Metalness Map the value sampled from the map file is multiplied by
the value of the Metalness property. In practice this means that the maximum
Metalness value possible will be the value set by the Metalness map is the
value in the Metalness property. So most of the time when using a Metalness
Map it will make sense to leave the value of Metalness to 1.0.
"
        }
        Button {
            text: "Reset Metalness Value"
            onClicked: targetMaterial.metalness = 1.0
        }

        TextureSourceControl {
            defaultClearColor: "black"
            defaultTexture: "maps/metallic/metallic.jpg"
            onTargetTextureChanged: {
                targetMaterial.metalnessMap = targetTexture
            }
        }

        VerticalSectionSeparator {}

        MarkdownLabel {
            text: "## Roughness
The Roughness property defines the amount of irregularities in the surface of
a material. The more irregular a surface is, the more the light reflecting off
of it will be spread and cause light diffusion. If the surface instead is smooth
with few surface irregularity then light will be reflected in a more focused and
intense manner. A value of 0.0 means that the surface is smooth and reflective as
a mirror, and a value of 1.0 means that the surface is so rough its hard for light
to be reflected at all."
        }
        RowLayout {
            Label {
                text: "Roughness (" + targetMaterial.roughness.toFixed(2) + ")"
                Layout.fillWidth: true
            }
            Slider {
                from: 0
                to: 1
                value: targetMaterial.roughness
                onValueChanged: targetMaterial.roughness = value
            }
        }
        MarkdownLabel {
            text: "### Roughness (Map)
The Roughness Map property specifies a texture to sample the Roughness value
from. Since the Roughness property is only a single floating point value between
0.0 and 1.0, it's only necessary to use a single color channel of the image, or
a greyscale image. By default PrincipledMaterial will use the value in the green
channel of the texture, but it's possible to change which color channel is used.
"
        }
        ComboBox {
            id: roughnessChannelComboBox
            textRole: "text"
            valueRole: "value"
            implicitContentWidthPolicy: ComboBox.WidestText
            onActivated: targetMaterial.roughnessChannel = currentValue
            Component.onCompleted: currentIndex = indexOfValue(targetMaterial.roughnessChannel)
            model: [
                { value: PrincipledMaterial.R, text: "Red Channel"},
                { value: PrincipledMaterial.G, text: "Green Channel"},
                { value: PrincipledMaterial.B, text: "Blue Channel"},
                { value: PrincipledMaterial.A, text: "Alpha Channel"}
            ]
        }
        MarkdownLabel {
            text: "
When using a Roughness Map the value sampled from the map file is multiplied by
the value of the Roughness property. In practice this means that the maximum
Roughness value possible will be the value set by the Roughness map is the
value in the Roughness property. So most of the time when using a Roughness
Map it will make sense to leave the value of Roughness to 1.0.
"
        }
        Button {
            text: "Reset Roughness Value"
            onClicked: targetMaterial.roughness = 1.0
        }

        TextureSourceControl {
            defaultClearColor: "black"
            defaultTexture: "maps/metallic/roughness.jpg"
            onTargetTextureChanged: {
                targetMaterial.roughnessMap = targetTexture
            }
        }
    }
}
