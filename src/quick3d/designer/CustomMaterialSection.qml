/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.12

Section {
    caption: qsTr("Custom Material")

    width: parent.width
    SectionLayout {
        Label {
            text: qsTr("Shading Mode")
            tooltip: qsTr("Specifies the type of the material.")
        }
        SecondColumnLayout {
            ComboBox {
                scope: "CustomMaterial"
                model: ["Unshaded", "Shaded"]
                backendValue: backendValues.shadingMode
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Vertex Shader")
            tooltip: qsTr("Holds the location of a vertex shader file for this material.")
        }
        SecondColumnLayout {
            UrlChooser {
                backendValue: backendValues.vertexShader
                filter: "*.*"
            }
        }

        Label {
            text: qsTr("Fragment Shader")
            tooltip: qsTr("Holds the location of a fragment shader file for this material.")
        }
        SecondColumnLayout {
            UrlChooser {
                backendValue: backendValues.fragmentShader
                filter: "*.*"
            }
        }

        Label {
            text: qsTr("Source Blend")
            tooltip: qsTr("Specifies the source blend factor.")
        }
        SecondColumnLayout {
            ComboBox {
                scope: "CustomMaterial"
                model: ["NoBlend", "Zero", "One", "SrcColor", "OneMinusSrcColor", "DstColor", "OneMinusDstColor", "SrcAlpha", "OneMinusSrcAlpha", "DstAlpha", "OneMinusDstAlpha", "ConstantColor", "OneMinusConstantColor", "ConstantAlpha", "OneMinusConstantAlpha", "SrcAlphaSaturate"]
                backendValue: backendValues.sourceBlend
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Destination Blend")
            tooltip: qsTr("Specifies the destination blend factor.")
        }
        SecondColumnLayout {
            ComboBox {
                scope: "CustomMaterial"
                model: ["NoBlend", "Zero", "One", "SrcColor", "OneMinusSrcColor", "DstColor", "OneMinusDstColor", "SrcAlpha", "OneMinusSrcAlpha", "DstAlpha", "OneMinusDstAlpha", "ConstantColor", "OneMinusConstantColor", "ConstantAlpha", "OneMinusConstantAlpha", "SrcAlphaSaturate"]
                backendValue: backendValues.destinationBlend
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Always Dirty")
            tooltip: qsTr("Always dirty material is refreshed every time it is used by QtQuick3D.")
        }
        SecondColumnLayout {
            CheckBox {
                text: backendValues.alwaysDirty.valueToString
                backendValue: backendValues.alwaysDirty
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Line Width")
            tooltip: qsTr("Determines the width of the lines when the geometry is using lines or line strips.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 999999
                minimumValue: 1
                realDragRange: 10
                decimals: 2
                backendValue: backendValues.lineWidth
                Layout.fillWidth: true
            }
        }
    }
}
