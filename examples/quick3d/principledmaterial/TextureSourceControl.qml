/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D
import QtQuick.Dialogs

import Example

RowLayout {
    id: root
    property Texture targetTexture: null
    property bool stampMode: false
    required property url defaultTexture
    property url stampSource: ""
    property alias defaultClearColor: drawer.clearColor
    property bool envMapMode: false

    GroupBox {
        title: "Image Source Mode"
        ColumnLayout {
            RadioButton {
                id: noTextureChoice
                text: "None"
                checked: true
                onCheckedChanged: {
                    targetTexture = null
                }
            }
            RadioButton {
                id: selectTextureChoice
                text: "Texture"
                checked: false
                onCheckedChanged: targetTexture = selectedTexture
            }
            RadioButton {
                id: loadImageChoice
                text: "Load Image"
                onCheckedChanged: targetTexture = loadTextureTexture
            }
            RadioButton {
                id: drawerChoice
                text: "Draw Texture"
                onCheckedChanged: targetTexture = drawerTexture
            }
        }
    }
    Item {
        visible: selectTextureChoice.checked
        width: 256
        height: 256
        Image {
            id: previewImage
            anchors.fill: parent
            sourceSize.width: width
            sourceSize.height: height
            fillMode: Image.PreserveAspectFit
            source: root.defaultTexture
            Texture {
                id: selectedTexture
                source: root.defaultTexture
                mappingMode: envMapMode ? Texture.Environment : Texture.UV
            }
        }
    }


    Rectangle {
        id: loadTextureFrame
        width: 256
        height: 256
        color: "transparent"
        border.color: "black"
        visible: loadImageChoice.checked
        property url textureSource: ""
        Text {
            anchors.centerIn: parent
            text: "[Load Image]"
        }
        Image {
            anchors.fill: parent
            sourceSize.width: width
            sourceSize.height: height
            fillMode: Image.PreserveAspectFit
            visible: loadTextureFrame.textureSource !== null
            source: loadTextureFrame.textureSource
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                textureSourceDialog.open()
            }
        }

        Texture {
            id: loadTextureTexture
            source: loadTextureFrame.textureSource
            mappingMode: envMapMode ? Texture.Environment : Texture.UV
        }

        ImageHelper {
            id: imageHelper
        }

        FileDialog {
            id: textureSourceDialog
            title: "Open an Image File"
            nameFilters: [ imageHelper.getSupportedImageFormatsFilter()]
            onAccepted: {
                if (textureSourceDialog.selectedFile !== null) {
                    loadTextureFrame.textureSource = textureSourceDialog.selectedFile
                }
            }
        }

    }
    ColumnLayout {
        visible: drawerChoice.checked
        Rectangle {
            width: 260
            height: 260
            color: "transparent"
            border.color: "black"
            Canvas {
                id: drawer
                width: 256
                height: 256
                x: 2
                y: 2

                property color penColor: "blue"
                property real penWidth: penWidthSlider.value
                property bool needsClear: true
                property color clearColor: "white"
                property var commands: []
                property var stampCommands: []
                property bool stampMode: root.stampMode
                //property point prevPoint : Qt.point(0, 0)

                onPaint: {
                    let ctx = getContext('2d');
                    if (needsClear) {
                        ctx.fillStyle = Qt.rgba(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
                        ctx.fillRect(0, 0, width, height)
                        needsClear = false;
                    }
                    if (!stampMode) {
                        ctx.strokeStyle = Qt.rgba(penColor.r, penColor.g, penColor.b, penColor.a)
                        ctx.lineCap = "round"
                        ctx.lineWidth = penWidth;

                        for (let i = 0; i < commands.length; ++i) {
                            let command = commands[i];
                            ctx.beginPath()
                            ctx.moveTo(command.start.x, command.start.y);
                            ctx.lineTo(command.end.x, command.end.y);
                            ctx.stroke();
                        }
                        commands = [];
                    } else {
                        for (let i = 0; i < stampCommands.length; ++i) {
                            let stampCommand = stampCommands[i]
                            // get offset
                            let dX = stampCommand.x - stampcursor.width * 0.5
                            let dY = stampCommand.y - stampcursor.height * 0.5
                            ctx.drawImage(stampcursor, dX, dY)
                        }
                        stampCommands = [];
                    }
                }
            }

            MouseArea {
                id: mouseArea
                anchors.fill: drawer
                enabled: drawerChoice.checked
                hoverEnabled: true
                //acceptedButtons: Qt.LeftButton
                property bool isDrawing: false
                property var lastPosition: Qt.point(0, 0)
                preventStealing: true
                clip: true

                Item {
                    id: cursor
                    Rectangle {
                        anchors.centerIn: parent
                        visible: !root.stampMode
                        width: drawer.penWidth
                        height: drawer.penWidth
                        radius: width * 0.5
                        color: drawer.penColor
                    }

                    Image {
                       id: stampcursor
                       anchors.centerIn: parent
                       visible: root.stampMode
                       source: root.stampSource
                    }
                }

                onEntered: cursor.visible = true
                onExited: cursor.visible = false

                onPressed: (mouse)=> {
                               if (mouse.button === Qt.LeftButton && !root.stampMode) {
                                   lastPosition = Qt.point(mouse.x, mouse.y)
                                   isDrawing = true
                               }
                           }
                onPositionChanged: (mouse)=> {
                                       if (isDrawing) {
                                           let pos = Qt.point(mouse.x, mouse.y);
                                           let command = {"start": lastPosition, "end": pos}
                                           drawer.commands.push(command)
                                           lastPosition = pos;
                                           drawer.requestPaint();
                                       }
                                       cursor.x = mouse.x
                                       cursor.y = mouse.y
                                   }
                onReleased: (mouse)=> {
                                if (mouse.button === Qt.LeftButton && isDrawing) {
                                    let pos = Qt.point(mouse.x, mouse.y);
                                    let command = {"start": lastPosition, "end": pos}
                                    drawer.commands.push(command)
                                    isDrawing = false;
                                    drawer.requestPaint();
                                } else if (stampMode) {
                                    drawer.stampCommands.push(Qt.point(mouse.x, mouse.y));
                                    drawer.requestPaint();
                                }
                            }
            }

        }
        RowLayout {
            visible: !root.stampMode
            spacing: 0
            Rectangle {
                id: whiteBrush
                width: 25
                height: 25
                color: "white"
                border.color: "black"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        drawer.penColor = parent.color;
                    }
                }
            }
            Rectangle {
                id: blackBrush
                width: 25
                height: 25
                color: "black"
                border.color: "black"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        drawer.penColor = parent.color;
                    }
                }
            }
            Rectangle {
                id: redBrush
                width: 25
                height: 25
                color: "red"
                border.color: "black"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        drawer.penColor = parent.color;
                    }
                }
            }
            Rectangle {
                id: greenBrush
                width: 25
                height: 25
                color: "green"
                border.color: "black"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        drawer.penColor = parent.color;
                    }
                }
            }
            Rectangle {
                id: blueBrush
                width: 25
                height: 25
                color: "blue"
                border.color: "black"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        drawer.penColor = parent.color;
                    }
                }
            }
            Label {
                text: " "

            }

            Button {
                text: "Clear"
                onClicked: {

                    drawer.needsClear = true
                    drawer.requestPaint()
                }
            }

        }
        RowLayout {
            visible: !root.stampMode
            Slider {
                id: penWidthSlider
                from: 1
                to: 50
                value: 5
            }
            Label {
                Layout.fillWidth: true
                text: "Pen Width"
            }
        }

    }

    Texture {
        id: drawerTexture
        sourceItem: drawerChoice.checked ? drawer : null
        mappingMode: envMapMode ? Texture.Environment : Texture.UV
    }

}

