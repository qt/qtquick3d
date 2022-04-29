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
    caption: qsTr("Repeater")
    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: qsTr("Model")
            tooltip: qsTr("The model providing data for the repeater. This can simply specify the number of delegate instances to create or it can be bound to an actual model.")
        }

        SecondColumnLayout {
            LineEdit {
                backendValue: backendValues.model
                showTranslateCheckBox: false
                writeAsExpression: true
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Delegate")
            tooltip: qsTr("The delegate provides a template defining each object instantiated by the repeater.")
        }

        SecondColumnLayout {
            IdComboBox {
                typeFilter: "Component"
                backendValue: backendValues.delegate
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
