/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick
import QtQuick3D
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Window

Rectangle {
    height: 256
    width: 256

    color: "white"
    border.color: "black"
    border.width: 5

    MouseArea {
        objectName: "background rgb dot controller"
        anchors.fill: parent
        cursorShape: Qt.CrossCursor
        onPressed: function(mouse) {
            debugLabel.text = "(" + Math.floor(mouse.x) + ", " + Math.floor(mouse.y) + ")"
            debugPoint.visible = true
            debugPoint.x = mouse.x - 10
            debugPoint.y = mouse.y - 10
            debugPoint.color = "red"
        }
        onReleased: function(mouse) {
            debugLabel.text = "(" + Math.floor(mouse.x) + ", " + Math.floor(mouse.y) + ")"
            debugPoint.visible = true
            debugPoint.x = mouse.x - 10
            debugPoint.y = mouse.y - 10
            debugPoint.color = "green"
        }
        onPositionChanged: function(mouse) {
            debugLabel.text = "(" + Math.floor(mouse.x) + ", " + Math.floor(mouse.y) + ")"
            debugPoint.visible = true
            debugPoint.x = mouse.x - 10
            debugPoint.y = mouse.y - 10
            debugPoint.color = "blue"
        }
    }

    Rectangle {
        id: debugPoint
        height: 20
        width: 20
        radius: 10
        color: "red"
        visible: false
    }

    ColumnLayout {
        anchors.centerIn: parent
        Label {
            id: debugLabel
            text: "no input"
        }
        Button {
            text: "Click Me"
            onClicked: {
                debugLabel.text("clicked 'Me'")
            }
        }
        Rectangle {
            width: 100; height: 40
            color: btap.pressed ? "tomato" : bhover.hovered ? "wheat" : "beige"
            HoverHandler {
                id: bhover
                cursorShape: Qt.OpenHandCursor
            }
            TapHandler {
                id: btap
                objectName: "me too"
                onTapped: btxt.text = "clicked @ " + point.position.x.toFixed(1) + ", " + point.position.y.toFixed(1)
            }
            Text {
                id: btxt
                anchors.centerIn: parent
                text: "click me too"
            }
        }
        TextInput {
            id: textInput
            text: "Some Text"
            focus: true
        }
        Text {
            text: "text cursor pos " + textInput.cursorPosition
        }
        Text {
            Layout.maximumWidth: 220
            elide: Text.ElideRight
            text: "focus " + Window.activeFocusItem
        }
    }
}
