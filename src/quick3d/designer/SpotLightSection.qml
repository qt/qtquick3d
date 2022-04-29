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
    caption: qsTr("Spot Light")
    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: qsTr("Constant Fade")
            tooltip: qsTr("Sets the constant attenuation of the light.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 10
                decimals: 2
                stepSize: 0.1
                backendValue: backendValues.constantFade
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Linear Fade")
            tooltip: qsTr("Sets the linear attenuation of the light.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 10
                decimals: 2
                stepSize: 0.1
                backendValue: backendValues.linearFade
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Quadratic Fade")
            tooltip: qsTr("Sets the quadratic attenuation of the light.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 10
                decimals: 2
                stepSize: 0.1
                backendValue: backendValues.quadraticFade
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Cone Angle")
            tooltip: qsTr("Sets the angle of the light cone.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 180
                decimals: 2
                backendValue: backendValues.coneAngle
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Inner Cone Angle")
            tooltip: qsTr("Sets the angle of the inner light cone.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 180
                decimals: 2
                backendValue: backendValues.innerConeAngle
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
