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
            text: "# Clearcoat
Clearcoat when enabled gives the appearance of another protective layer on top
of the existing material. A real life analog would be a clear lacquer.

## Clearcoat Amount
To enable the clearcoat layer, the value of Clearcoat amount must be change to
something greater than 0.0. There is a cost for adding a clearcoat layer
(basically doing all specular lighting for a second time) so is disabled by
default.
"
        }

        RowLayout {
            Label {
                text: "Clearcoat Amount (" + targetMaterial.clearcoatAmount.toFixed(2) + ")"
                Layout.fillWidth: true
            }
            Slider {
                from: 0
                to: 1
                value: targetMaterial.clearcoatAmount
                onValueChanged: targetMaterial.clearcoatAmount = value
            }
        }

        MarkdownLabel {
            text: "### Clearcoat Map
The Clearcoat Map is a single channel texture in which when multiplied against
the Clearcoat Amount property determines what the clearcoat amount is for the
material."
        }
        ComboBox {
            id: clearcoatChannelComboBox
            textRole: "text"
            valueRole: "value"
            implicitContentWidthPolicy: ComboBox.WidestText
            onActivated: targetMaterial.clearcoatChannel = currentValue
            Component.onCompleted: currentIndex = indexOfValue(targetMaterial.clearcoatChannel)
            model: [
                { value: PrincipledMaterial.R, text: "Red Channel"},
                { value: PrincipledMaterial.G, text: "Green Channel"},
                { value: PrincipledMaterial.B, text: "Blue Channel"},
                { value: PrincipledMaterial.A, text: "Alpha Channel"}
            ]
        }

        TextureSourceControl {
            defaultTexture: "maps/noise.png"
            defaultClearColor: "black"
            onTargetTextureChanged: {
                targetMaterial.clearcoatMap = targetTexture
            }
        }

        MarkdownLabel {
            text: "## Clearcoat Roughness
Just like the base material layer, the Clearcoat layer also has has a roughness
property. The Clearcoat Roughness property will make the specular reflection of
the clearcoat layer more diffuse."
        }

        RowLayout {
            Label {
                text: "Clearcoat Roughness (" + targetMaterial.clearcoatRoughnessAmount.toFixed(2) + ")"
                Layout.fillWidth: true
            }
            Slider {
                from: 0
                to: 1
                value: targetMaterial.clearcoatRoughnessAmount
                onValueChanged: targetMaterial.clearcoatRoughnessAmount = value
            }
        }

        MarkdownLabel {
            text: "### Clearcoat Roughness Map
The Clearcoat Roughness Map is a single channel texture in which when multiplied
against the Clearcoat Roughness Amount property determines the roughness of the clearcoat
layer for the material."
        }

        ComboBox {
            id: clearcoatRoughnessChannelComboBox
            textRole: "text"
            valueRole: "value"
            implicitContentWidthPolicy: ComboBox.WidestText
            onActivated: targetMaterial.clearcoatRoughnessChannel = currentValue
            Component.onCompleted: currentIndex = indexOfValue(targetMaterial.clearcoatRoughnessChannel)
            model: [
                { value: PrincipledMaterial.R, text: "Red Channel"},
                { value: PrincipledMaterial.G, text: "Green Channel"},
                { value: PrincipledMaterial.B, text: "Blue Channel"},
                { value: PrincipledMaterial.A, text: "Alpha Channel"}
            ]
        }

        TextureSourceControl {
            defaultTexture: "maps/noise.png"
            defaultClearColor: "black"
            onTargetTextureChanged: {
                targetMaterial.clearcoatRoughnessMap = targetTexture
            }
        }

        MarkdownLabel {
            text: "## Clearcoat Normal Map
Finally, the clearcoat layer also supports applying a normal map to further
define the surface details. This normal map works the same way as the Normal
Map property of PrincipledMaterial, just only applies to the clearcoat layer.
It is possible that the material would have two normal maps applied in this
case.
"
        }

        TextureSourceControl {
            defaultTexture: "maps/metallic/normal.jpg"
            defaultClearColor: Qt.rgba(0.5, 0.5, 1.0, 1.0)
            stampMode: true
            stampSource: "maps/normal_stamp.png"
            onTargetTextureChanged: {
                targetMaterial.clearcoatNormalMap = targetTexture
            }
        }
    }
}
