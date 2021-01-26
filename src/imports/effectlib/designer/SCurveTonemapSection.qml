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
        caption: qsTr("Curve")
        width: parent.width

        SectionLayout {
            Label {
                text: qsTr("Shoulder Slope")
                tooltip: qsTr("Set the slope of the curve shoulder.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 3
                    minimumValue: 0
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.shoulderSlope
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Shoulder Emphasis")
                tooltip: qsTr("Set the emphasis of the curve shoulder.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 1
                    minimumValue: -1
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.shoulderEmphasis
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Toe Slope")
                tooltip: qsTr("Set the slope of the curve toe.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 3
                    minimumValue: 0
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.toeSlope
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Toe Emphasis")
                tooltip: qsTr("Set the emphasis of the curve toe.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 1
                    minimumValue: -1
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.toeEmphasis
                    Layout.fillWidth: true
                }
            }
        }
    }
    Section {
        caption: qsTr("Color")
        width: parent.width

        SectionLayout {
            Label {
                text: qsTr("Contrast Boost")
                tooltip: qsTr("Set the contrast boost amount.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 2
                    minimumValue: -1
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.contrastBoost
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Saturation Level")
                tooltip: qsTr("Set the color saturation level.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 2
                    minimumValue: 0
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.saturationLevel
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Gamma")
                tooltip: qsTr("Set the gamma value.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 8
                    minimumValue: 0.1
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.gammaValue
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Use Exposure")
                tooltip: qsTr("Specifies if the exposure or white point should be used.")
            }
            SecondColumnLayout {
                CheckBox {
                    text: backendValues.useExposure.valueToString
                    backendValue: backendValues.useExposure
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("White Point")
                tooltip: qsTr("Set the white point value.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 128
                    minimumValue: 0.01
                    decimals: 2
                    backendValue: backendValues.whitePoint
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Exposure")
                tooltip: qsTr("Set the exposure value.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 16
                    minimumValue: 0.01
                    decimals: 2
                    backendValue: backendValues.exposureValue
                    Layout.fillWidth: true
                }
            }
        }
    }
}
