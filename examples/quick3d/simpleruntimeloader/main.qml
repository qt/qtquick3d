// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D
import QtQuick3D.AssetUtils

Window {
    id: root
    width: 900
    height: 600
    visible: true
    title: qsTr("Simple RuntimeLoader")

    property var sceneMaterials: []
    property var sceneCameras: []
    property var sceneLights: []
    property var selectedMaterial: null

    //! [query-function]
    function applyColorToSelected(color) {
        if (!root.selectedMaterial)
            return
        if (root.selectedMaterial instanceof PrincipledMaterial)
            (root.selectedMaterial as PrincipledMaterial).baseColor = color
        else if (root.selectedMaterial instanceof SpecularGlossyMaterial)
            (root.selectedMaterial as SpecularGlossyMaterial).albedoColor = color
    }
    //! [query-function]

    RowLayout {
        anchors.fill: parent
        spacing: 0

        View3D {
            id: view3d
            Layout.fillWidth: true
            Layout.fillHeight: true

            environment: SceneEnvironment {
                backgroundMode: SceneEnvironment.Color
                clearColor: "#1a1a2e"
            }


            //! [runtime-loader]
            RuntimeLoader {
                id: loader
                source: "torus_and_cone.glb"

                onStatusChanged: {
                    if (status !== RuntimeLoader.Success)
                        return
                    //! [queryAll]
                    root.sceneMaterials = loader.queryAll(RuntimeLoader.Materials)
                    root.sceneCameras = loader.queryAll(RuntimeLoader.Cameras)
                    root.sceneLights = loader.queryAll(RuntimeLoader.Lights)
                    //! [queryAll]
                    if (root.sceneCameras.length > 0)
                        view3d.camera = root.sceneCameras[0]
                    if (root.sceneMaterials.length > 0)
                        root.selectedMaterial = root.sceneMaterials[0]
                }
            }
            //! [runtime-loader]

            Label {
                anchors.centerIn: parent
                visible: loader.status === RuntimeLoader.Error
                text: "Error: " + loader.errorString
                color: "white"
            }
        }

        // Side panel
        Rectangle {
            Layout.preferredWidth: 200
            Layout.fillHeight: true
            color: "#22223b"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 12

                // Materials section
                Label {
                    text: "Materials"
                    color: "#aaaacc"
                    font.pixelSize: 11
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 1.2
                }

                ListView {
                    id: materialList
                    Layout.fillWidth: true
                    Layout.preferredHeight: contentHeight
                    clip: true
                    model: root.sceneMaterials

                    delegate: ItemDelegate {
                        id: materialDelegate
                        width: parent.width
                        text: modelData.objectName || "(unnamed)"
                        highlighted: modelData === root.selectedMaterial
                        onClicked: root.selectedMaterial = modelData

                        contentItem: Label {
                            text: materialDelegate.text
                            color: materialDelegate.highlighted ? "#ffffff" : "#bbbbbb"
                            font.pixelSize: 13
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: materialDelegate.highlighted ? "#4a4a7a" : "transparent"
                            radius: 4
                        }
                    }

                    Label {
                        anchors.fill: parent
                        visible: root.sceneMaterials.length === 0
                        text: "No materials"
                        color: "#666688"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                // Color swatches for selected material
                Label {
                    text: "Base color"
                    color: "#aaaacc"
                    font.pixelSize: 11
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 1.2
                    visible: root.selectedMaterial !== null
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: root.selectedMaterial !== null

                    Repeater {
                        model: ["#e74c3c", "#2ecc71", "#3498db", "#f39c12",
                                "#9b59b6", "#1abc9c", "#e67e22", "#ecf0f1"]
                        delegate: Rectangle {
                            width: 28
                            height: 28
                            radius: 14
                            color: modelData
                            border.color: "white"; border.width: 1
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.applyColorToSelected(Qt.color(parent.color))
                            }
                        }
                    }
                }

                // Cameras section
                Label {
                    text: "Cameras"
                    color: "#aaaacc"
                    font.pixelSize: 11
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 1.2
                }

                ListView {
                    id: cameraList
                    Layout.fillWidth: true
                    Layout.preferredHeight: contentHeight
                    clip: true
                    model: root.sceneCameras

                    delegate: ItemDelegate {
                        id: cameraDelegate
                        width: parent.width
                        text: modelData.objectName || "(unnamed)"
                        highlighted: view3d.camera === modelData
                        onClicked: view3d.camera = modelData

                        contentItem: Label {
                            text: cameraDelegate.text
                            color: cameraDelegate.highlighted ? "#ffffff" : "#bbbbbb"
                            font.pixelSize: 13
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: cameraDelegate.highlighted ? "#4a4a7a" : "transparent"
                            radius: 4
                        }
                    }
                }

                // Lights section
                Label {
                    text: "Lights"
                    color: "#aaaacc"
                    font.pixelSize: 11
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 1.2
                }

                ListView {
                    id: lightList
                    Layout.fillWidth: true
                    Layout.preferredHeight: contentHeight
                    clip: true
                    model: root.sceneLights

                    delegate: ItemDelegate {
                        id: lightDelegate
                        width: parent.width
                        padding: 0
                        leftPadding: 4

                        contentItem: RowLayout {
                            spacing: 4

                            Label {
                                Layout.fillWidth: true
                                text: modelData.objectName || "(unnamed)"
                                color: modelData.visible ? "#bbbbbb" : "#555577"
                                font.pixelSize: 13
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            Switch {
                                checked: modelData.visible
                                onToggled: modelData.visible = checked
                            }
                        }

                        background: Item {}
                    }

                    Label {
                        anchors.fill: parent
                        visible: root.sceneLights.length === 0
                        text: "No lights"
                        color: "#666688"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }
}
