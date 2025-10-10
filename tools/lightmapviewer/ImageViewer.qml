// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LightmapFile 1.0

ColumnLayout {
    id: root
    anchors.fill: parent

    property real imageZoom: 1
    property real imageCenterX: 0
    property real imageCenterY: 0

    Pane {
        Layout.alignment: Qt.AlignLeft
        implicitHeight: 30
        padding: 8

        RowLayout {
            spacing: 20

            Label {
                text: "Zoom: " + root.imageZoom.toFixed(1)
            }

            Switch {
                id: alphaSwitch
                padding: 0
                checked: true
                text: "Alpha"
            }
        }
    }

    Rectangle {
        id: scrollView
        clip: true
        color: "black"
        Layout.fillWidth: true
        Layout.fillHeight: true

        property real lastMouseX: 0
        property real lastMouseY: 0

        function clamp() {
            // If the image is smaller than the scroll view, center it
            if (image.width <= scrollView.width) {
                root.imageCenterX = 0
            } else {
                const maxOffsetX = (image.width - scrollView.width) / 2
                root.imageCenterX = Math.max(-maxOffsetX,
                                             Math.min(root.imageCenterX,
                                                      maxOffsetX))
            }

            if (image.height <= scrollView.height) {
                root.imageCenterY = 0
            } else {
                const maxOffsetY = (image.height - scrollView.height) / 2
                root.imageCenterY = Math.max(-maxOffsetY,
                                             Math.min(root.imageCenterY,
                                                      maxOffsetY))
            }
        }

        onWidthChanged: scrollView.clamp()
        onHeightChanged: scrollView.clamp()

        Connections {
            target: window
            function onSelectedKeyChanged() {
                if (imageLoader.item === scrollView) {
                    root.imageZoom = 1
                    root.imageCenterX = 0
                    root.imageCenterY = 0
                }
            }
        }

        MouseArea {
            id: mouseArea
            property bool dragging: false
            anchors.fill: parent
            onPressed: mouse => {
                           scrollView.lastMouseX = mouse.x
                           scrollView.lastMouseY = mouse.y
                           dragging = true
                       }
            onReleased: mouse => {
                            dragging = false
                        }

            onPositionChanged: mouse => {
                                   var dx = mouse.x - scrollView.lastMouseX
                                   var dy = mouse.y - scrollView.lastMouseY

                                   scrollView.lastMouseX = mouse.x
                                   scrollView.lastMouseY = mouse.y

                                   root.imageCenterX += dx
                                   root.imageCenterY += dy

                                   scrollView.clamp()
                               }
            cursorShape: mouseArea.dragging ? Qt.ClosedHandCursor : Qt.ArrowCursor

            onWheel: event => {
                         const oldZoom = imageZoom
                         const zoomDelta = event.angleDelta.y / 256
                         const newZoom = Math.max(1,
                                                  Math.min(32,
                                                           oldZoom + zoomDelta))

                         if (newZoom === oldZoom)
                         return

                         // Adjust center offset so the same point remains at the center
                         const scaleFactor = newZoom / oldZoom
                         root.imageCenterX *= scaleFactor
                         root.imageCenterY *= scaleFactor

                         root.imageZoom = newZoom
                         scrollView.clamp()

                         event.accepted = true
                     }
        }

        Image {
            id: baseGrid
            anchors.fill: scrollView
            source: "grid.png"
            fillMode: Image.Tile
            opacity: 0.75
        }

        Rectangle {
            width: image.width + (border.width * 2)
            height: image.height + (border.width * 2)
            x: image.x - border.width
            y: image.y - border.width
            color: "white"

            border.width: 0
            border.color: "white"
            opacity: 0.25
        }

        Image {
            id: image
            x: Math.round(parent.width / 2 - width / 2) + root.imageCenterX
            y: Math.round(parent.height / 2 - height / 2) + root.imageCenterY
            source: LightmapFile.imageUrlFor(selectedKey, selectedTextureTag,
                                             alphaSwitch.checked)
            onWidthChanged: scrollView.clamp()
            onHeightChanged: scrollView.clamp()
            fillMode: Image.PreserveAspectFit
            smooth: false
            antialiasing: false

            // Let the image scale visibly
            width: sourceSize.width * root.imageZoom
            height: sourceSize.height * root.imageZoom
        }
    }
}
