/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
******************************************************************************/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Distortion")
    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: qsTr("Radius")
            tooltip: qsTr("Radius of the effect.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 1
                decimals: 2
                stepSize: 0.1
                backendValue: backendValues.radius
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Strength")
            tooltip: qsTr("Strength of the distortion.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: -10
                maximumValue: 10
                decimals: 2
                backendValue: backendValues.distortionStrength
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Center")
            tooltip: qsTr("Center of the distortion.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 1
                decimals: 2
                stepSize: 0.1
                backendValue: backendValues.center_x
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                            + StudioTheme.Values.actionIndicatorWidth
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel { text: "X" }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            SpinBox {
                minimumValue: 0
                maximumValue: 1
                decimals: 2
                stepSize: 0.1
                backendValue: backendValues.center_y
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                            + StudioTheme.Values.actionIndicatorWidth
            }

            Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

            ControlLabel { text: "Y" }

            Spacer { implicitWidth: StudioTheme.Values.controlGap }

            ExpandingSpacer {}
        }
    }
}
