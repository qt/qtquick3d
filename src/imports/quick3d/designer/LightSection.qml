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

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.12

Column {
    width: parent.width

    Section {
        caption: qsTr("Light")
        width: parent.width

        SectionLayout {
            Label {
                text: qsTr("Light Type")
                tooltip: qsTr("Type of illumination to use")
            }
            ComboBox {
                id: lightTypeComboBox
                scope: "Light"
                model: ["Directional", "Point", "Area"]
                backendValue: backendValues.lightType
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Brightness")
                tooltip: qsTr("Strength of the light")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 9999999
                    minimumValue: -9999999
                    realDragRange: 5000
                    decimals: 0
                    backendValue: backendValues.brightness
                    Layout.fillWidth: true
                }
            }

            // ### only for Point Lights
            Label {
                text: qsTr("Linear Fade")
                tooltip: qsTr("Falloff of the point light")
            }
            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                    backendValue: backendValues.linearFade
                    Layout.fillWidth: true
                    enabled: lightTypeComboBox.backendValue.enumeration === "Point"
                }
            }

            // ### only for Point Lights
            Label {
                text: qsTr("Exponential Fade")
                tooltip: qsTr("Additional falloff")
            }
            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                    backendValue: backendValues.exponentialFade
                    Layout.fillWidth: true
                    enabled: lightTypeComboBox.backendValue.enumeration === "Point"
                }
            }

            // ### only for Area Lights
            Label {
                text: qsTr("Area Width")
                tooltip: qsTr("Width of the surface of the area light")
            }
            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 100
                    decimals: 0
                    backendValue: backendValues.areaWidth
                    Layout.fillWidth: true
                    enabled: lightTypeComboBox.backendValue.enumeration === "Area"
                }
            }

            // ### only for Area Lights
            Label {
                text: qsTr("Area Height")
                tooltip: qsTr("Height of the surface of the area light")
            }
            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 100
                    decimals: 0
                    backendValue: backendValues.areaHeight
                    Layout.fillWidth: true
                    enabled: lightTypeComboBox.backendValue.enumeration === "Area"
                }
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

    Section {
        caption: qsTr("Emissive Color")
        width: parent.width
        ColorEditor {
            caption: qsTr("Emissive Color")
            backendValue: backendValues.emissiveColor
            supportGradient: false
            Layout.fillWidth: true
        }
    }

    Section {
        caption: qsTr("Specular Tint Color")
        width: parent.width
        ColorEditor {
            caption: qsTr("Specular Tint Color")
            backendValue: backendValues.specularTint
            supportGradient: false
            Layout.fillWidth: true
        }
    }

    Section {
        caption: qsTr("Shadows")
        width: parent.width

        SectionLayout {

//            Label {
//                text: "Casts Shadow"
//                tooltip: qsTr("Enable shadow casting for this light")
//            }
//            SecondColumnLayout {
//                CheckBox {
//                    id: shadowCheckBox
//                    text: backendValues.castShadow.valueToString
//                    backendValue: backendValues.castShadow
//                    Layout.fillWidth: true
//                }
//            }

            // ### all the following should only be shown when shadows are enabled
            Label {
                text: qsTr("Shadow Darkness")
                tooltip: qsTr("Factor used to darken shadows")
            }
            SecondColumnLayout {
                SpinBox {
                    minimumValue: 1.0
                    maximumValue: 100.0
                    decimals: 0
                    backendValue: backendValues.shadowFactor
                    Layout.fillWidth: true
                    enabled: shadowCheckBox.backendValue.value === true
                }
            }

            Label {
                text: qsTr("Shadow Softness")
                tooltip: qsTr("Width of the blur filter on the shadow map")
            }
            SecondColumnLayout {
                SpinBox {
                    minimumValue: 1.0
                    maximumValue: 100.0
                    decimals: 0
                    backendValue: backendValues.shadowFilter
                    Layout.fillWidth: true
                    enabled: shadowCheckBox.backendValue.value === true
                }
            }

            Label {
                text: qsTr("Shadow Resolution")
                tooltip: qsTr("Resolution of shadow map (powers of two)")
            }
            SecondColumnLayout {
                ComboBox {
                    model: [7, 8, 9, 10, 11, 12]
                    backendValue: backendValues.shadowMapResolution
                    Layout.fillWidth: true
                    enabled: shadowCheckBox.backendValue.value === true
                }
            }

            Label {
                text: qsTr("Shadow Depth Bias")
                tooltip: qsTr("Slight offset to avoid self-shadowing artifacts")
            }
            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 100
                    decimals: 0
                    backendValue: backendValues.shadowBias
                    Layout.fillWidth: true
                    enabled: shadowCheckBox.backendValue.value === true
                }
            }

            Label {
                text: qsTr("Shadow Far Clip")
                tooltip: qsTr("Affects the maximum distance for the shadow depth map")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 9999999
                    minimumValue: -9999999
                    realDragRange: 5000
                    decimals: 0
                    backendValue: backendValues.shadowMapFar
                    Layout.fillWidth: true
                    enabled: shadowCheckBox.backendValue.value === true
                }
            }

            Label {
                text: qsTr("Shadow Field of View")
                tooltip: qsTr("Affects the field of view of the shadow camera")
            }
            SecondColumnLayout {
                SpinBox {
                    minimumValue: 1.0
                    maximumValue: 179.0
                    decimals: 0
                    backendValue: backendValues.shadowMapFieldOfView
                    Layout.fillWidth: true
                    enabled: shadowCheckBox.backendValue.value === true
                }
            }
        }
    }
}
