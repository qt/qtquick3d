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
        caption: qsTr("Mask")
        width: parent.width

        SectionLayout {
            Label {
                text: qsTr("Mask Texture")
                tooltip: qsTr("Defines a texture for mask.")
            }
            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QtQuick3D.Texture"
                    Layout.fillWidth: true
                    backendValue: backendValues.maskTexture_texture
                    defaultItem: qsTr("Default")
                }
            }
        }
    }

    Section {
        caption: qsTr("Aberration")
        width: parent.width

        SectionLayout {
            Label {
                text: qsTr("Amount")
                tooltip: qsTr("Amount of aberration.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 1000
                    minimumValue: -1000
                    decimals: 0
                    backendValue: backendValues.aberrationAmount
                    Layout.fillWidth: true
                }
            }
            Label {
                text: qsTr("Focus Depth")
                tooltip: qsTr("Focus depth of the aberration.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 10000
                    minimumValue: 0
                    realDragRange: 5000
                    decimals: 0
                    backendValue: backendValues.focusDepth
                    Layout.fillWidth: true
                }
            }
        }
    }
}
