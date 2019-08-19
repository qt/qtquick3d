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
        caption: qsTr("Scene Environment")
        width: parent.width
        SectionLayout {
            Label {
                text: qsTr("Progressive AA")
                tooltip: qsTr("Improves the visual quality when no\nitems are moving")
            }
            SecondColumnLayout {
                ComboBox {
                    scope: "SceneEnvironment"
                    model: ["NoAA", "SSAA", "X2", "X4", "X8"]
                    backendValue: backendValues.progressiveAAMode
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Multisample AA")
                tooltip: qsTr("Improves geometry quality, e.g. silhouettes.")
            }
            SecondColumnLayout {
                ComboBox {
                    scope: "SceneEnvironment"
                    model: ["NoAA", "SSAA", "X2", "X4", "X8"]
                    backendValue: backendValues.multisampleAAMode
                    Layout.fillWidth: true
                }
            }
            Label {
                text: "Temporal AA"
                tooltip: qsTr("Improve overall quality using camera jittering and frame blending")
            }
            SecondColumnLayout {
                CheckBox {
                    text: backendValues.temporalAAEnabled.valueToString
                    backendValue: backendValues.temporalAAEnabled
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Background Mode")
                tooltip: qsTr("How the scene be cleared")
            }
            SecondColumnLayout {
                ComboBox {
                    scope: "SceneEnvironment"
                    model: ["Transparent", "Unspecified", "Color"]
                    backendValue: backendValues.backgroundMode
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Blend Mode")
            }
            SecondColumnLayout {
                ComboBox {
                    scope: "SceneEnvironment"
                    model: ["Normal", "Screen", "Multiply", "Add", "Subtract", "Overlay", "ColorBurn", "ColorDodge"]
                    backendValue: backendValues.blendType
                    Layout.fillWidth: true
                }
            }

            Label {
                text: "Disabled Depth Test"
                tooltip: qsTr("Optimize render speed for layers with mostly transparent objects")
            }
            SecondColumnLayout {
                CheckBox {
                    text: backendValues.isDepthTestDisabled.valueToString
                    backendValue: backendValues.isDepthTestDisabled
                    Layout.fillWidth: true
                }
            }
            Label {
                text: "Disable Depth Prepass"
                tooltip: qsTr("Optimize render speed for layers with low depth complexity")
            }
            SecondColumnLayout {
                CheckBox {
                    text: backendValues.isDepthPrePassDisabled.valueToString
                    backendValue: backendValues.isDepthPrePassDisabled
                    Layout.fillWidth: true
                }
            }
        }
    }

    Section {
        caption: qsTr("Clear Color")
        width: parent.width
        ColorEditor {
            caption: qsTr("Clear Color")
            backendValue: backendValues.clearColor
            supportGradient: false
            Layout.fillWidth: true
        }
    }

    Section {
        caption: qsTr("Ambient Occlusion")
        width: parent.width

        SectionLayout {

            Label {
                text: qsTr("AO Strength")
                tooltip: qsTr("Amount of ambient occlusion shading to apply")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 100
                    minimumValue: 0
                    decimals: 0
                    backendValue: backendValues.aoStrength
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("AO Distance")
                tooltip: qsTr("Size of the ambient occlusion shading")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 99999
                    minimumValue: 0
                    decimals: 0
                    backendValue: backendValues.aoDistance
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("AO Softness")
                tooltip: qsTr("Magnitude of the blurring used to soften shading")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 50
                    minimumValue: 0
                    decimals: 0
                    backendValue: backendValues.aoSoftness
                    Layout.fillWidth: true
                }
            }

            Label {
                text: "AO Detail"
                tooltip: qsTr("Use close-range detail AO")
            }
            SecondColumnLayout {
                CheckBox {
                    text: backendValues.aoDither.valueToString
                    backendValue: backendValues.aoDither
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("AO Sampling Rate")
                tooltip: qsTr("Quality of AO sampling")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 4
                    minimumValue: 2
                    decimals: 0
                    backendValue: backendValues.aoSampleRate
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("AO Threshold")
                tooltip: qsTr("Remove AO from flat surfaces to prevent artifacts")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 999999
                    minimumValue: -999999
                    decimals: 2
                    backendValue: backendValues.aoBias
                    Layout.fillWidth: true
                }
            }
        }
    }

    Section {
        caption: qsTr("Shadows")
        width: parent.width
        SectionLayout {
            Label {
                text: qsTr("Shadow Strength")
                tooltip: qsTr("Amount of shadowing to apply")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 100
                    minimumValue: 0
                    decimals: 0
                    backendValue: backendValues.shadowStrength
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Shadow Distance")
                tooltip: qsTr("Maximum distance to ray march for shadows test")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: -999999
                    minimumValue: 999999
                    decimals: 2
                    backendValue: backendValues.shadowDistance
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Shadow Softness")
                tooltip: qsTr("Amount of softening of the shadow edges")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 100
                    minimumValue: 0
                    decimals: 0
                    backendValue: backendValues.shadowSoftness
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Shadow Threshold")
                tooltip: qsTr("Remove self-shadowing from flat surfaces")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 999999
                    minimumValue: -999999
                    decimals: 0
                    backendValue: backendValues.shadowBias
                    Layout.fillWidth: true
                }
            }
        }
    }

    Section {
        caption: qsTr("Image Based Lighting")
        width: parent.width
        SectionLayout {
            // ### lightProbe
            Label {
                text: qsTr("IBL Brightness")
                tooltip: qsTr("Amount of light emitted by the light probe")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 999999
                    minimumValue: -999999
                    decimals: 0
                    backendValue: backendValues.probeBrightness
                    Layout.fillWidth: true
                }
            }

            Label {
                text: "Fast IBL"
                tooltip: qsTr("Use a faster approximation to image-based lighting")
            }
            SecondColumnLayout {
                CheckBox {
                    text: backendValues.aoDither.valueToString
                    backendValue: backendValues.fastIBL
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("IBL Horizon Cutoff")
                tooltip: qsTr("Upper limit for horizon darkening of the light probe")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: -0.001
                    minimumValue: -1
                    decimals: 3
                    backendValue: backendValues.probeHorizon
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("IBL FOV Angle")
                tooltip: qsTr("Image source FOV for the case of using a camera-source as the IBL probe")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 180
                    minimumValue: 1.0
                    decimals: 1
                    backendValue: backendValues.probeFieldOfView
                    Layout.fillWidth: true
                }
            }

            // ### lightProbe2

            Label {
                text: qsTr("Probe Crossfade")
                tooltip: qsTr("Blend amount between the primary and\nseconary probes")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 1.0
                    minimumValue: 0
                    decimals: 2
                    backendValue: backendValues.probe2Fade
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Secondary Probe Window")
                tooltip: qsTr("Texture-U window size used for the moving window (for scrolling textures)")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 1.0
                    minimumValue: 0.01
                    decimals: 2
                    backendValue: backendValues.probe2Window
                    Layout.fillWidth: true
                }
            }


            Label {
                text: qsTr("Secondary Probe Offset")
                tooltip: qsTr("Texture-U window offset used for the moving window")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 999999
                    minimumValue: -999999
                    decimals: 2
                    backendValue: backendValues.probe2Position
                    Layout.fillWidth: true
                }
            }
        }
    }
}
