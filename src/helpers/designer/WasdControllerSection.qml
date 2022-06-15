// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Column {
    width: parent.width

    Section {
        width: parent.width
        caption: qsTr("WASD Controller")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Controlled Node")
                tooltip: qsTr("The 3D node controlled by this controller.")
            }

            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QtQuick3D.Node"
                    backendValue: backendValues.controlledObject
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Invert X")
                tooltip: qsTr("Invert X-axis controls.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: qsTr("Enabled")
                    backendValue: backendValues.xInvert
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Invert Y")
                tooltip: qsTr("Invert Y-axis controls.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: qsTr("Enabled")
                    backendValue: backendValues.yInvert
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Mouse Control")
                tooltip: qsTr("Use mouse to control the target node.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: qsTr("Enabled")
                    backendValue: backendValues.mouseEnabled
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Keyboard Control")
                tooltip: qsTr("Use keyboard to control the target node.")
            }

            SecondColumnLayout {
                CheckBox {
                    text: qsTr("Enabled")
                    backendValue: backendValues.keysEnabled
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            // TODO: acceptedButtons has no control as there is currently no support for a flags
            // type of property control in QDS.
        }
    }

    Section {
        width: parent.width
        caption: qsTr("Speeds")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Speed")
                tooltip: qsTr("General navigation speed multiplier.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 999999
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.speed
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Shift Speed")
                tooltip: qsTr("Navigation speed multiplier when the Shift key is pressed.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 999999
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.shiftSpeed
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Forward Speed")
                tooltip: qsTr("Navigation speed when forward key is pressed.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 999999
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.forwardSpeed
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Back Speed")
                tooltip: qsTr("Navigation speed when back key is pressed.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 999999
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.backSpeed
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Right Speed")
                tooltip: qsTr("Navigation speed when right key is pressed.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 999999
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.rightSpeed
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Left Speed")
                tooltip: qsTr("Navigation speed when left key is pressed.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 999999
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.leftSpeed
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Up Speed")
                tooltip: qsTr("Navigation speed when up key is pressed.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 999999
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.upSpeed
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Down Speed")
                tooltip: qsTr("Navigation speed when down key is pressed.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 999999
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.downSpeed
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("X Speed")
                tooltip: qsTr("Navigation speed when mouse is moved along X-axis.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 999999
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.xSpeed
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Y Speed")
                tooltip: qsTr("Navigation speed when mouse is moved along Y-axis.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 999999
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.ySpeed
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }
}
