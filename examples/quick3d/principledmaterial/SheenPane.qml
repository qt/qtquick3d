// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D

ScrollView {
    id: rootView
    required property PrincipledMaterial targetMaterial
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    width: availableWidth
    property bool specularGlossyMode: false

    ColumnLayout {
        width: rootView.availableWidth

        MarkdownLabel {
            text: `# Sheen
Sheen adds a soft, retroreflective highlight at grazing angles, which is what
gives fabrics like velvet, satin and denim their characteristic look. Where the
specular lobe of the base material concentrates light around the mirror
direction, the sheen lobe spreads it towards the silhouette of the object.

Sheen is only available on PrincipledMaterial.${rootView.specularGlossyMode ? "\n\n**The scene is currently showing the SpecularGlossyMaterial, so switch back to the PrincipledMaterial in the Basics tab to see these controls take effect.**" : ""}

## Sheen Color
The sheen layer is disabled while Sheen Color is black, which is the default.
That is because the color doubles as the strength of the layer: it is the
reflectance of the sheen lobe, and it is also what the Sheen Color Map is
multiplied with. Set it to something other than black to enable sheen.
`
        }

        RowLayout {
            Label {
                text: "Red (" + rootView.targetMaterial.sheenColor.r.toFixed(2) + ")"
                Layout.fillWidth: true
            }
            Slider {
                from: 0
                to: 1
                value: rootView.targetMaterial.sheenColor.r
                onValueChanged: rootView.targetMaterial.sheenColor.r = value
            }
        }
        RowLayout {
            Label {
                text: "Green (" + rootView.targetMaterial.sheenColor.g.toFixed(2) + ")"
                Layout.fillWidth: true
            }
            Slider {
                from: 0
                to: 1
                value: rootView.targetMaterial.sheenColor.g
                onValueChanged: rootView.targetMaterial.sheenColor.g = value
            }
        }
        RowLayout {
            Label {
                text: "Blue (" + rootView.targetMaterial.sheenColor.b.toFixed(2) + ")"
                Layout.fillWidth: true
            }
            Slider {
                from: 0
                to: 1
                value: rootView.targetMaterial.sheenColor.b
                onValueChanged: rootView.targetMaterial.sheenColor.b = value
            }
        }

        Button {
            text: "Reset Sheen Color"
            onClicked: rootView.targetMaterial.sheenColor = "black"
        }

        MarkdownLabel {
            text: `### Sheen Color Map
The Sheen Color Map is an RGB texture that is multiplied with Sheen Color, so
it can be used to vary both the tint and the strength of the sheen layer across
the surface. Remember that the layer stays disabled where the product is black.
`
        }

        TextureSourceControl {
            defaultTexture: "maps/noise.png"
            defaultClearColor: "black"
            onTargetTextureChanged: {
                rootView.targetMaterial.sheenColorMap = targetTexture
            }
        }

        MarkdownLabel {
            text: `## Sheen Roughness
Sheen Roughness controls how far the highlight spreads. At 0.0 the sheen is a
narrow rim right at the silhouette; as it approaches 1.0 the lobe widens until
it covers the whole surface with an even, dusty looking sheen. Note that even
at 0.0 the sheen lobe is considerably wider than the specular lobe of the base
material at the same roughness.
`
        }

        RowLayout {
            Label {
                text: "Sheen Roughness (" + rootView.targetMaterial.sheenRoughness.toFixed(2) + ")"
                Layout.fillWidth: true
            }
            Slider {
                from: 0
                to: 1
                value: rootView.targetMaterial.sheenRoughness
                onValueChanged: rootView.targetMaterial.sheenRoughness = value
            }
        }

        MarkdownLabel {
            text: `### Sheen Roughness Map
The Sheen Roughness Map is a single channel texture which is multiplied against
the Sheen Roughness value. glTF assets pack sheen roughness into the alpha
channel of the sheen color texture, which is why the Alpha channel is the
default here.
`
        }

        ComboBox {
            id: sheenRoughnessChannelComboBox
            textRole: "text"
            valueRole: "value"
            implicitContentWidthPolicy: ComboBox.WidestText
            onActivated: rootView.targetMaterial.sheenRoughnessChannel = currentValue
            Component.onCompleted: currentIndex = indexOfValue(rootView.targetMaterial.sheenRoughnessChannel)
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
                rootView.targetMaterial.sheenRoughnessMap = targetTexture
            }
        }
    }
}
