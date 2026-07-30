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
            text: `# Iridescence
Iridescence is what gives a soap bubble, an oil slick and a beetle shell their
shifting colors. A film only a few hundred nanometers thick sits over the
surface, and light reflecting off its front face interferes with light that
went through and reflected off the back. Some wavelengths cancel out and others
reinforce, so a white light comes back colored, and the color changes as the
surface curves away from you.

Iridescence is only available on PrincipledMaterial.${rootView.specularGlossyMode ? "\n\n**The scene is currently showing the SpecularGlossyMaterial, so switch back to the PrincipledMaterial in the Basics tab to see these controls take effect.**" : ""}

Like anisotropy, this is not an extra layer of reflection. It replaces the
Fresnel response of the specular reflection the material already has, which
means it is most visible where that reflection is strongest. Turning Metalness
up in the Basics tab makes the effect far more dramatic; over a dielectric it
is deliberately subtle, because a real soap film only reflects a few percent of
the light that hits it.

## Iridescence Factor
This blends between the material without and with the film. At \`0.0\`, the
default, there is no film and nothing changes.
`
        }

        RowLayout {
            Label {
                text: "Iridescence Factor (" + rootView.targetMaterial.iridescenceFactor.toFixed(2) + ")"
                Layout.fillWidth: true
            }
            Slider {
                from: 0
                to: 1
                value: rootView.targetMaterial.iridescenceFactor
                onValueChanged: rootView.targetMaterial.iridescenceFactor = value
            }
        }

        MarkdownLabel {
            text: `### Iridescence Map
The Iridescence Map is a single channel texture multiplied against the
Iridescence Factor, so it can fade the film in and out across the surface.
`
        }

        ComboBox {
            id: iridescenceChannelComboBox
            textRole: "text"
            valueRole: "value"
            implicitContentWidthPolicy: ComboBox.WidestText
            onActivated: rootView.targetMaterial.iridescenceChannel = currentValue
            Component.onCompleted: currentIndex = indexOfValue(rootView.targetMaterial.iridescenceChannel)
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
                rootView.targetMaterial.iridescenceMap = targetTexture
            }
        }

        MarkdownLabel {
            text: `## Index Of Refraction
This is the index of refraction of the film itself, not of the material under
it. Together with the thickness it decides which colors come back. The default
of \`1.3\` is about that of a soap film; raising it strengthens the effect and
shifts the palette.
`
        }

        RowLayout {
            Label {
                text: "Iridescence IOR (" + rootView.targetMaterial.iridescenceIndexOfRefraction.toFixed(2) + ")"
                Layout.fillWidth: true
            }
            Slider {
                from: 1
                to: 3
                value: rootView.targetMaterial.iridescenceIndexOfRefraction
                onValueChanged: rootView.targetMaterial.iridescenceIndexOfRefraction = value
            }
        }

        MarkdownLabel {
            text: `## Thickness
Thickness is measured in nanometers, and it is what picks the hue. Sweeping it
across the range of visible light, roughly \`200\` to \`800\` nanometers, walks
through the whole sequence of colors the film can produce, then begins to
repeat as higher interference orders take over.

With no Thickness Map the film is uniformly Thickness Maximum thick, and the
minimum has no effect. Add a map and the two values become the ends of the
range that the map interpolates between.
`
        }

        RowLayout {
            Label {
                text: "Thickness Minimum (" + rootView.targetMaterial.iridescenceThicknessMinimum.toFixed(0) + " nm)"
                Layout.fillWidth: true
            }
            Slider {
                from: 0
                to: 1200
                value: rootView.targetMaterial.iridescenceThicknessMinimum
                onValueChanged: rootView.targetMaterial.iridescenceThicknessMinimum = value
            }
        }

        RowLayout {
            Label {
                text: "Thickness Maximum (" + rootView.targetMaterial.iridescenceThicknessMaximum.toFixed(0) + " nm)"
                Layout.fillWidth: true
            }
            Slider {
                from: 0
                to: 1200
                value: rootView.targetMaterial.iridescenceThicknessMaximum
                onValueChanged: rootView.targetMaterial.iridescenceThicknessMaximum = value
            }
        }

        MarkdownLabel {
            text: `### Thickness Map
A single channel texture interpolating the film thickness between the minimum
and the maximum. The glTF extension packs thickness into the green channel,
which is why that is the default here.
`
        }

        ComboBox {
            id: thicknessChannelComboBox
            textRole: "text"
            valueRole: "value"
            implicitContentWidthPolicy: ComboBox.WidestText
            onActivated: rootView.targetMaterial.iridescenceThicknessChannel = currentValue
            Component.onCompleted: currentIndex = indexOfValue(rootView.targetMaterial.iridescenceThicknessChannel)
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
                rootView.targetMaterial.iridescenceThicknessMap = targetTexture
            }
        }
    }
}
