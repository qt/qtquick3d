/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.12
import HelperWidgets 2.0
import QtQuick.Layouts 1.12

Column {
    width: parent.width
    Section {
        caption: qsTr("Default Material")
        width: parent.width
        SectionLayout {
            Label {
                text: qsTr("Lighting")
                tooltip: qsTr("Light model")
            }
            ComboBox {
                scope: "DefaultMaterial"
                model: ["NoLighting", "VertexLighting", "FragmentLighting"]
                backendValue: backendValues.lighting
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("Blending Mode")
                tooltip: qsTr("How this material blends with content behind it.")
            }
            ComboBox {
                scope: "DefaultMaterial"
                model: ["Normal", "Screen", "Multiply", "Overlay", "ColorBurn", "ColorDodge" ]
                backendValue: backendValues.blendMode
                Layout.fillWidth: true
            }
        }
    }
    Section {
        caption: qsTr("Diffuse Color")
        width: parent.width
        ColorEditor {
            caption: qsTr("Diffuse Color")
            backendValue: backendValues.diffuseColor
            supportGradient: false
            Layout.fillWidth: true
        }
    }

    // ### diffuseMap
    // ### diffuseMap2
    // ### diffuseMap3

    Section {
        caption: qsTr("Emissive")
        width: parent.width
        SectionLayout {
            Label {
                text: qsTr("Emissive Power")
                tooltip: qsTr("Amount of self-illumination for this material. (will not light other objects)")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 9999999
                    minimumValue: -9999999
                    decimals: 0
                    backendValue: backendValues.emissivePower
                    Layout.fillWidth: true
                }
            }
            // ### emissiveMap
        }
    }

    Section {
        caption: "Emissive Color"
        width: parent.width
        ColorEditor {
            caption: qsTr("Emissive Color")
            backendValue: backendValues.emissiveColor
            supportGradient: false
            Layout.fillWidth: true
        }
    }

    // ### specularReflectionMap
    // ### specularMap
    Section {
        caption: "Specular Tint"
        width: parent.width
        ColorEditor {
            caption: qsTr("Specular Tint")
            backendValue: backendValues.specularTint
            supportGradient: false
            Layout.fillWidth: true
        }
    }

    Section {
        caption: "Specular"
        width: parent.width
        SectionLayout {
            Label {
                text: qsTr("Specular Model")
                tooltip: qsTr("Equation to use when calculating specular highlights for CG lights")
            }
            ComboBox {
                scope: "DefaultMaterial"
                model: ["Default", "KGGX", "KWard"]
                backendValue: backendValues.specularModel
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("Index of Refraction")
                tooltip: qsTr("Index of refraction of the material")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 9999999
                    minimumValue: 1
                    decimals: 2
                    backendValue: backendValues.indexOfRefraction
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Fresnel Power")
                tooltip: qsTr("Damping of head-on reflections")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 9999999
                    minimumValue: -9999999
                    decimals: 2
                    backendValue: backendValues.fresnelPower
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Specular Amount")
                tooltip: qsTr("Amount of shine/gloss")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 9999999
                    minimumValue: -9999999
                    decimals: 2
                    backendValue: backendValues.specularAmount
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Specular Roughness")
                tooltip: qsTr("Softening applied to reflections and highlights")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 1
                    minimumValue: 0.001
                    decimals: 2
                    backendValue: backendValues.specularRoughness
                    Layout.fillWidth: true
                }
            }
            // ### roughnessMap
        }
    }

    Section {
        caption: "Opacity"
        width: parent.width
        SectionLayout {
            Label {
                text: qsTr("Opacity")
                tooltip: qsTr("Visibility of the geometry for this material.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 1
                    minimumValue: 0
                    decimals: 2
                    backendValue: backendValues.opacity
                    Layout.fillWidth: true
                }
            }

            // ### opacityMap
        }
    }

    Section {
        caption: "Bump"
        width: parent.width
        SectionLayout {
            Label {
                text: qsTr("Bump Amount")
                tooltip: qsTr("Strength of bump/normal map effect")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 999999
                    minimumValue: -999999
                    decimals: 2
                    backendValue: backendValues.bumpAmount
                    Layout.fillWidth: true
                }
            }
            // ### blumpMap

        }
    }

    // ### normapMap

    Section {
        caption: "Translucency"
        width: parent.width
        SectionLayout {
            Label {
                text: qsTr("Translucency Falloff")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 999999
                    minimumValue: -999999
                    decimals: 2
                    backendValue: backendValues.translucentFalloff
                    Layout.fillWidth: true
                }
            }
        }
    }

    Section {
        caption: "Diffuse Light Wrap"
        width: parent.width
        SectionLayout {
            Label {
                text: qsTr("Diffuse Light Wrap")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 1
                    minimumValue: 0
                    decimals: 2
                    backendValue: backendValues.diffuseLightWrap
                    Layout.fillWidth: true
                }
            }
        }
    }

    Section {
        caption: "Vertex Colors"
        width: parent.width
        SectionLayout {
            Label {
                text: "Enable Vertex Colors"
                tooltip: qsTr("Use vertex colors from the mesh")
            }
            SecondColumnLayout {
                CheckBox {
                    text: backendValues.vertexColors.valueToString
                    backendValue: backendValues.vertexColors
                    Layout.fillWidth: true
                }
            }
        }
    }
}
