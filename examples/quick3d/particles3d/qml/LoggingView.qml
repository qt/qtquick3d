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
import QtQuick3D.Particles3D
import QtQuick.Controls

Frame {
    property var particleSystems
    readonly property bool loggingEnabled: settings.showLoggingView
    property bool intervalInstant: false
    property real itemWidth: (width - loggingButton.width - intervalButton.width) / 6

    width: parent.width
    height: tableContent.height + 30

    Component.onCompleted: {
        for (const psystem of particleSystems)
            psystem.logging = settings.showLoggingView;
    }

    // Background
    background: Rectangle {
        color: "#80000000"
        visible: loggingEnabled
    }

    Button {
        id: loggingButton
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 10
        opacity: loggingEnabled ? 1.0 : 0.4
        icon.source: "qrc:/qml/images/icon_logging.png"
        icon.width: 32
        icon.height: 32
        icon.color: "transparent"
        background: Rectangle {
            color: "transparent"
        }
        onClicked: {
            settings.showLoggingView = !settings.showLoggingView
            for (const psystem of particleSystems) {
                psystem.logging = settings.showLoggingView;
            }
        }
    }

    Button {
        id: intervalButton
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: loggingButton.left
        anchors.rightMargin: 0
        visible: loggingEnabled
        opacity: intervalInstant ? 1.0 : 0.2
        icon.source: "qrc:/qml/images/icon_interval.png"
        icon.width: 32
        icon.height: 32
        icon.color: "transparent"
        background: Rectangle {
            color: "transparent"
        }
        onClicked: {
            intervalInstant = !intervalInstant;
            var interval = intervalInstant ? 0 : 1000;
            for (const psystem of particleSystems)
                psystem.loggingData.loggingInterval = interval;
        }
    }

    Component {
        id: systemItem
        Row {
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: modelData.seed
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: modelData.loggingData.updates
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: modelData.loggingData.particlesMax
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: modelData.loggingData.particlesUsed
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: modelData.loggingData.time.toFixed(4)
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: modelData.loggingData.timeAverage.toFixed(4)
            }
        }
    }

    Column {
        id: tableContent
        width: parent.width
        anchors.verticalCenter: parent.verticalCenter
        visible: loggingEnabled
        Row {
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: qsTr("SEED")
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: qsTr("UPDATES")
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: qsTr("P. MAX")
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: qsTr("P. USED")
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: qsTr("TIME")
            }
            Text {
                width: itemWidth
                horizontalAlignment: Text.AlignHCenter
                color: "#ffffff"
                font.pointSize: settings.fontSizeSmall
                text: qsTr("TIME AVG.")
            }
        }
        Repeater {
            model: particleSystems
            delegate: systemItem
        }
    }
}
