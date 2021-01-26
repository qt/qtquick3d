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
        caption: qsTr("Tilt Shift")
        width: parent.width

        SectionLayout {
            Label {
                text: qsTr("Focus Position")
                tooltip: qsTr("Set the focus position.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 1
                    minimumValue: 0
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.focusPosition
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Focus Width")
                tooltip: qsTr("Set the focus width.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 1
                    minimumValue: 0
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.focusWidth
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Blur Amount")
                tooltip: qsTr("Set the blur amount.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 10
                    minimumValue: 0
                    decimals: 2
                    backendValue: backendValues.blurAmount
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Vertical")
                tooltip: qsTr("Specifies if the tilt shift is vertical.")
            }
            SecondColumnLayout {
                CheckBox {
                    text: backendValues.isVertical.valueToString
                    backendValue: backendValues.isVertical
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Inverted")
                tooltip: qsTr("Specifies if the tilt shift is inverted.")
            }
            SecondColumnLayout {
                CheckBox {
                    text: backendValues.isInverted.valueToString
                    backendValue: backendValues.isInverted
                    Layout.fillWidth: true
                }
            }
        }
    }
}
