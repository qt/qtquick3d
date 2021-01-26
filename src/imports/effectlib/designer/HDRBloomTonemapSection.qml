/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.12

Column {
    width: parent.width

    Section {
        caption: qsTr("Tonemap")
        width: parent.width

        SectionLayout {
            Label {
                text: qsTr("Gamma")
                tooltip: qsTr("Amount of gamma.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 4
                    minimumValue: 0.1
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.gamma
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Exposure")
                tooltip: qsTr("Amount of exposure.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 9
                    minimumValue: -9
                    decimals: 2
                    backendValue: backendValues.exposure
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Blur Falloff")
                tooltip: qsTr("Amount of blur falloff.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 10
                    minimumValue: 0
                    decimals: 2
                    backendValue: backendValues.blurFalloff
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Tonemapping Lerp")
                tooltip: qsTr("Tonemapping linear interpolation value.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 1
                    minimumValue: 0
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.tonemappingLerp
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Bloom Threshold")
                tooltip: qsTr("Bloom color threshold value.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 1
                    minimumValue: 0
                    decimals: 3
                    stepSize: 0.1
                    backendValue: backendValues.bloomThreshold
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Channel Threshold")
                tooltip: qsTr("Channel color threshold value.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 1
                    minimumValue: 0
                    decimals: 3
                    stepSize: 0.1
                    backendValue: backendValues.channelThreshold
                    Layout.fillWidth: true
                }
            }
        }
    }
}
