// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D

Rectangle {
    id: dbgView
    property var source: null
    property bool resourceDetailsVisible: false
    property int maxResourceDetailsHeight: 480
    width: layout.width + 10
    height: layout.height + 10
    color: "#80778BA5"
    radius: 5

    Column {
        id: layout
        anchors.centerIn: parent

        Text {
            text: source.renderStats.fps + " FPS (" + (source.renderStats.frameTime).toFixed(3) + "ms)"
            font.pointSize: 13
            color: "white"
        }
        Text {
            text: "Sync: " + (source.renderStats.syncTime).toFixed(3) + "ms"
            font.pointSize: 9
            color: "white"
        }
        Text {
            id: renderTimeInfo
            text: "Render: " + (source.renderStats.renderTime).toFixed(3) + "ms (prep: " + (source.renderStats.renderPrepareTime).toFixed(3) + "ms)"
            font.pointSize: 9
            color: "white"
        }
        Text {
            text: "Max: " + (source.renderStats.maxFrameTime).toFixed(3) + "ms"
            font.pointSize: 9
            color: "white"
        }
        Text {
            text: source.renderStats.drawCallCount + " draw calls with " + source.renderStats.drawVertexCount + " vertices in " + source.renderStats.renderPassCount + " render passes"
            font.pointSize: 9
            color: "white"
            visible: resourceDetailsVisible
        }
        Text {
            text: "Window assets: " + (source.renderStats.imageDataSize / 1024).toFixed(2) + " KB tex., " + (source.renderStats.meshDataSize / 1024).toFixed(2) + " KB mesh, " + source.renderStats.pipelineCount + " pipl."
            font.pointSize: 9
            color: "white"
            visible: resourceDetailsVisible
        }
        MenuSeparator {
            padding: 0
            topPadding: 8
            bottomPadding: 8
            width: Math.max(renderTimeInfo.implicitWidth, resDetailsText.implicitWidth)
            visible: resourceDetailsVisible
        }
        TabBar {
            id: bar
            opacity: 0.75
            width: Math.max(renderTimeInfo.implicitWidth, resDetailsText.implicitWidth)
            visible: resourceDetailsVisible
            TabButton {
                text: "Render passes"
                font.pointSize: 9
            }
            TabButton {
                text: "Textures"
                font.pointSize: 9
            }
            TabButton {
                text: "Meshes"
                font.pointSize: 9
            }
            TabButton {
                text: "Debug"
                font.pointSize: 9
            }
        }
        ScrollView {
            id: resDetails
            visible: resourceDetailsVisible && bar.currentIndex < 3
            width: resDetailsText.implicitWidth + 20
            height: Math.min(maxResourceDetailsHeight, resDetailsText.implicitHeight)
            // the default scrollbar behavior is bad for dynamic content, do something sensible instead
            ScrollBar.vertical.policy: height >= maxResourceDetailsHeight ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
            Text {
                id: resDetailsText
                text: bar.currentIndex === 0 ? source.renderStats.renderPassDetails
                                             : (bar.currentIndex === 1 ? source.renderStats.textureDetails : source.renderStats.meshDetails)
                font.pointSize: 9
                color: "white"
                textFormat: Text.PlainText //Text.MarkdownText // markdown is too slow to display tables so default to plain text
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    onClicked: contextMenu.popup()
                }
                Menu {
                    id: contextMenu
                    MenuItem {
                        text: "Print to console"
                        onTriggered: console.log(resDetailsText.text)
                    }
                }
            }
        }
        Item {
            visible: resourceDetailsVisible && bar.currentIndex === 3
            width: bar.width
            height: visCtrCol.height
            Column {
                id: visCtrCol
                width: parent.width
                ColumnLayout {
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
                        onClicked: source.renderStats.releaseCachedResources(dbgView)
                    }
                }
            }
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
