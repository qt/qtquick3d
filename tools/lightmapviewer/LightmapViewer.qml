// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Dialogs
import QtQuick3D
import QtQuick3D.Helpers

import QtQuick3D.lightmapviewer
import LightmapFile 1.0

ApplicationWindow {
    width: 1200
    height: 800
    visible: true
    title: qsTr("Lightmap Viewer")

    id: window

    property var selectedKey: listView.model.length ? listView.model[0] : null
    property string meshKey: ""
    property var textureTagsAvailable: []
    property int selectedTextureTag: -1

    onSelectedKeyChanged: {
        refresh()
    }

    Connections {
        target: LightmapFile
        function onKeysChanged() {
            refresh()
        }
    }

    function refresh() {
        meshKey = ""
        if (selectedKey === "") {
            textureTagsAvailable = []
            return
        }

        var newTags = LightmapFile.texturesAvailableFor(selectedKey)
        if (textureTagsAvailable !== newTags) {
            textureTagsAvailable = newTags
        }

        if (selectedTextureTag === -1 && textureTagsAvailable.length > 0)
            selectedTextureTag = textureTagsAvailable[0].value

        meshKey = LightmapFile.meshKeyFor(selectedKey)
    }

    Dialog {
        id: sceneMetadataDialog
        modal: true
        standardButtons: Dialog.NoButton
        x: Math.round((window.width - width) / 2)
        y: Math.round((window.height - height) / 2)
        visible: false
        width: 220
        height: 360

        contentItem: SceneMetadataView {}
    }

    header: ToolBar {
        RowLayout {
            Button {
                text: qsTr("Open Lightmap...")
                onClicked: fileDialog.open()
            }

            Button {
                text: qsTr("Scene Metadata...")
                onClicked: sceneMetadataDialog.open()
            }

            Rectangle {
                width: 1
                color: "darkgray"
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignVCenter
            }


            Text {
                text: "Path: " + LightmapFile.source
            }
        }
    }

    FileDialog {
        id: fileDialog
        onAccepted: {
            LightmapFile.source = selectedFile
        }
    }

    Shortcut {
        sequences: [StandardKey.Open]
        onActivated: {
            fileDialog.open()
        }
    }

    function selectTextureByIndex(i) {
        const n = window.textureTagsAvailable?.length || 0
        if (i >= 0 && i < n) {
            const row = window.textureTagsAvailable[i]
            const v = row[comboLmTextureCandidate.valueRole]
            window.selectedTextureTag = Number(v)
            comboLmTextureCandidate.currentIndex = i
        }
    }

    Instantiator {
        model: Math.min(window.textureTagsAvailable?.length || 0, 9)
        delegate: Shortcut {
            sequence: (index + 1).toString()
            context: Qt.ApplicationShortcut
            enabled: !filterField.activeFocus
            onActivated: selectTextureByIndex(index)
        }
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        focus: true
        Keys.onPressed: event => {
                            if (event.key === Qt.Key_Up || event.key === Qt.Key_Down) {
                                var i = listView.currentIndex
                                const dir = (event.key === Qt.Key_Down) ? 1 : -1

                                do { i += dir } while (i >= 0 && i < listView.count && !listView.itemAtIndex(i)?.matches)

                                if (i >= 0 && i < listView.count) {
                                    listView.currentIndex = i
                                    selectedKey = listView.model[i]
                                }

                                event.accepted = true
                            }
                        }

        SplitView {
            id: leftSplit
            SplitView.preferredWidth: 250
            SplitView.minimumWidth: 120
            orientation: Qt.Vertical

            ColumnLayout {
                spacing: 8

                RowLayout {

                    Label {
                        text: "Texture:"
                    }
                    ComboBox {
                        id: comboLmTextureCandidate
                        model: window.textureTagsAvailable
                        textRole: "name"
                        valueRole: "value"

                        // Closed state text
                        displayText: {
                            const idx = currentIndex
                            if (idx < 0) return ""
                            const row = window.textureTagsAvailable[idx]
                            const base = row?.[comboLmTextureCandidate.textRole] ?? ""
                            const prefix = (idx < 9) ? `${idx + 1}. ` : ""
                            return prefix + base
                        }

                        // Popup items
                        delegate: ItemDelegate {
                            required property int index
                            required property var modelData
                            width: parent.width
                            text: {
                                const base = modelData[comboLmTextureCandidate.textRole] ?? ""
                                const prefix = (index < 9) ? `${index + 1}. ` : ""
                                return prefix + base
                            }
                            highlighted: comboLmTextureCandidate.highlightedIndex === index
                        }

                        function indexForValue(val) {
                            for (var i = 0; i < window.textureTagsAvailable.length; ++i)
                                if (window.textureTagsAvailable[i].value === val)
                                    return i
                            return -1
                        }

                        onActivated: window.selectedTextureTag = Number(currentValue)
                        currentIndex: indexForValue(window.selectedTextureTag)
                    }
                }

                Pane {
                    id: metaPane
                    Layout.fillWidth: true
                    clip: true

                    ScrollView {
                        ColumnLayout {
                            id: metadataColumn
                            spacing: 4

                            Repeater {
                                model: LightmapFile.metadataFor(selectedKey)
                                delegate: RowLayout {
                                    width: metadataColumn.width
                                    spacing: 8

                                    Label {
                                        text: (modelData.key ?? "—") + ":"
                                        font.bold: true
                                    }
                                    Label {
                                        text: modelData.value
                                              !== undefined ? String(
                                                                  modelData.value) : "—"
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Timer {
                id: debounce
                interval: 120; repeat: false; running: false
                onTriggered: listView.forceLayout()
            }

            TextField {
                id: filterField
                placeholderText: "Filter keys…"
                Layout.fillWidth: true
                onTextEdited: debounce.restart()
            }


            ListView {
                id: listView
                Layout.fillWidth: true
                clip: true
                spacing: 2
                highlightMoveVelocity: -1
                highlightMoveDuration: 1
                model: LightmapFile.keys

                delegate: Item {
                    readonly property bool matches: {
                        const q = filterField.text.trim().toLowerCase()
                        if (!q) return true
                        const tokens = q.split(/\s+/)
                        const hay = String(modelData).toLowerCase()
                        return tokens.every(t => hay.includes(t))
                    }

                    width: listView.width
                    height: matches ? Math.max(24, rowText.implicitHeight + 6) : 0
                    visible: matches
                    HoverHandler { id: hh }

                    Behavior on height {
                        NumberAnimation {
                            duration: 120
                            easing.type: Easing.OutCubic
                        }
                    }
                    Behavior on opacity {
                        NumberAnimation {
                            duration: 120
                            easing.type: Easing.OutCubic
                        }
                    }

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            id: rowText
                            text: modelData
                            elide: Text.ElideRight
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: 6
                        visible: hh.hovered
                        color: Qt.rgba(76/255, 134/255, 191/255, 0.10)
                        z: -1
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        enabled: true
                        onClicked: {
                            listView.currentIndex = index
                            selectedKey = modelData
                        }
                    }
                }

                highlight: Rectangle {
                    color: Qt.rgba(76 / 255, 134 / 255, 191 / 255, 0.25)
                    radius: 6
                    anchors.margins: 2
                }

                ScrollBar.vertical: ScrollBar {}
            }
        }

        SplitView {
            id: rightSplit
            orientation: Qt.Horizontal

            Loader {
                id: meshLoader
                sourceComponent: MeshViewer {}
                active: true
                enabled: visible
                SplitView.preferredWidth: Math.round(rightSplit.width * 0.50)
            }

            Loader {
                id: imageLoader
                sourceComponent: ImageViewer {}
                active: true
                enabled: visible
            }
        }
    }

    DropArea {
        id: dropArea
        anchors.fill: parent
        onEntered: drag => {
                       drag.accept(Qt.LinkAction)
                   }
        // Just take first url if several
        onDropped: drop => {
                       if (drop.hasUrls) {
                           LightmapFile.source = drop.urls[0]
                       }
                   }
    }
}
