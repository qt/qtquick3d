// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D

ScrollView {
    id: rootView
    required property PrincipledMaterial principledMaterial
    required property SpecularGlossyMaterial specularGlossyMaterial
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    width: availableWidth
    property bool specularGlossyMode: false

    ColumnLayout {
        width: rootView.availableWidth
        MarkdownLabel {
            text: "# The Basics
PrincipledMaterial is a Physically Based Rendering (PBR) material that follows
the Metal/Roughness workflow. This section describes the basics of using the
fundamental properties of the PrincipledMaterial. An alternative workflow is
the Specular/Glossy workflow which works like the PrincipledMaterial and is
called SpecularGlossyMaterial.
"
        }
        RowLayout {
            Label {
                text: "Metalness/Roughness"
            }

            Switch {
                checked: rootView.specularGlossyMode
                onCheckedChanged: {
                    rootView.specularGlossyMode = checked
                }
            }
            Label {
                text: "Specular/Glossy"
            }
        }
        ColumnLayout {
            width: rootView.availableWidth
            visible: !rootView.specularGlossyMode
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
                    text: "Red (" + principledMaterial.baseColor.r.toFixed(2) + ")"
                    Layout.fillWidth: true
                }
                Slider {
                    from: 0
                    to: 1
                    value: principledMaterial.baseColor.r
                    onValueChanged: principledMaterial.baseColor.r = value
                }
            }
            RowLayout {
                Label {
                    text: "Green (" + principledMaterial.baseColor.g.toFixed(2) + ")"
                    Layout.fillWidth: true
                }

                Slider {
                    from: 0
                    to: 1
                    value: principledMaterial.baseColor.g
                    onValueChanged: principledMaterial.baseColor.g = value
                }
            }
            RowLayout {
                Label {
                    text: "Blue  (" + principledMaterial.baseColor.b.toFixed(2) + ")"
                    Layout.fillWidth: true
                }

                Slider {
                    from: 0
                    to: 1
                    value: principledMaterial.baseColor.b
                    onValueChanged: principledMaterial.baseColor.b = value
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
                    principledMaterial.baseColor = "white"
                }
            }

            TextureSourceControl {
                defaultTexture: "maps/metallic/basecolor.jpg"
                onTargetTextureChanged: {
                    principledMaterial.baseColorMap = targetTexture
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
                    text: "Metalness (" + principledMaterial.metalness.toFixed(2) + ")"
                    Layout.fillWidth: true
                }
                Slider {
                    from: 0
                    to: 1
                    value: principledMaterial.metalness
                    onValueChanged: principledMaterial.metalness = value
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
                onActivated: principledMaterial.metalnessChannel = currentValue
                Component.onCompleted: currentIndex = indexOfValue(principledMaterial.metalnessChannel)
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
                onClicked: principledMaterial.metalness = 1.0
            }

            TextureSourceControl {
                defaultClearColor: "black"
                defaultTexture: "maps/metallic/metallic.jpg"
                onTargetTextureChanged: {
                    principledMaterial.metalnessMap = targetTexture
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
                    text: "Roughness (" + principledMaterial.roughness.toFixed(2) + ")"
                    Layout.fillWidth: true
                }
                Slider {
                    from: 0
                    to: 1
                    value: principledMaterial.roughness
                    onValueChanged: principledMaterial.roughness = value
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
                onActivated: principledMaterial.roughnessChannel = currentValue
                Component.onCompleted: currentIndex = indexOfValue(principledMaterial.roughnessChannel)
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
                onClicked: principledMaterial.roughness = 1.0
            }

            TextureSourceControl {
                defaultClearColor: "black"
                defaultTexture: "maps/metallic/roughness.jpg"
                onTargetTextureChanged: {
                    principledMaterial.roughnessMap = targetTexture
                }
            }
        }
        ColumnLayout {
            width: rootView.availableWidth
            visible: rootView.specularGlossyMode
            MarkdownLabel {
                text: "## Albedo Color
The Albedo Color property of the material in the Specular/Glossy workflow defines
both a diffuse color without reflectance values. The SpecularGlossyMaterial enables
the Albedo Color to be defined either as a single color, or as a texture.
### Albedo Color (Scalar)
    "
            }
            RowLayout {
                Label {
                    text: "Red (" + specularGlossyMaterial.baseColor.r.toFixed(2) + ")"
                    Layout.fillWidth: true
                }
                Slider {
                    from: 0
                    to: 1
                    value: specularGlossyMaterial.albedoColor.r
                    onValueChanged: specularGlossyMaterial.albedoColor.r = value
                }
            }
            RowLayout {
                Label {
                    text: "Green (" + specularGlossyMaterial.albedoColor.g.toFixed(2) + ")"
                    Layout.fillWidth: true
                }

                Slider {
                    from: 0
                    to: 1
                    value: specularGlossyMaterial.albedoColor.g
                    onValueChanged: specularGlossyMaterial.albedoColor.g = value
                }
            }
            RowLayout {
                Label {
                    text: "Blue  (" + specularGlossyMaterial.albedoColor.b.toFixed(2) + ")"
                    Layout.fillWidth: true
                }

                Slider {
                    from: 0
                    to: 1
                    value: specularGlossyMaterial.albedoColor.b
                    onValueChanged: specularGlossyMaterial.albedoColor.b = value
                }
            }

            MarkdownLabel {
                text: "### Albedo (Map)
When using a texture for Albedo Color, it is important to remember that the final
Albedo Color value will be the combination of both the Albedo Color (Scalar) and the
Albedo Color Map (Texture). Setting the Albedo Color (Scaler) to White, or in other
words 1.0 for all color channels, the Colors in the Albedo Map will be exactly
as intended.
    "
            }
            Button {
                text: "Reset Albedo Color"
                onClicked: {
                    specularGlossyMaterial.albedoColor = "white"
                }
            }

            TextureSourceControl {
                defaultTexture: "maps/metallic/basecolor.jpg"
                onTargetTextureChanged: {
                    specularGlossyMaterial.albedoMap = targetTexture
                }
            }

            VerticalSectionSeparator {}

            MarkdownLabel {
                text: "## Specular Color
The Specular Color property of the SpecularGlossyMaterial defines the reflectance
Color of the material."
            }
            RowLayout {
                Label {
                    text: "Red (" + specularGlossyMaterial.specularColor.r.toFixed(2) + ")"
                    Layout.fillWidth: true
                }
                Slider {
                    from: 0
                    to: 1
                    value: specularGlossyMaterial.specularColor.r
                    onValueChanged: specularGlossyMaterial.specularColor.r = value
                }
            }
            RowLayout {
                Label {
                    text: "Green (" + specularGlossyMaterial.specularColor.g.toFixed(2) + ")"
                    Layout.fillWidth: true
                }

                Slider {
                    from: 0
                    to: 1
                    value: specularGlossyMaterial.specularColor.g
                    onValueChanged: specularGlossyMaterial.specularColor.g = value
                }
            }
            RowLayout {
                Label {
                    text: "Blue  (" + specularGlossyMaterial.specularColor.b.toFixed(2) + ")"
                    Layout.fillWidth: true
                }

                Slider {
                    from: 0
                    to: 1
                    value: specularGlossyMaterial.specularColor.b
                    onValueChanged: specularGlossyMaterial.specularColor.b = value
                }
            }
            MarkdownLabel {
                text: "### Specular (Map)
The Specular Map property specifies a texture to sample the Specular color value
from. When using a Specular Map the value sampled from the map file is multiplied
by the value of the Specular Color property. In practice this means that the maximum
Specular value possible will be the value set by the Specular map is the
value in the Specular color property. So most of the time when using a Specular
Map it will make sense to leave the value of Specular to 1.0.
    "
            }
            Button {
                text: "Reset Specular Color Value"
                onClicked: specularGlossyMaterial.specularColor = "white"
            }

            TextureSourceControl {
                defaultClearColor: "black"
                defaultTexture: "maps/metallic/metallic.jpg"
                onTargetTextureChanged: {
                    specularGlossyMaterial.specularMap = targetTexture
                }
            }

            VerticalSectionSeparator {}

            MarkdownLabel {
                text: "## Glossiness
The Glossiness property defines the amount of irregularities in the surface of
a material. The more irregular a surface is, the more the light reflecting off
of it will be spread and cause light diffusion. If the surface instead is smooth
with few surface irregularity then light will be reflected in a more focused and
intense manner. A value of 1.0 means that the surface is smooth and reflective as
a mirror, and a value of 0.0 means that the surface is so rough its hard for light
to be reflected at all."
            }
            RowLayout {
                Label {
                    text: "Glossiness (" + specularGlossyMaterial.glossiness.toFixed(2) + ")"
                    Layout.fillWidth: true
                }
                Slider {
                    from: 0
                    to: 1
                    value: specularGlossyMaterial.glossiness
                    onValueChanged: specularGlossyMaterial.glossiness = value
                }
            }
            MarkdownLabel {
                text: "### Glossiness (Map)
The Glossiness Map property specifies a texture to sample the Glossiness value
from. Since the Glossiness property is only a single floating point value between
0.0 and 1.0, it's only necessary to use a single color channel of the image, or
a greyscale image. By default SpecularGlossyMaterial will use the value in the alpha
channel of the texture, but it's possible to change which color channel is used.
    "
            }
            ComboBox {
                textRole: "text"
                valueRole: "value"
                implicitContentWidthPolicy: ComboBox.WidestText
                onActivated: specularGlossyMaterial.glossinessChannel = currentValue
                Component.onCompleted: currentIndex = indexOfValue(specularGlossyMaterial.glossinessChannel)
                model: [
                    { value: SpecularGlossyMaterial.R, text: "Red Channel"},
                    { value: SpecularGlossyMaterial.G, text: "Green Channel"},
                    { value: SpecularGlossyMaterial.B, text: "Blue Channel"},
                    { value: SpecularGlossyMaterial.A, text: "Alpha Channel"}
                ]
            }
            MarkdownLabel {
                text: "
When using a Glossiness Map the value sampled from the map file is multiplied by
the value of the Glossiness property. In practice this means that the maximum
Glossiness value possible will be the value set by the Glossiness map is the
value in the Glossiness property. So most of the time when using a Glossiness
Map it will make sense to leave the value of Glossiness to 1.0.
    "
            }
            Button {
                text: "Reset Glossiness Value"
                onClicked: specularGlossyMaterial.glossiness = 1.0
            }

            TextureSourceControl {
                defaultClearColor: "white"
                defaultTexture: "maps/metallic/roughness.jpg"
                onTargetTextureChanged: {
                    specularGlossyMaterial.glossinessMap = targetTexture
                }
            }
        }
    }
}
