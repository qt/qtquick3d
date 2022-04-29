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
    caption: qsTr("Particle System")
    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: qsTr("Start Time")
            tooltip: qsTr("This property defines time in milliseconds where the system starts.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 2147483647
                decimals: 0
                backendValue: backendValues.startTime
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Time")
            tooltip: qsTr("This property defines time in milliseconds for the system.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 2147483647
                decimals: 0
                backendValue: backendValues.time
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Running")
            tooltip: qsTr("This property defines if system is currently running.")
        }

        SecondColumnLayout {
            CheckBox {
                id: runningCheckBox
                text: backendValues.running.valueToString
                backendValue: backendValues.running
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Paused")
            tooltip: qsTr("This property defines if system is currently paused.")
        }

        SecondColumnLayout {
            CheckBox {
                id: pausedCheckBox
                text: backendValues.paused.valueToString
                backendValue: backendValues.paused
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Logging")
            tooltip: qsTr("Set this to true to collect loggingData.")
        }

        SecondColumnLayout {
            CheckBox {
                id: loggingCheckBox
                text: backendValues.logging.valueToString
                backendValue: backendValues.logging
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Use Random Seed")
            tooltip: qsTr("This property defines if particle system seed should be random or user defined.")
        }

        SecondColumnLayout {
            CheckBox {
                id: useRandomSeedCheckBox
                text: backendValues.useRandomSeed.valueToString
                backendValue: backendValues.useRandomSeed
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Seed")
            tooltip: qsTr("This property defines the seed value used for particles randomization.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0
                maximumValue: 2147483647
                decimals: 0
                backendValue: backendValues.seed
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
