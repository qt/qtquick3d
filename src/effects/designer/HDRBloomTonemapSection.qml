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
    caption: qsTr("Tonemap")
    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: qsTr("Gamma")
            tooltip: qsTr("Amount of gamma.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0.1
                maximumValue: 4
                decimals: 2
                stepSize: 0.1
                backendValue: backendValues.gamma
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Exposure")
            tooltip: qsTr("Amount of exposure.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: -9
                maximumValue: 9
                decimals: 2
                backendValue: backendValues.exposure
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Blur Falloff")
            tooltip: qsTr("Amount of blur falloff.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 10
                decimals: 2
                backendValue: backendValues.blurFalloff
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Tonemapping Lerp")
            tooltip: qsTr("Tonemapping linear interpolation value.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 1
                decimals: 2
                stepSize: 0.1
                backendValue: backendValues.tonemappingLerp
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Bloom Threshold")
            tooltip: qsTr("Bloom color threshold value.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 1
                decimals: 3
                stepSize: 0.1
                backendValue: backendValues.bloomThreshold
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Channel Threshold")
            tooltip: qsTr("Channel color threshold value.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 1
                decimals: 3
                stepSize: 0.1
                backendValue: backendValues.channelThreshold
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
