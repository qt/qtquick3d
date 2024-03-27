// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D

// qmllint disable missing-property
// Disabling missing-property because the targetMaterial property
// will either be a PrincipaledMaterial or SpecularGlossyMaterial
// but the shared properties are not part of the common base class
ScrollView {
    id: rootView
    required property Material targetMaterial
    property bool specularGlossyMode: false

    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    width: availableWidth

    ColumnLayout {
        width: rootView.availableWidth
        MarkdownLabel {
            text: `# Vertex Color
You can control the usage of the mesh vertex colors here.`
        }

        RowLayout {
            Label {
                text: "Enabled use the vertex color as base color for materials"
            }

            Switch {
                checked: targetMaterial.vertexColorsEnabled
                onCheckedChanged: {
                    targetMaterial.vertexColorsEnabled = checked
                }
            }

        }

        MarkdownLabel {
            text: `# Vertex Color as Mask
You can defines the vertex color channels used as the specifies mask.`
        }
        RowLayout {
            Label {
                text: "Enabled use the vertex color as mask for materials"
            }
            Switch {
                id: vertexColorMaskSwitch
                checked: targetMaterial.vertexColorsMaskEnabled
                onCheckedChanged: {
                    targetMaterial.vertexColorsMaskEnabled = checked
                }
            }
        }

        MarkdownLabel {
            enabled: vertexColorMaskSwitch.checked
            text: `## Vertex Color Red Channel Mask Flags
You can add flags to the vertex color red channel mask to get used as the specifies mask.`
        }

        VertexColorMaskFlagsControl {
            enabled: vertexColorMaskSwitch.checked
            specularGlossyMode: rootView.specularGlossyMode
            onSpecularGlossyMaterialMaskChanged: {
                targetMaterial.vertexColorRedMask = specularGlossyMaterialMask
            }
            onPrincipledMaterialMaskChanged: {
                targetMaterial.vertexColorRedMask = principledMaterialMask
            }
        }

        MarkdownLabel {
            enabled: vertexColorMaskSwitch.checked
            text: `## Vertex Color Green Channel Mask Flags
You can add flags to the vertex color green channel mask to get used as the specifies mask.`
        }

        VertexColorMaskFlagsControl {
            enabled: vertexColorMaskSwitch.checked
            specularGlossyMode: rootView.specularGlossyMode
            onSpecularGlossyMaterialMaskChanged: {
                targetMaterial.vertexColorGreenMask = specularGlossyMaterialMask
            }
            onPrincipledMaterialMaskChanged: {
                targetMaterial.vertexColorGreenMask = principledMaterialMask
            }
        }

        MarkdownLabel {
            enabled: vertexColorMaskSwitch.checked
            text: `## Vertex Color Blue Channel Mask Flags
You can add flags to the vertex color blue channel mask to get used as the specifies mask.`
        }

        VertexColorMaskFlagsControl {
            enabled: vertexColorMaskSwitch.checked
            specularGlossyMode: rootView.specularGlossyMode
            onSpecularGlossyMaterialMaskChanged: {
                targetMaterial.vertexColorBlueMask = specularGlossyMaterialMask
            }
            onPrincipledMaterialMaskChanged: {
                targetMaterial.vertexColorBlueMask = principledMaterialMask
            }
        }

        MarkdownLabel {
            enabled: vertexColorMaskSwitch.checked
            text: `## Vertex Color Alpha Channel Mask Flags
You can add flags to the vertex color alpha channel mask to get used as the specifies mask.`
        }

        VertexColorMaskFlagsControl {
            enabled: vertexColorMaskSwitch.checked
            specularGlossyMode: rootView.specularGlossyMode
            onSpecularGlossyMaterialMaskChanged: {
                targetMaterial.vertexColorAlphaMask = specularGlossyMaterialMask
            }
            onPrincipledMaterialMaskChanged: {
                targetMaterial.vertexColorAlphaMask = principledMaterialMask
            }
        }
    }
}
// qmllint enable missing-property
