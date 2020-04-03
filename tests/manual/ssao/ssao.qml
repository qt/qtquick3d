/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

import QtQuick 2.15
import QtQuick3D 1.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    Text {
        color: "#ffffff"
        style: Text.Outline
        styleColor: "#606060"
        font.pixelSize: 28
        property int api: GraphicsInfo.api
        text: {
            if (GraphicsInfo.api === GraphicsInfo.OpenGL)
                "OpenGL";
            else if (GraphicsInfo.api === GraphicsInfo.Software)
                "Software";
            else if (GraphicsInfo.api === GraphicsInfo.Direct3D12)
                "D3D12";
            else if (GraphicsInfo.api === GraphicsInfo.OpenVG)
                "OpenVG";
            else if (GraphicsInfo.api === GraphicsInfo.OpenGLRhi)
                "OpenGL via QRhi";
            else if (GraphicsInfo.api === GraphicsInfo.Direct3D11Rhi)
                "D3D11 via QRhi";
            else if (GraphicsInfo.api === GraphicsInfo.VulkanRhi)
                "Vulkan via QRhi";
            else if (GraphicsInfo.api === GraphicsInfo.MetalRhi)
                "Metal via QRhi";
            else if (GraphicsInfo.api === GraphicsInfo.Null)
                "Null via QRhi";
            else
                "Unknown API";
        }
    }

    ColumnLayout {
        y: 30
        RowLayout {
            Label {
                text: "aoStrength"
                color: "white"
                font.pointSize: 12
            }
            Label {
                text: aoStrengthSlider.value
                color: "red"
            }
            Slider {
                id: aoStrengthSlider
                from: 0
                to: 100
                value: 0
                stepSize: 1
            }
            Label {
                text: "aoBias"
                color: "white"
                font.pointSize: 12
            }
            Label {
                text: aoBiasSlider.value.toPrecision(2)
                color: "red"
            }
            Slider {
                id: aoBiasSlider
                from: 0
                to: 4
                value: 0.5
                stepSize: 0.01
            }
            Label {
                text: "aoSoftness"
                color: "white"
                font.pointSize: 12
            }
            Label {
                text: aoSoftnessSlider.value
                color: "red"
            }
            Slider {
                id: aoSoftnessSlider
                from: 0
                to: 50
                value: 50
                stepSize: 1
            }
        }
        RowLayout {
            Label {
                text: "aoDistance"
                color: "white"
                font.pointSize: 12
            }
            Label {
                text: aoDistanceSlider.value
                color: "red"
            }
            Slider {
                id: aoDistanceSlider
                from: 0
                to: 100
                value: 5
                stepSize: 1
            }
            Label {
                text: "aoSampleRate"
                color: "white"
                font.pointSize: 12
            }
            Label {
                text: aoSampleRateSlider.value
                color: "red"
            }
            Slider {
                id: aoSampleRateSlider
                from: 2
                to: 4
                value: 2
                stepSize: 1
            }
            Label {
                text: "aoDither"
                color: "white"
                font.pointSize: 12
            }
            CheckBox {
                id: aoDitherCheckBox
                checked: true
            }
        }
        ColumnLayout {
            Label {
                text: "rotation x"
                color: "white"
                font.pointSize: 12
            }
            Label {
                text: rotationXSlider.value
                color: "red"
            }
            Slider {
                id: rotationXSlider
                from: 0
                to: 360
                value: 0
                stepSize: 1
            }
            Label {
                text: "rotation y"
                color: "white"
                font.pointSize: 12
            }
            Label {
                text: rotationYSlider.value
                color: "red"
            }
            Slider {
                id: rotationYSlider
                from: 0
                to: 360
                value: 120
                stepSize: 1
            }
            Label {
                text: "rotation z"
                color: "white"
                font.pointSize: 12
            }
            Label {
                text: rotationZSlider.value
                color: "red"
            }
            Slider {
                id: rotationZSlider
                from: 0
                to: 360
                value: 340
                stepSize: 1
            }
            Label {
                text: "scale"
                color: "white"
                font.pointSize: 12
            }
            Label {
                text: scaleSlider.value
                color: "red"
            }
            Slider {
                id: scaleSlider
                from: 1
                to: 200
                value: 80
                stepSize: 1
            }
        }
    }

    View3D {
        id: v3d
        anchors.fill: parent
        camera: camera
        renderMode: View3D.Overlay

        environment: SceneEnvironment {
            aoStrength: aoStrengthSlider.value
            aoBias: aoBiasSlider.value
            aoSampleRate: aoSampleRateSlider.value
            aoDistance: aoDistanceSlider.value
            aoSoftness: aoSoftnessSlider.value
            aoDither: aoDitherCheckBox.checked
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 200, 300)
            eulerRotation: Qt.vector3d(-30, 0, 0)
        }
        Model {
            source: "object1.mesh"
            scale: Qt.vector3d(scaleSlider.value, scaleSlider.value, scaleSlider.value)
            materials: [ DefaultMaterial {
                    lighting: DefaultMaterial.FragmentLighting
                    cullMode: Material.NoCulling
                } ]
            eulerRotation: Qt.vector3d(rotationXSlider.value, rotationYSlider.value, rotationZSlider.value)
        }
        DirectionalLight {
            id: dirLight
            ambientColor: Qt.rgba(0.1, 0.1, 0.1, 1.0);
        }
    }
}
