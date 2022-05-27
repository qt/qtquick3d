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
    required property Material targetMaterial
    required property Model linesModel
    required property Model pointsModel
    property bool specularGlossyMode: false

    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    width: availableWidth

    ColumnLayout {
        width: rootView.availableWidth
        MarkdownLabel {
            text: "# Special Stuff
The content in this section is as titled \"Special\" because you should really
only use this stuff for fairly specific reasons.
## Lighting Mode
The lighting mode property defines how the material handles the shading of
lights. The default mode is *Fragment Lighting* which perform all light shading
in the fragment shader porsion of the graphics pipeline. In this case all both
image based lighting as well as any punctual lights are used to shade the material.
The other case *No Lighting* which ignores all light sources in the scene and
the material is rendered without shading.  This mode can be useful with models
that should not be shaded, for example tools (gizmos) inside of a 3D editor.
"
        }

        ComboBox {
            id: lightingComboBox
            textRole: "text"
            valueRole: "value"
            implicitContentWidthPolicy: ComboBox.WidestText
            onActivated: targetMaterial.lighting = currentValue
            Component.onCompleted: currentIndex = indexOfValue(targetMaterial.lighting)
            model: [
                { value: PrincipledMaterial.NoLighting, text: "No Lighting"},
                { value: PrincipledMaterial.FragmentLighting, text: "Fragment Lighting"}
            ]
        }

        VerticalSectionSeparator {}

        MarkdownLabel {
            text: "## Non Triangle Primitive Sizes
Generally if you use the Mesh file format baked by balsam, or the runtime
loader for loading glTF2 scenes, the geometry primitive will be individual
triangles, so these properties are not useful. However it is possible to
create your own mesh geometry where the primitive is instead either Points
or Lines, and in these cases it is possible to change the size of these
primitives when rendered. These properties will only have an affect on a
material when the geometry primitive is set to Line or Point respectively.
The behavior of these properties will depend on which graphics API the is
being used, and is possible that they will do nothing at all."
        }

        Button {
            text: checked ? "Hide Wireframe Model" : "Show Wireframe Model"
            checkable: true
            checked: linesModel.visible
            onClicked: {
                linesModel.visible = !linesModel.visible
            }
        }

        RowLayout {
            Label {
                text: "Line Width (" + targetMaterial.lineWidth.toFixed(2) + ")"
                Layout.fillWidth: true
            }
            Slider {
                from: 0
                to: 25
                value: targetMaterial.lineWidth
                onValueChanged: targetMaterial.lineWidth = value
            }
        }

        Button {
            text: checked ? "Hide Point Cloud" : "Show Point Cloud"
            checkable: true
            checked: pointsModel.visible
            onClicked: {
                pointsModel.visible = !pointsModel.visible
            }
        }

        RowLayout {
            Label {
                text: "Point Size (" + targetMaterial.pointSize.toFixed(2) + ")"
                Layout.fillWidth: true
            }
            Slider {
                from: 0
                to: 25
                value: targetMaterial.pointSize
                onValueChanged: targetMaterial.pointSize = value
            }
        }

        ColumnLayout {
            width: rootView.availableWidth
            visible: !specularGlossyMode
            VerticalSectionSeparator {}

            MarkdownLabel {
                text: "## Specular Overrides
If you are using this material for Physically Based Rendering (PBR) then
these properties should not be touched as they are not energy conserving and
will make materials look less realistic. These exist for adjusting the material
for artistic or stylistic reasons."
            }

            MarkdownLabel {
                text: "### Specular Amount
This Specular Amount property controls the strength of specularity (highlights
and reflections) of a dielectric material (non-metallic). Normally this should
be left at 1.0, but can be adjusted to reduce the specularity. Adjusting this
property will not affect reflections caused by Image Based Lighting."
            }
            RowLayout {
                Label {
                    text: "Specular Amount  (" + targetMaterial.specularAmount.toFixed(2) + ")"
                    Layout.fillWidth: true
                }
                Slider {
                    from: 0
                    to: 1
                    value: targetMaterial.specularAmount
                    onValueChanged: targetMaterial.specularAmount = value
                }
            }

            MarkdownLabel {
                text: "### Specular Tint
The Specular Tint property defines how much of the Base Color of the material
contributes to specular reflections.  This property only has an affect with
dielectric materials (non-metallic) as metallic material's specular reflections
are already primarily tinted by the materials base color. This property allows
the Base Color to also tint the specular reflections for dielectrics as well for
artistic reasons."
            }
            RowLayout {
                Label {
                    text: "Specular Tint (" + targetMaterial.specularTint.toFixed(2) + ")"
                    Layout.fillWidth: true
                }
                Slider {
                    from: 0
                    to: 1
                    value: targetMaterial.specularTint
                    onValueChanged: targetMaterial.specularTint = value
                }
            }

            MarkdownLabel {
                text: "### Specular Map
The Specular Map property defines color (RGB) texture to modulate the amount
and the color of specularity across the surface of the material. These values
are multiplied by the Specular Amount property. This property will only have
an affect if the material is a dielectric (non-metallic)."
            }

            TextureSourceControl {
                defaultClearColor: "black"
                defaultTexture: "maps/metallic/metallic.jpg"
                onTargetTextureChanged: {
                    targetMaterial.specularMap = targetTexture
                }
            }

            MarkdownLabel {
                text: "### Specular Reflection Map
The Specular Reflection Map property is for providing a static environment map
for cheap reflection calculations.  Unlike most properties which take textures
in PrincipledMaterial, this property requires the source Texture to use the
EnvironmentMap mapping mode to look correct. In addition this means that the
source image file should be an equirectangular projection. This is an alternative
to using Imaged Based Lighting via lightProbes to get static reflections."
            }

            TextureSourceControl {
                defaultClearColor: "black"
                defaultTexture: "maps/small_envmap.jpg"
                envMapMode: true
                onTargetTextureChanged: {
                    targetMaterial.specularReflectionMap = targetTexture
                }
            }
        }
    }

}
