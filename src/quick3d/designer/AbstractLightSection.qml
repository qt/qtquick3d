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

Column {
    width: parent.width

    Section {
        caption: qsTr("Light")
        width: parent.width

        SectionLayout {
            PropertyLabel {
                text: qsTr("Color")
                tooltip: qsTr("This property defines the color applied to models illuminated by this light.")
            }

            ColorEditor {
                backendValue: backendValues.color
                supportGradient: false
            }

            PropertyLabel {
                text: qsTr("Ambient Color")
                tooltip: qsTr("The property defines the ambient color applied to materials before being lit by this light.")
            }

            ColorEditor {
                backendValue: backendValues.ambientColor
                supportGradient: false
            }

            PropertyLabel {
                text: qsTr("Brightness")
                tooltip: qsTr("This property defines an overall multiplier for this light’s effects.")
            }

            SecondColumnLayout {
                SpinBox {
                    minimumValue: 0
                    maximumValue: 9999999
                    decimals: 2
                    stepSize: 0.01
                    backendValue: backendValues.brightness
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Scope")
                tooltip: qsTr("The property allows the selection of a Node in the scene. Only that node and it's children are affected by this light.")
            }

            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QtQuick3D.Node"
                    backendValue: backendValues.scope
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }

    NodeSection {
        width: parent.width
    }

    ShadowSection {
        width: parent.width
    }
}
