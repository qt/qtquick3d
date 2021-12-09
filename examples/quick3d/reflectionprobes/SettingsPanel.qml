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
import QtQuick.Controls

ScrollView {
    property int timeSlicingIndex: timeSlicingComboBox.currentIndex
    property int refreshModeIndex: refreshModeComboBox.currentIndex
    property int qualityIndex: qualityComboBox.currentIndex
    property vector3d probeSize: Qt.vector3d(probeSizeXSlider.value, probeSizeYSlider.value, probeSizeZSlider.value)
    property vector3d probePosition: Qt.vector3d(probePositionXSlider.value, probePositionYSlider.value, probePositionZSlider.value)
    property bool probeParallaxCorrection: parallaxCheckBox.checked
    property real sphereRoughness: materialRoughnessSlider.value
    property bool sphereReceivesReflection: sphereReceivesReflectionsCheckBox.checked
    property bool floorReceivesReflection: floorReceivesReflectionsCheckBox.checked
    property bool spriteParticlesEnabled: particleSystemSpriteCheckBox.checked
    property bool modelParticlesEnabled: particleSystemModelCheckBox.checked
    ScrollBar.vertical.policy: ScrollBar.AlwaysOn
    padding: 10
    background: Rectangle {
        color: "white"
    }

    Flickable {
        clip: true
        contentWidth: settingsArea.width
        contentHeight: settingsArea.height

        Column {
            id: settingsArea
            spacing: 5

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Reflection Map"
                font.pixelSize: 14
            }

            Item { width: 1; height: 10 }

            Text {
                text: "Time Slicing"
            }

            ComboBox {
                id: timeSlicingComboBox
                width: 200
                model: [ "None", "All Faces At Once", "Individual Faces" ]
            }

            Text {
                text: "Refresh Mode"
            }

            ComboBox {
                id: refreshModeComboBox
                width: 200
                model: [ "Every Frame", "First Frame" ]
            }

            Text {
                text: "Reflection Map Quality"
            }

            ComboBox {
                id: qualityComboBox
                width: 200
                model: [ "Very Low", "Low", "Medium", "High", "Very High" ]
            }

            Item { width: 1; height: 10 }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Floor Reflection Probe"
                font.pixelSize: 14
            }

            Item { width: 1; height: 10 }

            CheckBox {
                id: parallaxCheckBox
                checked: true
                text: qsTr("Parallax Correction")
            }

            Text {
                text: "Box Size (" + probeSizeXSlider.value + ", " + probeSizeYSlider.value + ", " + probeSizeZSlider.value + ")"
            }

            Slider {
                id: probeSizeXSlider
                from: 0
                value: 1000
                to: 1000
                stepSize: 1
            }

            Slider {
                id: probeSizeYSlider
                from: 0
                value: 950
                to: 1000
                stepSize: 1
            }

            Slider {
                id: probeSizeZSlider
                from: 0
                value: 650
                to: 1000
                stepSize: 1
            }

            Text {
                text: "Position (" + probePositionXSlider.value + ", " + probePositionYSlider.value + ", " + probePositionZSlider.value + ")"
            }

            Slider {
                id: probePositionXSlider
                from: 0
                value: 0
                to: 500
                stepSize: 1
            }

            Slider {
                id: probePositionYSlider
                from: 0
                value: 0
                to: 500
                stepSize: 1
            }

            Slider {
                id: probePositionZSlider
                from: 0
                value: 0
                to: 500
                stepSize: 1
            }

            Item { width: 1; height: 10 }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Particles"
                font.pixelSize: 14
            }

            CheckBox {
                id: particleSystemSpriteCheckBox
                checked: false
                text: qsTr("Sprite Particles")
            }

            CheckBox {
                id: particleSystemModelCheckBox
                checked: false
                text: qsTr("Model Particles")
            }

            Item { width: 1; height: 10 }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Model"
                font.pixelSize: 14
            }

            Item { width: 1; height: 10 }

            Text {
                text: "Sphere Roughness : " + materialRoughnessSlider.value.toFixed(2);
            }

            Slider {
                id: materialRoughnessSlider
                from: 0
                to: 1.0
                value: 0.1
            }

            CheckBox {
                id: sphereReceivesReflectionsCheckBox
                checked: true
                text: qsTr("Sphere receives reflections")
            }

            CheckBox {
                id: floorReceivesReflectionsCheckBox
                checked: true
                text: qsTr("Floor receives reflections")
            }
        }
    }
}
