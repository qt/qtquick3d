/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MaterialEditorHelpers 1.0

Flickable {
    id: flickable
    property alias font: textArea.font
    property alias text: textArea.text
    property alias textDocument: textArea.textDocument
    property alias lineColumn: lineNumbers
    property alias textArea: textArea

    property int rowHeight: Math.ceil(fontMetrics.lineSpacing)
    property int marginsTop: 10
    property int marginsLeft: 4
    property int lineCountWidth: 40
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    ScrollBar.vertical: ScrollBar {
        width: 15
        active: true
    }
    ScrollBar.horizontal: ScrollBar {
        width: 15
        active: true
    }

    FontMetrics {
        id: fontMetrics
        font: textArea.font
    }


    Column {
        id: lineNumbers
        anchors.left: parent.left
        anchors.leftMargin: marginsLeft
        anchors.topMargin:  marginsTop
        y: marginsTop
        width: lineCountWidth

        function labelAt(lineNr) {
            if (lineNr > 0) {
                if (lineNr > repeater.count)
                    lineNr = repeater.count; // Best guess at this point...
                return repeater.itemAt(lineNr - 1);
            }

            return null;
        }

        function range(start, end) {
            var rangeArray = new Array(end-start);
            for (var i = 0; i < rangeArray.length; i++)
                rangeArray[i] = start+i;
            return rangeArray;
        }

        Repeater {
            id: repeater
            model: textArea.lineCount
            delegate: Label {
                font: textArea.font
                width: parent.width
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
                height: rowHeight
                renderType: Text.NativeRendering
                text: index+1
            }
        }
    }
    Rectangle {
        id: lineNumbersSeperator
        y: 4
        height: parent.height
        anchors.left: lineNumbers.right
        anchors.leftMargin: marginsLeft
        width: 1
        color: "#ddd"
    }

    SyntaxHighlighter {
        id: syntaxHighlighter
        document: textArea.textDocument
    }

    TextArea.flickable: TextArea {
        id: textArea
        textFormat: Qt.PlainText
        focus: false
        selectByMouse: true
        leftPadding: flickable.marginsLeft
        rightPadding: flickable.marginsLeft
        topPadding: flickable.marginsTop
        bottomPadding: flickable.marginsTop
        tabStopDistance: fontMetrics.averageCharacterWidth * 4;
        anchors.left: lineNumbersSeperator.right
    }
}
