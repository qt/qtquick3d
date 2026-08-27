// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    width: settingsDrawer.width
    height: parent.height
    property alias drawerVisible: settingsDrawer.visible
    property bool drawerOpen: settingsDrawer.position === 1.0

    property real shadowFactor: sliderDirectionaLightShadowFactor.value
    property vector3d eulerRotation: Qt.vector3d(sliderDirectionalLightRotX.value,
                                                       sliderDirectionalLightRotY.value, 0)
    property real csmSplit1: sliderCSMSplit1.value
    property real csmSplit2: sliderCSMSplit2.value
    property real csmSplit3: sliderCSMSplit3.value
    property int csmNumSplits: sliderNumSplits.currentIndex
    property int shadowMapQuality: shadowmapquality_combobox.currentIndex
    property real csmBlendRatio: sliderBlendRatio.value
    property real shadowBias: sliderShadowBiasDirLight.value
    property real pcfFactor: sliderPCFFactor.value
    property int softShadowQuality: softshadowquality_combobox.currentIndex
    property real shadowMapFar: sliderShadowMapFar.value
    property real clipFar: sliderCameraClipFar.value
    property bool lockShadowmapTexels: checkboxLockShadowmapTexels.checked
    property real fogSpeed: fogSpeedSlider.value
    property real jitterIntensity: jitterIntensitySlider.value
    property bool enableMotionVectorTAA: checkboxEnableMotionVectorTAA.checked
    property bool spotLightEnabled: checkboxSpotLight.checked
    property bool dirLightEnabled: checkboxDirLight.checked
    property bool pointLightEnabled: checkboxPointLight.checked
    property bool iesLights: checkboxIESLights.checked
    property int iesLightIndex: iesIndexSpinBox.value
    property bool spotLightShadow: checkboxSpotLightShadow.checked
    property bool dirLightShadow: checkboxDirLightShadow.checked
    property bool pointLightShadow: checkboxPointLightShadow.checked
    property bool iesLightsShadow: checkboxIESLightsShadow.checked
    property bool sceneVisible: checkboxScene.checked

    property color fogVolumeColor: Qt.rgba(fogVolumeColorR.value, fogVolumeColorG.value, fogVolumeColorB.value, 1.0)
    property real fogVolumeDensity: fogVolumeDensitySlider.value
    property real fogVolumeNoiseScale: fogVolumeNoiseScaleSlider.value
    property bool fogVolumeHeightEnabled: fogVolumeHeightEnabledCheck.checked
    property real fogVolumeHeightLeastY: fogVolumeHeightLeastYSlider.value
    property real fogVolumeHeightMostY: fogVolumeHeightMostYSlider.value
    property real fogVolumeHeightCurve: fogVolumeHeightCurveSlider.value

    property real viewX: settingsDrawer.visible ? (settingsDrawer.x + settingsDrawer.width) : 0

    readonly property color clBg: "#11111b"
    readonly property color clSurface: "#1e1e2e"
    readonly property color clSurface2: "#27273d"
    readonly property color clBorder: "#313244"
    readonly property color clTextPrimary: "#cdd6f4"
    readonly property color clTextMuted: "#6c7086"
    readonly property color clBlue: "#89b4fa"
    readonly property color clGreen: "#a6e3a1"
    readonly property color clYellow: "#f9e2af"
    readonly property color clRed: "#f38ba8"
    readonly property color clPurple: "#cba6f7"
    readonly property color clTeal: "#94e2d5"
    readonly property color clOrange: "#fab387"

    RoundButton {
        id: iconOpen
        icon.source: "assets/sliders.svg"
        icon.width: 25
        icon.height: 25
        padding: 10
        x: padding + root.viewX
        y: padding
        onClicked: {
            settingsDrawer.visible = !settingsDrawer.visible;
        }
    }

    Drawer {
        id: settingsDrawer
        edge: Qt.LeftEdge
        interactive: false
        modal: false
        height: parent.height
        width: 300

        background: Rectangle { color: root.clBg }

        enter: Transition { NumberAnimation { property: "position"; to: 1.0; duration: 280; easing.type: Easing.OutCubic } }
        exit: Transition { NumberAnimation { property: "position"; to: 0.0; duration: 280; easing.type: Easing.InCubic } }

        Page {
            anchors.fill: parent
            background: Rectangle { color: root.clBg }

            header: Rectangle {
                width: parent.width
                height: 46
                color: root.clSurface
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width; height: 1
                    color: root.clBorder
                }
                Label {
                    anchors.centerIn: parent
                    text: "SETTINGS"
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    color: root.clTextPrimary
                }
            }

            ScrollView {
                anchors.fill: parent
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                contentWidth: availableWidth

                ColumnLayout {
                    id: settingsArea
                    width: parent.width
                    spacing: 0

                    component SectionHeader : Rectangle {
                        id: secHdr
                        property string title: ""
                        property color  accent: root.clBlue
                        property bool   expanded: true

                        Layout.fillWidth: true
                        height: 34
                        color: Qt.rgba(secHdr.accent.r, secHdr.accent.g, secHdr.accent.b, 0.09)

                        Rectangle {
                            width: 3; height: parent.height
                            color: secHdr.accent; radius: 1
                        }
                        Label {
                            anchors { left: parent.left; leftMargin: 14; verticalCenter: parent.verticalCenter }
                            text: secHdr.title
                            font.pixelSize: 10; font.weight: Font.DemiBold
                            font.capitalization: Font.AllUppercase;
                            color: secHdr.accent
                        }
                        Label {
                            anchors { right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter }
                            text: secHdr.expanded ? "▴" : "▾"
                            color: Qt.rgba(secHdr.accent.r, secHdr.accent.g, secHdr.accent.b, 0.55)
                            font.pixelSize: 10
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: secHdr.expanded = !secHdr.expanded
                        }
                    }

                    component SliderRow : ColumnLayout {
                        id: slRow
                        property string label: ""
                        property alias  value: sl.value
                        property alias  from: sl.from
                        property alias  to: sl.to
                        property alias  stepSize: sl.stepSize
                        property int    numDecimals: 0
                        property color  accentColor: root.clBlue

                        readonly property real handleSize: 14

                        Layout.fillWidth: true
                        Layout.leftMargin: 14; Layout.rightMargin: 14; Layout.topMargin: 8
                        spacing: 3

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Label {
                                text: slRow.label
                                font.pixelSize: 11; color: root.clTextMuted
                                Layout.fillWidth: true; elide: Text.ElideRight
                            }
                            Label {
                                text: sl.value.toFixed(slRow.numDecimals)
                                font.pixelSize: 11; font.weight: Font.Medium
                                color: root.clTextPrimary
                                horizontalAlignment: Text.AlignRight
                                Layout.minimumWidth: 44
                            }
                        }

                        Slider {
                            id: sl
                            Layout.fillWidth: true
                            implicitHeight: 22
                            stepSize: 0.01

                            background: Item {
                                x: sl.leftPadding
                                width: sl.availableWidth
                                implicitWidth: 200; implicitHeight: 4
                                Rectangle {
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: parent.width; height: 4; radius: 2
                                    color: root.clBorder
                                    Rectangle {
                                        width: slRow.handleSize / 2
                                               + sl.visualPosition * (parent.width - slRow.handleSize)
                                        height: parent.height; radius: 2
                                        color: slRow.accentColor
                                    }
                                }
                            }
                            handle: Rectangle {
                                x: sl.leftPadding + sl.visualPosition * (sl.availableWidth - width)
                                y: sl.topPadding + sl.availableHeight / 2 - height / 2
                                implicitWidth: slRow.handleSize
                                implicitHeight: slRow.handleSize
                                radius: slRow.handleSize / 2
                                color: sl.pressed ? Qt.lighter(slRow.accentColor, 1.5)
                                                  : (sl.hovered ? Qt.lighter(slRow.accentColor, 1.2)
                                                                : slRow.accentColor)
                                Behavior on color { ColorAnimation { duration: 100 } }
                            }
                        }
                    }

                    component StyledCheckBox : CheckBox {
                        id: styledCb
                        property color accentColor: root.clBlue

                        Layout.leftMargin: 14; Layout.topMargin: 4

                        contentItem: Label {
                            text: styledCb.text
                            leftPadding: styledCb.indicator.width + 6
                            color: root.clTextPrimary; font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                        }
                        indicator: Rectangle {
                            implicitWidth: 16; implicitHeight: 16
                            x: styledCb.leftPadding
                            y: (styledCb.height - height) / 2
                            radius: 3
                            color: styledCb.checked ? styledCb.accentColor : "transparent"
                            border.color: styledCb.checked ? styledCb.accentColor : root.clBorder
                            border.width: 1.5
                            Behavior on color { ColorAnimation { duration: 100 } }
                            Label {
                                anchors.centerIn: parent
                                text: "✓"; font.pixelSize: 10; color: root.clBg
                                visible: styledCb.checked
                            }
                        }
                    }

                    component ShadowCheckBox : CheckBox {
                        id: shadowCb
                        property color accentColor: root.clBlue

                        contentItem: Label {
                            text: "shadow"
                            leftPadding: shadowCb.indicator.width + 4
                            color: root.clTextMuted; font.pixelSize: 10
                            verticalAlignment: Text.AlignVCenter
                        }
                        indicator: Rectangle {
                            implicitWidth: 13; implicitHeight: 13
                            x: shadowCb.leftPadding
                            y: (shadowCb.height - height) / 2
                            radius: 2
                            color: shadowCb.checked ? shadowCb.accentColor : "transparent"
                            border.color: shadowCb.checked ? shadowCb.accentColor : root.clBorder
                            border.width: 1.5
                            Behavior on color { ColorAnimation { duration: 100 } }
                            Label {
                                anchors.centerIn: parent
                                text: "✓"; font.pixelSize: 9; color: root.clBg
                                visible: shadowCb.checked
                            }
                        }
                    }

                    component ComboRow : RowLayout {
                        id: comboRow
                        property string label: ""
                        property alias  model: cb.model
                        property alias  currentIndex: cb.currentIndex
                        property color  accentColor: root.clBlue
                        property int    comboWidth: 100

                        Layout.fillWidth: true
                        Layout.leftMargin: 14; Layout.rightMargin: 14; Layout.topMargin: 8
                        spacing: 8

                        Label {
                            text: comboRow.label
                            font.pixelSize: 11; color: root.clTextMuted
                            Layout.fillWidth: true; elide: Text.ElideRight
                        }
                        ComboBox {
                            id: cb
                            implicitWidth: comboRow.comboWidth; implicitHeight: 26
                            font.pixelSize: 11
                            contentItem: Label {
                                leftPadding: 8; rightPadding: 24
                                text: cb.currentText
                                color: root.clTextPrimary; font.pixelSize: 11
                                verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight
                            }
                            indicator: Label {
                                x: cb.width - width - 6
                                y: (cb.height - height) / 2
                                text: "▾"; font.pixelSize: 9
                                color: root.clTextMuted
                            }
                            background: Rectangle {
                                color: cb.hovered ? root.clSurface2 : root.clSurface
                                radius: 4
                                border.color: cb.hovered ? comboRow.accentColor : root.clBorder
                                border.width: 1
                                Behavior on border.color { ColorAnimation { duration: 120 } }
                            }
                            popup: Popup {
                                y: cb.height + 2
                                width: cb.width
                                padding: 4
                                background: Rectangle {
                                    color: root.clSurface2; radius: 5
                                    border.color: root.clBorder; border.width: 1
                                }
                                contentItem: ListView {
                                    clip: true
                                    implicitHeight: Math.min(contentHeight, 180)
                                    model: cb.delegateModel
                                    ScrollBar.vertical: ScrollBar {}
                                }
                            }
                            delegate: ItemDelegate {
                                width: cb.width - 8
                                height: 26
                                highlighted: cb.highlightedIndex === index
                                contentItem: Label {
                                    text: modelData
                                    color: highlighted ? root.clTextPrimary : root.clTextMuted
                                    font.pixelSize: 11
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: 4
                                }
                                background: Rectangle {
                                    color: highlighted ? root.clBorder : "transparent"
                                    radius: 3
                                }
                            }
                        }
                    }

                    component SectionSep : Rectangle {
                        Layout.fillWidth: true
                        height: 1; color: root.clBorder
                    }

                    SectionHeader { id: hdrDirLight; title: "Directional Light"; accent: root.clYellow }

                    ColumnLayout {
                        visible: hdrDirLight.expanded
                        Layout.fillWidth: true; Layout.bottomMargin: 6; spacing: 0

                        SliderRow {
                            id: sliderDirectionalLightRotX
                            label: "Rotation X"
                            value: -160; from: -180; to: 0
                            accentColor: root.clYellow
                        }
                        SliderRow {
                            id: sliderDirectionalLightRotY
                            label: "Rotation Y"
                            value: 10; from: -180; to: 180
                            accentColor: root.clYellow
                        }
                    }

                    SectionSep {}

                    SectionHeader { id: hdrLights; title: "Lights"; accent: root.clOrange }

                    ColumnLayout {
                        visible: hdrLights.expanded
                        Layout.fillWidth: true; Layout.bottomMargin: 8; spacing: 0

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 14; Layout.topMargin: 4; Layout.rightMargin: 14
                            spacing: 8
                            ShadowCheckBox {
                                id: checkboxSpotLightShadow
                                checked: true; accentColor: root.clYellow
                                opacity: checkboxSpotLight.checked ? 1.0 : 0.35
                                Behavior on opacity { NumberAnimation { duration: 120 } }
                            }
                            StyledCheckBox {
                                id: checkboxSpotLight
                                checked: true; text: "Spot Light"
                                accentColor: root.clYellow
                                Layout.leftMargin: 0; Layout.topMargin: 0; Layout.fillWidth: true
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 14; Layout.topMargin: 4; Layout.rightMargin: 14
                            spacing: 8
                            ShadowCheckBox {
                                id: checkboxDirLightShadow
                                checked: true; accentColor: root.clYellow
                                opacity: checkboxDirLight.checked ? 1.0 : 0.35
                                Behavior on opacity { NumberAnimation { duration: 120 } }
                            }
                            StyledCheckBox {
                                id: checkboxDirLight
                                checked: true; text: "Directional Light"
                                accentColor: root.clYellow
                                Layout.leftMargin: 0; Layout.topMargin: 0; Layout.fillWidth: true
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 14; Layout.topMargin: 4; Layout.rightMargin: 14
                            spacing: 8
                            ShadowCheckBox {
                                id: checkboxPointLightShadow
                                checked: true; accentColor: root.clRed
                                opacity: checkboxPointLight.checked ? 1.0 : 0.35
                                Behavior on opacity { NumberAnimation { duration: 120 } }
                            }
                            StyledCheckBox {
                                id: checkboxPointLight
                                checked: true; text: "Point Light"
                                accentColor: root.clRed
                                Layout.leftMargin: 0; Layout.topMargin: 0; Layout.fillWidth: true
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 14; Layout.topMargin: 4; Layout.rightMargin: 14
                            spacing: 8

                            ShadowCheckBox {
                                id: checkboxIESLightsShadow
                                checked: true; accentColor: root.clOrange
                                opacity: checkboxIESLights.checked ? 1.0 : 0.35
                                Behavior on opacity { NumberAnimation { duration: 120 } }
                            }

                            StyledCheckBox {
                                id: checkboxIESLights
                                checked: false; text: "IES Lights"
                                onCheckStateChanged: {
                                    if (checkState) {
                                        checkboxDirLight.checked = false
                                        checkboxSpotLight.checked = false
                                        checkboxPointLight.checked = false
                                    }
                                }
                                accentColor: root.clOrange
                                Layout.leftMargin: 0; Layout.topMargin: 0; Layout.fillWidth: true
                            }

                            Label {
                                text: "Index"
                                font.pixelSize: 11
                                color: root.clTextMuted
                                opacity: checkboxIESLights.checked ? 1.0 : 0.35
                                Behavior on opacity { NumberAnimation { duration: 120 } }
                            }

                            SpinBox {
                                id: iesIndexSpinBox
                                from: 0; to: 5; value: 0
                                implicitWidth: 84; implicitHeight: 24
                                enabled: checkboxIESLights.checked
                                opacity: enabled ? 1.0 : 0.35
                                Behavior on opacity { NumberAnimation { duration: 120 } }

                                background: Rectangle {
                                    color: root.clSurface2
                                    border.color: iesIndexSpinBox.activeFocus ? root.clOrange : root.clBorder
                                    border.width: 1
                                    radius: 4
                                    Behavior on border.color { ColorAnimation { duration: 100 } }
                                }
                                contentItem: TextInput {
                                    text: iesIndexSpinBox.textFromValue(iesIndexSpinBox.value, iesIndexSpinBox.locale)
                                    color: root.clTextPrimary
                                    font.pixelSize: 11
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    readOnly: !iesIndexSpinBox.editable
                                    validator: iesIndexSpinBox.validator
                                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                                }
                            }
                        }
                        StyledCheckBox {
                            id: checkboxScene
                            checked: true; text: "Scene"
                            accentColor: root.clTeal
                        }
                    }

                    SectionSep {}

                    SectionHeader { id: hdrFog; title: "Fog"; accent: root.clTeal }

                    ColumnLayout {
                        visible: hdrFog.expanded
                        Layout.fillWidth: true; Layout.bottomMargin: 6; spacing: 0

                        SliderRow {
                            id: fogSpeedSlider
                            label: "Speed"
                            value: 0.4; from: 0; to: 1; numDecimals: 3
                            accentColor: root.clTeal
                        }
                        SliderRow {
                            id: fogVolumeNoiseScaleSlider
                            label: "Noise Scale"
                            value: 0.30; from: 0.01; to: 4.0; numDecimals: 2
                            accentColor: root.clTeal
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 14; Layout.rightMargin: 14; Layout.topMargin: 8
                            spacing: 4
                            Label {
                                text: "Color"
                                font.pixelSize: 11; color: root.clTextMuted
                                Layout.fillWidth: true; elide: Text.ElideRight
                            }
                            Rectangle {
                                width: 44; height: 14; radius: 2
                                color: root.fogVolumeColor
                                border.color: root.clBorder; border.width: 1
                            }
                        }
                        SliderRow { id: fogVolumeColorR; label: "R"; from: 0; to: 1; value: 1.0; numDecimals: 2; accentColor: root.clRed;   Layout.topMargin: 2 }
                        SliderRow { id: fogVolumeColorG; label: "G"; from: 0; to: 1; value: 1.0; numDecimals: 2; accentColor: root.clGreen; Layout.topMargin: 2 }
                        SliderRow { id: fogVolumeColorB; label: "B"; from: 0; to: 1; value: 1.0; numDecimals: 2; accentColor: root.clBlue;  Layout.topMargin: 2 }

                        SliderRow {
                            id: fogVolumeDensitySlider
                            label: "Density"
                            value: 0.5; from: 0; to: 3; numDecimals: 2
                            accentColor: root.clTeal
                        }

                        StyledCheckBox {
                            id: fogVolumeHeightEnabledCheck
                            text: "Height Falloff"
                            accentColor: root.clTeal
                        }
                        SliderRow {
                            id: fogVolumeHeightLeastYSlider
                            label: "Least Intense Y"
                            from: -2000; to: 3000; stepSize: 50; value: 900; numDecimals: 0
                            accentColor: root.clTeal
                            visible: fogVolumeHeightEnabledCheck.checked
                        }
                        SliderRow {
                            id: fogVolumeHeightMostYSlider
                            label: "Most Intense Y"
                            from: -2000; to: 3000; stepSize: 50; value: 800; numDecimals: 0
                            accentColor: root.clTeal
                            visible: fogVolumeHeightEnabledCheck.checked
                        }
                        SliderRow {
                            id: fogVolumeHeightCurveSlider
                            label: "Height Curve"
                            from: 0.1; to: 5.0; stepSize: 0.1; value: 1.0; numDecimals: 1
                            accentColor: root.clTeal
                            visible: fogVolumeHeightEnabledCheck.checked
                        }
                    }

                    SectionSep {}

                    SectionHeader { id: hdrPost; title: "Post-Process"; accent: root.clGreen }

                    ColumnLayout {
                        visible: hdrPost.expanded
                        Layout.fillWidth: true; Layout.bottomMargin: 10; spacing: 0

                        StyledCheckBox {
                            id: checkboxEnableMotionVectorTAA
                            checked: true; text: "Motion Vector TAA"
                            accentColor: root.clGreen
                        }
                        SliderRow {
                            id: jitterIntensitySlider
                            label: "Jitter Intensity"
                            value: 0.015; from: 0; to: 0.1; stepSize: 0.001; numDecimals: 3
                            accentColor: root.clGreen
                        }
                    }


                    SectionHeader { id: hdrShadows; title: "Shadows"; accent: root.clRed }

                    ColumnLayout {
                        visible: hdrShadows.expanded
                        Layout.fillWidth: true; Layout.bottomMargin: 6; spacing: 0

                        SliderRow {
                            id: sliderDirectionaLightShadowFactor
                            label: "Shadow Factor"
                            value: 75; from: 0; to: 100; stepSize: 1
                            accentColor: root.clRed
                        }
                        SliderRow {
                            id: sliderShadowBiasDirLight
                            label: "Shadow Bias"
                            value: 10; from: 0; to: 30; stepSize: 1
                            accentColor: root.clRed
                        }
                        SliderRow {
                            id: sliderShadowMapFar
                            label: "Shadow Map Far"
                            value: 10000; from: 0; to: 30000; stepSize: 1000
                            accentColor: root.clRed
                        }

                        ComboRow {
                            id: softshadowquality_combobox
                            label: "Soft Shadow"
                            model: ["Hard", "PCF4", "PCF8", "PCF16"]
                            currentIndex: 3
                            accentColor: root.clRed; comboWidth: 90
                        }
                        ComboRow {
                            id: shadowmapquality_combobox
                            label: "Map Quality"
                            model: ["Low", "Medium", "High", "VeryHigh", "Ultra"]
                            currentIndex: 2
                            accentColor: root.clRed; comboWidth: 90
                        }

                        StyledCheckBox {
                            id: checkboxLockShadowmapTexels
                            checked: false; text: "Lock Shadowmap Texels"
                            accentColor: root.clRed
                        }
                    }

                    SectionSep {}

                    SectionHeader { id: hdrCSM; title: "Cascade Shadows"; accent: root.clPurple }

                    ColumnLayout {
                        visible: hdrCSM.expanded
                        Layout.fillWidth: true; Layout.bottomMargin: 6; spacing: 0

                        ComboRow {
                            id: sliderNumSplits
                            label: "Num Splits"
                            model: ["0", "1", "2", "3"]
                            currentIndex: 0
                            accentColor: root.clPurple; comboWidth: 60
                        }
                        SliderRow {
                            id: sliderCSMSplit1
                            label: "Split 1"
                            value: 0.15; from: 0.01; to: 0.99; numDecimals: 2
                            accentColor: root.clPurple
                        }
                        SliderRow {
                            id: sliderCSMSplit2
                            label: "Split 2"
                            value: 0.5; from: 0.01; to: 0.99; numDecimals: 2
                            accentColor: root.clPurple
                        }
                        SliderRow {
                            id: sliderCSMSplit3
                            label: "Split 3"
                            value: 0.75; from: 0.01; to: 0.99; numDecimals: 2
                            accentColor: root.clPurple
                        }
                        SliderRow {
                            id: sliderBlendRatio
                            label: "Blend Ratio"
                            value: 0.05; from: 0; to: 1; numDecimals: 2
                            accentColor: root.clPurple
                        }
                        SliderRow {
                            id: sliderPCFFactor
                            label: "PCF Factor"
                            value: 3; from: 0; to: 30; stepSize: 1
                            accentColor: root.clPurple
                        }
                    }

                    SectionHeader { id: hdrCamera; title: "Camera"; accent: root.clBlue }

                    ColumnLayout {
                        visible: hdrCamera.expanded
                        Layout.fillWidth: true; Layout.bottomMargin: 6; spacing: 0

                        SliderRow {
                            id: sliderCameraClipFar
                            label: "Clip Far"
                            value: 4000; from: 0; to: 30000; stepSize: 1000
                            accentColor: root.clBlue
                        }
                    }

                    SectionSep {}

                    Item { Layout.minimumHeight: 16 }
                }
            }
        }
    }
}
