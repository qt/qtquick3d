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
import QtQuick.Window
import QtQuick.Controls

import Qt.labs.platform
import Qt.labs.settings

import QtQuick3D
import QtQuick3D.Helpers
import QtQuick3D.AssetUtils

Window {
    visible: true
    width: 800
    height: 600

    property url importUrl;

    View3D {
        id: view3D
        anchors.fill: parent

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.SkyBox
            lightProbe: Texture {
                source: "maps/OpenfootageNET_lowerAustria01-1024.hdr"
            }
        }

        DirectionalLight {
            eulerRotation: "-30, -20, 0"
            ambientColor: "#777"
        }

        PerspectiveCamera {
            id: camera
            Component.onCompleted: view3D.resetView()
        }

        function resetView() {
            camera.position = "0, 150, 400"
            camera.eulerRotation = "0, 0, 0"
            wheelHandler.factor = 1.0
            import_node.eulerRotation = "0, 0, 0"
        }

        RuntimeLoader {
            property real sf: wheelHandler.factor * 10
            scale: Qt.vector3d(sf, sf, sf)
            id: import_node
            source: importUrl
        }

        Rectangle {
            visible: import_node.status !== RuntimeLoader.Success

            color: "red"
            width: parent.width * 0.8
            height: parent.height * 0.8
            anchors.centerIn: parent
            radius: Math.min(width, height) / 10
            Text {
                anchors.fill: parent
                font.pixelSize: 36
                text: "Status: " + import_node.errorString + "\nPress \"Import...\" to import a model"
                color: "white"
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            opacity: 0.6
        }
    }

    WheelHandler {
        id: wheelHandler
        property real factor: 1.0
        onWheel: (event)=> {
            if (event.angleDelta.y > 0)
                factor *= 1.1
            else
                factor /= 1.1
       }
    }

    PointHandler {
        id: rotateHandler
        acceptedButtons: Qt.MiddleButton
        onPointChanged: {
            if (Math.abs(point.velocity.x) >= Math.abs(point.velocity.y))
                import_node.eulerRotation.y += point.velocity.x / 2000
            else
                import_node.eulerRotation.x += point.velocity.y / 2000
        }
    }
    WasdController {
        controlledObject: camera
        focus: true
    }

    Row {
        RoundButton {
            id: importButton
            text: "Import..."
            onClicked: fileDialog.open()
            focusPolicy: Qt.NoFocus
        }
        RoundButton {
            id: resetButton
            text: "Reset view"
            onClicked: view3D.resetView()
            focusPolicy: Qt.NoFocus
        }
        RoundButton {
            id: scaleButton
            text: "Scale: " + import_node.sf.toPrecision(3)
            onClicked: wheelHandler.factor = 1
            focusPolicy: Qt.NoFocus
        }
    }
    FileDialog {
        id: fileDialog
        nameFilters: ["glTF files (*.gltf *.glb)", "All files (*)"]
        onAccepted: importUrl = file
        Settings {
            id: fileDialogSettings
            category: "QtQuick3D.Examples.RuntimeLoader"
            property alias folder: fileDialog.folder
        }
    }
}
