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

Section {
    caption: qsTr("Instance List")
    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: qsTr("Instances")
            tooltip: qsTr("This property contains the list of instance definitions. Modifying this list, or any of its elements, will cause the instance table to be updated.")
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 5
        }

        SecondColumnLayout {
            EditableListView {
                backendValue: backendValues.instances
                model: backendValues.instances.expressionAsList
                Layout.fillWidth: true
                typeFilter: "QtQuick3D.InstanceListEntry"

                onAdd: function(value) { backendValues.instances.idListAdd(value) }
                onRemove: function(idx) { backendValues.instances.idListRemove(idx) }
                onReplace: function (idx, value) { backendValues.instances.idListReplace(idx, value) }
            }

            ExpandingSpacer {}
        }
    }
}
