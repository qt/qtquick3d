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
        caption: qsTr("Noise")
        width: parent.width

        SectionLayout {
            Label {
                text: qsTr("Noise Sample Texture")
                tooltip: qsTr("Defines a texture for noise samples.")
            }
            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QtQuick3D.Texture"
                    Layout.fillWidth: true
                    backendValue: backendValues.noiseSample_texture
                    defaultItem: qsTr("Default")
                }
            }
        }
    }

    Section {
        caption: qsTr("Scatter")
        width: parent.width

        SectionLayout {
            Label {
                text: qsTr("Amount")
                tooltip: qsTr("Amount of scatter.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 127
                    minimumValue: 0
                    decimals: 2
                    backendValue: backendValues.amount
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Direction")
                tooltip: qsTr("Direction of scatter. 0 = both, 1 = horizontal, 2 = vertical.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 2
                    minimumValue: 0
                    decimals: 0
                    backendValue: backendValues.direction
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Randomize")
                tooltip: qsTr("Specifies if the scatter is random.")
            }
            SecondColumnLayout {
                CheckBox {
                    text: backendValues.randomize.valueToString
                    backendValue: backendValues.randomize
                    Layout.fillWidth: true
                }
            }
        }
    }
}
