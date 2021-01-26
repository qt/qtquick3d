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
        caption: qsTr("Colors")
        width: parent.width

        SectionLayout {
            Label {
                text: qsTr("Red Strength")
                tooltip: qsTr("Red strength.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 2
                    minimumValue: 0
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.redStrength
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Green Strength")
                tooltip: qsTr("Green strength.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 2
                    minimumValue: 0
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.greenStrength
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Blue Strength")
                tooltip: qsTr("Blue strength.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 2
                    minimumValue: 0
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.blueStrength
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Saturation")
                tooltip: qsTr("Color saturation.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 1
                    minimumValue: -1
                    decimals: 2
                    stepSize: 0.1
                    backendValue: backendValues.saturation
                    Layout.fillWidth: true
                }
            }
        }
    }
}
