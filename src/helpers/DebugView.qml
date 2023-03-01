// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D
import QtQuick3D.Helpers.impl

Pane {
    property var source: null
    property bool resourceDetailsVisible: false
    // yes it needs a fixed size when in detailed mode, the tables and stuff
    // will size themselves to this
    width: resourceDetailsVisible ? 600 : layout.contentWidth
    height: layout.contentHeight
    opacity: 0.9

    ColumnLayout {
        id: layout
        anchors.fill: parent
        RowLayout {
            Label {
                Layout.fillWidth: true
                text: source.renderStats.fps + " FPS"
                font.pointSize: 14
            }

            Label {
                text: "Details"
            }

            CheckBox {
                checked: resourceDetailsVisible
                onCheckedChanged: {
                    resourceDetailsVisible = checked;
                }
            }
        }

        component TimeLabel : RowLayout {
            id: timeLabel
            property alias text: label.text
            property real value: 0.0
            Label {
                id: label
                Layout.fillWidth: true
                text: "Frame: "

            }
            Label {
                text: timeLabel.value.toFixed(3) + "ms"
            }
        }

        TimeLabel {
            text: "Frame: "
            value: source.renderStats.frameTime
        }

        TimeLabel {
            text: "    Sync: "
            value: source.renderStats.syncTime
        }

        TimeLabel {
            text: "    Prep: "
            value: source.renderStats.renderPrepareTime
        }

        TimeLabel {
            text: "    Render: "
            value: source.renderStats.renderTime
        }

        TimeLabel {
            text: "Max: "
            value: source.renderStats.maxFrameTime
        }

        Page {
            Layout.fillWidth: true
            visible: resourceDetailsVisible
            header: TabBar {
                id: tabBar
                TabButton {
                    text: "Summary"
                }
                TabButton {
                    text: "Passes"
                }
                TabButton {
                    text: "Textures"
                }
                TabButton {
                    text: "Meshes"
                }
                TabButton {
                    text: "Tools"
                }
            }

            StackLayout {
                anchors.fill: parent
                anchors.margins: 10
                currentIndex: tabBar.currentIndex

                Pane {
                    id: summaryPane
                    ColumnLayout {
                        Label {
                            text: "Graphics API: " + source.renderStats.graphicsApiName
                            visible: resourceDetailsVisible
                        }
                        Label {
                            text: source.renderStats.renderPassCount + " render passes"
                            visible: resourceDetailsVisible
                        }
                        Label {
                            text: source.renderStats.drawCallCount + " draw calls"
                            visible: resourceDetailsVisible
                        }
                        Label {
                            text: source.renderStats.drawVertexCount + " vertices"
                            visible: resourceDetailsVisible
                        }
                        Label {
                            text: "Image assets: " + (source.renderStats.imageDataSize / 1024).toFixed(2) + " KB"
                            visible: resourceDetailsVisible
                        }
                        Label {
                            text: "Mesh assets: " + (source.renderStats.meshDataSize / 1024).toFixed(2) + " KB"
                            visible: resourceDetailsVisible
                        }
                        Label {
                            text: "Pipelines: " + source.renderStats.pipelineCount
                            visible: resourceDetailsVisible
                        }
                        Label {
                            text: "Material build time: " + source.renderStats.materialGenerationTime + " ms"
                            visible: resourceDetailsVisible
                        }
                        Label {
                            text: "Effect build time: " + source.renderStats.effectGenerationTime + " ms"
                            visible: resourceDetailsVisible
                        }
                        Label {
                            text: "Pipeline build time: " + source.renderStats.pipelineCreationTime + " ms"
                            visible: resourceDetailsVisible
                        }
                        Label {
                            text: source.renderStats.vmemAllocCount + " vmem allocs with " + source.renderStats.vmemUsedBytes + " bytes"
                            visible: resourceDetailsVisible && source.renderStats.vmemAllocCount > 0
                        }
                    }
                }

                Pane {
                    id: passesPane
                    RenderStatsPassesModel {
                        id: passesModel
                        passData: source.renderStats.renderPassDetails
                    }
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0
                        HorizontalHeaderView {
                            syncView: passesTableView
                            resizableColumns: false // otherwise QTBUG-111013 happens
                            boundsBehavior: Flickable.StopAtBounds
                            flickableDirection: Flickable.VerticalFlick
                        }
                        ListModel {
                            id: passesHeaderModel
                            ListElement {
                                columnWidth: 300 // name
                            }
                            ListElement {
                                columnWidth: 80 // size
                            }
                            ListElement {
                                columnWidth: 60 // vertices
                            }
                            ListElement {
                                columnWidth: 60 // draw calls
                            }
                        }
                        Item {
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            TableView {
                                id: passesTableView
                                anchors.fill: parent
                                // name, size, vertices, draw calls
                                property var columnFactors: [58, 14, 12, 12]; // == 96, leave space for the scrollbar
                                columnWidthProvider: function (column) {
                                    return passesPane.width * (columnFactors[column] / 100.0);
                                }
                                boundsBehavior: Flickable.StopAtBounds
                                flickableDirection: Flickable.VerticalFlick
                                ScrollBar.vertical: ScrollBar {
                                    parent: passesTableView.parent
                                    anchors.top: passesTableView.top
                                    anchors.bottom: passesTableView.bottom
                                    anchors.left: passesTableView.right
                                }
                                clip: true
                                model: passesModel
                                columnSpacing: 1
                                rowSpacing: 1
                                implicitWidth: parent.width + columnSpacing
                                implicitHeight: parent.height + rowSpacing
                                delegate: CustomTableItemDelegate {
                                    text: display
                                    color: passesTableView.palette.base
                                    textColor: passesTableView.palette.text
                                }
                            }
                        }
                    }
                }

                Pane {
                    id: texturesPane
                    RenderStatsTexturesModel {
                        id: texturesModel
                        textureData: source.renderStats.textureDetails
                    }
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0
                        HorizontalHeaderView {
                            syncView: texturesTableView
                            resizableColumns: false // otherwise QTBUG-111013 happens
                            boundsBehavior: Flickable.StopAtBounds
                            flickableDirection: Flickable.VerticalFlick
                        }
                        Item {
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            TableView {
                                id: texturesTableView
                                anchors.fill: parent
                                // name, size, format, miplevels, flags
                                property var columnFactors: [48, 12, 12, 12, 12]; // == 96, leave space for the scrollbar
                                columnWidthProvider: function (column) {
                                    return texturesPane.width * (columnFactors[column] / 100.0);
                                }
                                boundsBehavior: Flickable.StopAtBounds
                                flickableDirection: Flickable.VerticalFlick
                                ScrollBar.vertical: ScrollBar {
                                    parent: texturesTableView.parent
                                    anchors.top: texturesTableView.top
                                    anchors.bottom: texturesTableView.bottom
                                    anchors.left: texturesTableView.right
                                }
                                ScrollBar.horizontal: ScrollBar { }
                                clip: true
                                model: texturesModel
                                columnSpacing: 1
                                rowSpacing: 1
                                implicitWidth: parent.width + columnSpacing
                                implicitHeight: parent.height + rowSpacing
                                delegate: CustomTableItemDelegate {
                                    text: display
                                    color: texturesTableView.palette.base
                                    textColor: texturesTableView.palette.text
                                }
                            }
                        }
                    }
                }

                Pane {
                    id: meshesPane
                    RenderStatsMeshesModel {
                        id: meshesModel
                        meshData: source.renderStats.meshDetails
                    }
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0
                        HorizontalHeaderView {
                            syncView: meshesTableView
                            resizableColumns: false // otherwise QTBUG-111013 happens
                            boundsBehavior: Flickable.StopAtBounds
                            flickableDirection: Flickable.VerticalFlick
                        }
                        Item {
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            TableView {
                                id: meshesTableView
                                anchors.fill: parent
                                // name, submeshes, vertices, vbufsize, ibufsize
                                property var columnFactors: [48, 12, 12, 12, 12]; // == 96, leave space for the scrollbar
                                columnWidthProvider: function (column) {
                                    return meshesPane.width * (columnFactors[column] / 100.0);
                                }
                                boundsBehavior: Flickable.StopAtBounds
                                flickableDirection: Flickable.VerticalFlick
                                ScrollBar.vertical: ScrollBar {
                                    parent: meshesTableView.parent
                                    anchors.top: meshesTableView.top
                                    anchors.bottom: meshesTableView.bottom
                                    anchors.left: meshesTableView.right
                                }
                                clip: true
                                model: meshesModel
                                columnSpacing: 1
                                rowSpacing: 1
                                implicitWidth: parent.width + columnSpacing
                                implicitHeight: parent.height + rowSpacing
                                delegate: CustomTableItemDelegate {
                                    text: display
                                    color: meshesTableView.palette.base
                                    textColor: meshesTableView.palette.text
                                }
                            }
                        }
                    }
                }

                Pane {
                    id: visualizePane
                    ColumnLayout {
                        id: visCtrCol
                        width: parent.width
                        CheckBox {
                            text: "Wireframe mode"
                            onCheckedChanged: source.environment.debugSettings.wireframeEnabled = checked
                        }
                        RowLayout {
                            Label {
                                text: "Material override"
                            }
                            ComboBox {
                                id: materialOverrideComboBox
                                textRole: "text"
                                valueRole: "value"
                                implicitContentWidthPolicy: ComboBox.WidestText
                                onActivated: source.environment.debugSettings.materialOverride = currentValue
                                Component.onCompleted: materialOverrideComboBox.currentIndex = materialOverrideComboBox.indexOfValue(source.environment.debugSettings.materialOverride)
                                model: [
                                    { value: DebugSettings.None, text: "None"},
                                    { value: DebugSettings.BaseColor, text: "Base Color"},
                                    { value: DebugSettings.Roughness, text: "Roughness"},
                                    { value: DebugSettings.Metalness, text: "Metalness"},
                                    { value: DebugSettings.Diffuse, text: "Diffuse"},
                                    { value: DebugSettings.Specular, text: "Specular"},
                                    { value: DebugSettings.ShadowOcclusion, text: "Shadow Occlusion"},
                                    { value: DebugSettings.Emission, text: "Emission"},
                                    { value: DebugSettings.AmbientOcclusion, text: "Ambient Occlusion"},
                                    { value: DebugSettings.Normals, text: "Normals"},
                                    { value: DebugSettings.Tangents, text: "Tangents"},
                                    { value: DebugSettings.Binormals, text: "Binormals"},
                                    { value: DebugSettings.F0, text: "F0"}
                                ]
                            }
                        }
                        Button {
                            text: "Release cached resources"
                            onClicked: source.renderStats.releaseCachedResources()
                        }
                        Button {
                            text: "Bake lightmap"
                            onClicked: source.bakeLightmap()
                        }
                    }
                }
            }
        }
    }

    component CustomTableItemDelegate : Rectangle {
        property alias text: textLabel.text
        property alias textColor: textLabel.color
        implicitWidth: 100
        implicitHeight: textLabel.implicitHeight + 4
        color: palette.base
        Label {
            id: textLabel
            anchors.centerIn: parent
            color: palette.text
        }
    }

    function syncVisible() {
        if (source) {
            source.renderStats.extendedDataCollectionEnabled = visible && resourceDetailsVisible;
            if (source.renderStats.extendedDataCollectionEnabled)
                source.update();
        }
    }

    Component.onCompleted: syncVisible()
    onSourceChanged: syncVisible()
    onVisibleChanged: syncVisible()
    onResourceDetailsVisibleChanged: syncVisible()
}
