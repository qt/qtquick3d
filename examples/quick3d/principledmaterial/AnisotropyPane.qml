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
            text: `# Anisotropy
Anisotropy stretches the specular highlight along one direction of the
surface. Brushed metal, hair, and the grooves of a vinyl record all reflect
this way: the microscopic detail runs in a direction, so the reflection smears
across it instead of staying round.

Anisotropy is only available on PrincipledMaterial.${rootView.specularGlossyMode ? "\n\n**The scene is currently showing the SpecularGlossyMaterial, so switch back to the PrincipledMaterial in the Basics tab to see these controls take effect.**" : ""}

Unlike sheen or clearcoat this is not an extra layer of reflection. It changes
the shape of the existing specular highlight, so it is most visible on a
smooth, metallic material and barely visible on a rough dielectric one.

## Anisotropy Strength
At \`0.0\`, the default, the highlight is round and the material renders exactly
as it would without anisotropy. Raising the value stretches the highlight
further, until at \`1.0\` it becomes a band across the surface.
`
        }

        RowLayout {
            Label {
                text: "Anisotropy Strength (" + rootView.targetMaterial.anisotropyStrength.toFixed(2) + ")"
                Layout.fillWidth: true
            }
            Slider {
                from: 0
                to: 1
                value: rootView.targetMaterial.anisotropyStrength
                onValueChanged: rootView.targetMaterial.anisotropyStrength = value
            }
        }

        MarkdownLabel {
            text: `## Anisotropy Rotation
This turns the direction the highlight is stretched along, in degrees, within
the tangent plane of the surface. At \`0\` the highlight follows the tangent of
the mesh, which for most models means it follows the horizontal direction of
the texture coordinates.

Note that the direction is only as meaningful as the tangent frame it is
measured against. A mesh that carries tangent data, or a material with a
Normal Map, has a well defined tangent frame. Without either, Qt Quick 3D
derives a stable frame from the surface normal instead, and the highlight will
still stretch but in a direction that was not chosen by anyone.
`
        }

        RowLayout {
            Label {
                text: "Anisotropy Rotation (" + rootView.targetMaterial.anisotropyRotation.toFixed(1) + "°)"
                Layout.fillWidth: true
            }
            Slider {
                from: 0
                to: 360
                value: rootView.targetMaterial.anisotropyRotation
                onValueChanged: rootView.targetMaterial.anisotropyRotation = value
            }
        }

        MarkdownLabel {
            text: `## Anisotropy Map
The Anisotropy Map is the one map with a fixed channel layout, so there is no
channel selector to go with it. The red and green channels hold the direction
in tangent space, remapped from \`0.0\`-\`1.0\` to \`-1.0\`-\`1.0\`, and the blue
channel scales the Anisotropy Strength. That lets a single texture both aim and
fade the effect, which is how a brushed metal texture describes its grain.

Since the blue channel scales the strength, this map does nothing while
Anisotropy Strength is \`0.0\`.
`
        }

        TextureSourceControl {
            defaultTexture: "maps/noise.png"
            defaultClearColor: "black"
            onTargetTextureChanged: {
                rootView.targetMaterial.anisotropyMap = targetTexture
            }
        }
    }
}
