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
        caption: qsTr("Blur")
        width: parent.width

        SectionLayout {
            Label {
                text: qsTr("Blur Amount")
                tooltip: qsTr("Amount of blur.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 50
                    minimumValue: 0
                    decimals: 2
                    backendValue: backendValues.blurAmount
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Focus Distance")
                tooltip: qsTr("Focus distance of the blur.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 5000
                    minimumValue: 0
                    decimals: 0
                    backendValue: backendValues.focusDistance
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Focus Range")
                tooltip: qsTr("Focus range of the blur.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 5000
                    minimumValue: 0
                    decimals: 0
                    backendValue: backendValues.focusRange
                    Layout.fillWidth: true
                }
            }
        }
    }
}
