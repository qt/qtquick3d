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
        caption: qsTr("Set Uniform Value")
        width: parent.width

        SectionLayout {
            Label {
                text: qsTr("Target")
                tooltip: qsTr("The name of the uniform to change value for a pass.")
            }
            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.target
                    Layout.fillWidth: true
                    showTranslateCheckBox: false
                }
            }
            Label {
                text: qsTr("Value")
                tooltip: qsTr("The value of the uniform.")
            }
            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.value
                    Layout.fillWidth: true
                    showTranslateCheckBox: false
                    writeAsExpression: true
                }
            }
        }
    }
}
