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
        caption: qsTr("Pass")
        width: parent.width

        SectionLayout {
            Label {
                text: qsTr("Commands")
                tooltip: qsTr("Render commands of the pass.")
            }
            SecondColumnLayout {
                EditableListView {
                    backendValue: backendValues.commands
                    model: backendValues.commands.expressionAsList
                    Layout.fillWidth: true
                    typeFilter: "QtQuick3D.Command"

                    onAdd: function(value) { backendValues.commands.idListAdd(value) }
                    onRemove: function(idx) { backendValues.commands.idListRemove(idx) }
                    onReplace: function (idx, value) { backendValues.commands.idListReplace(idx, value) }
                }
            }
            Label {
                text: qsTr("Buffer")
                tooltip: qsTr("Output buffer for the pass.")
            }
            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QtQuick3D.Buffer"
                    Layout.fillWidth: true
                    backendValue: backendValues.output
                }
            }
            Label {
                text: qsTr("Shaders")
                tooltip: qsTr("Shaders for the pass.")
            }
            SecondColumnLayout {
                EditableListView {
                    backendValue: backendValues.shaders
                    model: backendValues.shaders.expressionAsList
                    Layout.fillWidth: true
                    typeFilter: "QtQuick3D.Shader"

                    onAdd: function(value) { backendValues.shaders.idListAdd(value) }
                    onRemove: function(idx) { backendValues.shaders.idListRemove(idx) }
                    onReplace: function (idx, value) { backendValues.shaders.idListReplace(idx, value) }
                }
            }
        }
    }
}
