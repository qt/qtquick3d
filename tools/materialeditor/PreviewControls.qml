/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.settings
import QtQuick3D

Item {
    id: previewControls
    required property View3D targetView
    property alias modelSource: modelComboBox.currentValue
    property alias enableIBL: iblEnableButton.checked
    property alias enableDirectionalLight: directionalLightEnabledButton.checked


    Settings {
        property alias enableIbl: previewControls.enableIBL
        property alias enableDirectionalLight: previewControls.enableDirectionalLight
        property alias environmentOrientationSliderValue: environmentOrientationSlider.value
    }

    FrostedGlass {
        width: parent.width
        height: layout.implicitHeight
        backgroundItem: targetView
        backgroundRect: Qt.rect(0, 0, width, height)
//        range: 0.05
//        blur: 0.005
        range: 0.05
        blur: 0.05
        //color: "pink"
    }

    RowLayout {
        id: layout
        Label {
            text: "Model"
        }
        ComboBox {
            id: modelComboBox
            textRole: "text"
            valueRole: "value"
            model: ListModel {
                ListElement {
                    text: "Sphere"
                    value: "#Sphere"
                }
                ListElement {
                    text: "Cube"
                    value: "#Cube"
                }
                ListElement {
                    text: "Plane"
                    value: "#Rectangle"
                }
                ListElement {
                    text: "Suzanne"
                    value: "assets/meshes/suzanne.mesh"
                }
            }
        }
        Button {
            text: "Reset View"
            onClicked: {
                targetView.cameraOrigin.rotation = Qt.quaternion(1, 0, 0, 0)
                targetView.camera.rotation = Qt.quaternion(1, 0, 0, 0)
                targetView.camera.position = Qt.vector3d(0, 0, 300)
                environmentOrientationSlider.value = 0
            }
        }
        ToolButton {
            id: iblEnableButton
            icon.source: "qrc:/assets/icons/texture.png"
            checkable: true
            checked: true
            hoverEnabled: true
            ToolTip.delay: 1000
            ToolTip.timeout: 5000
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Toggle the use of IBL")
        }

        Label {
            visible: enableIBL
            text: "Environment Orientation"
        }
        Slider {
            visible: enableIBL
            id: environmentOrientationSlider
            RowLayout.fillWidth: true
            from: -180
            to: 180
            value: 0
            onValueChanged: {
                targetView.environment.probeOrientation = Qt.vector3d(0, value, 0)
            }
        }
        ToolButton {
            id: directionalLightEnabledButton
            icon.source: "qrc:/assets/icons/lightdirectional.png"
            checkable: true
            checked: true
            hoverEnabled: true
            ToolTip.delay: 1000
            ToolTip.timeout: 5000
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Toggle a Directional Light")
        }
    }
}
