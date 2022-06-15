// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: comboBox
    property string text
    property string selection
    property alias values: combo.model
    property real parentWidth

    width: rowLayout.width
    height: rowLayout.height

    RowLayout {
        id: rowLayout
        ComboBox {
            id: combo
            displayText: text + ": " + model[currentIndex]
            textRole: text
            anchors.rightMargin: 0
            implicitWidth: Math.max(parentWidth - 5, 100)

            delegate: ItemDelegate {
                id: lightDelegate
                text: modelData
                anchors.left: parent.left
                anchors.right: parent.right
            }
            onCurrentIndexChanged: {
                comboBox.selection = model[currentIndex]
            }
        }
    }
}
