// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Particle Scale Affector")
    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: qsTr("Minimum Size")
            tooltip: qsTr("This property defines the minimum scale size.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 999999
                decimals: 2
                backendValue: backendValues.minSize
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Maximum Size")
            tooltip: qsTr("This property defines the maximum scale size.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 999999
                decimals: 2
                backendValue: backendValues.maxSize
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Duration")
            tooltip: qsTr("This property defines the duration of scaling period.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 999999
                decimals: 0
                backendValue: backendValues.duration
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Easing curve")
            tooltip: qsTr("Defines a custom scaling curve.")
        }

        SecondColumnLayout {
            BoolButtonRowButton {
                buttonIcon: StudioTheme.Constants.curveDesigner

                EasingCurveEditor {
                    id: easingCurveEditor
                    modelNodeBackendProperty: modelNodeBackend
                }

                onClicked: easingCurveEditor.runDialog()
            }

            ExpandingSpacer {}
        }
    }
}
