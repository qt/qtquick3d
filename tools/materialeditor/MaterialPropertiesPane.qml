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
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D
import MaterialEditorHelpers 1.0

Pane {
    required property MaterialAdapter targetMaterial

    ColumnLayout {
        RowLayout {
            Label {
                text: qsTr("Source Blend")
                Layout.fillWidth: true
            }
            ComboBox {
                id: sourceBlendComboBox
                textRole: "text"
                valueRole: "value"
                implicitContentWidthPolicy: ComboBox.WidestText
                onActivated: targetMaterial.sourceBlend = currentValue
                Component.onCompleted: currentIndex = indexOfValue(targetMaterial.sourceBlend)
                model: [
                    { value: CustomMaterial.NoBlend, text: qsTr("No Blend") },
                    { value: CustomMaterial.Zero, text: qsTr("Zero") },
                    { value: CustomMaterial.One, text: qsTr("One") },
                    { value: CustomMaterial.SrcColor, text: qsTr("Source Color") },
                    { value: CustomMaterial.OneMinusSrcColor, text: qsTr("1 - Source Color") },
                    { value: CustomMaterial.DstColor, text: qsTr("Destination Color") },
                    { value: CustomMaterial.OneMinusDstColor, text: qsTr("1 - Destination Color") },
                    { value: CustomMaterial.SrcAlpha, text: qsTr("Source Alpha") },
                    { value: CustomMaterial.OneMinusSrcAlpha, text: qsTr("1 - Source Alpha") },
                    { value: CustomMaterial.DstAlpha, text: qsTr("Destination Alpha") },
                    { value: CustomMaterial.OneMinusDstAlpha, text: qsTr("1 - Destination Alpha") },
                    { value: CustomMaterial.ConstantColor, text: qsTr("Constant Color") },
                    { value: CustomMaterial.OneMinusConstantColor, text: qsTr("1 - Constant Color") },
                    { value: CustomMaterial.ConstantAlpha, text: qsTr("Constant Alpha") },
                    { value: CustomMaterial.OneMinusConstantAlpha, text: qsTr("1 - Constant Alpha") },
                    { value: CustomMaterial.SrcAlphaSaturate, text: qsTr("Source Alpha Saturate") }
                ]
            }
        }
        RowLayout {
            Label {
                text: qsTr("Destination Blend")
                Layout.fillWidth: true
            }
            ComboBox {
                id: destinationBlendComboBox
                textRole: "text"
                valueRole: "value"
                implicitContentWidthPolicy: ComboBox.WidestText
                onActivated: targetMaterial.destinationBlend = currentValue
                Component.onCompleted: currentIndex = indexOfValue(targetMaterial.destinationBlend)

                model: [
                    { value: CustomMaterial.NoBlend, text: qsTr("No Blend") },
                    { value: CustomMaterial.Zero, text: qsTr("Zero") },
                    { value: CustomMaterial.One, text: qsTr("One") },
                    { value: CustomMaterial.SrcColor, text: qsTr("Source Color") },
                    { value: CustomMaterial.OneMinusSrcColor, text: qsTr("1 - Source Color") },
                    { value: CustomMaterial.DstColor, text: qsTr("Destination Color") },
                    { value: CustomMaterial.OneMinusDstColor, text: qsTr("1 - Destination Color") },
                    { value: CustomMaterial.SrcAlpha, text: qsTr("Source Alpha") },
                    { value: CustomMaterial.OneMinusSrcAlpha, text: qsTr("1 - Source Alpha") },
                    { value: CustomMaterial.DstAlpha, text: qsTr("Destination Alpha") },
                    { value: CustomMaterial.OneMinusDstAlpha, text: qsTr("1 - Destination Alpha") },
                    { value: CustomMaterial.ConstantColor, text: qsTr("Constant Color") },
                    { value: CustomMaterial.OneMinusConstantColor, text: qsTr("1 - Constant Color") },
                    { value: CustomMaterial.ConstantAlpha, text: qsTr("Constant Alpha") },
                    { value: CustomMaterial.OneMinusConstantAlpha, text: qsTr("1 - Constant Alpha") },
                    { value: CustomMaterial.SrcAlphaSaturate, text: qsTr("Source Alpha Saturate") }
                ]
            }
        }
        RowLayout {
            Label {
                text: qsTr("Cull Mode")
                Layout.fillWidth: true
            }
            ComboBox {
                id: cullModeComboBox
                textRole: "text"
                valueRole: "value"
                implicitContentWidthPolicy: ComboBox.WidestText
                onActivated: targetMaterial.cullMode = currentValue
                Component.onCompleted: currentIndex = indexOfValue(targetMaterial.cullMode)
                model: [
                    { value: CustomMaterial.BackFaceCulling, text: qsTr("Back Face Culling") },
                    { value: CustomMaterial.FrontFaceCulling, text: qsTr("Front Face Culling") },
                    { value: CustomMaterial.NoCulling, text: qsTr("No Culling") }
                ]
            }
        }
        RowLayout {
            Label {
                text: qsTr("Depth Draw Mode")
                Layout.fillWidth: true
            }
            ComboBox {
                id: depthDrawModeComboBox
                textRole: "text"
                valueRole: "value"
                implicitContentWidthPolicy: ComboBox.WidestText
                onActivated: targetMaterial.depthDrawMode = currentValue
                Component.onCompleted: currentIndex = indexOfValue(targetMaterial.depthDrawMode)
                model: [
                    { value: CustomMaterial.OpaqueOnlyDepthDraw, text: qsTr("Opaque Only") },
                    { value: CustomMaterial.AlwaysDepthDraw, text: qsTr("Always") },
                    { value: CustomMaterial.NeverDepthDraw, text: qsTr("Never") },
                    { value: CustomMaterial.OpaquePrePassDepthDraw, text: qsTr("Opaque Pre-pass") }
                ]
            }
        }
        RowLayout {
            Label {
                text: qsTr("Shading Mode")
                Layout.fillWidth: true
            }
            ComboBox {
                id: shadingModeComboBox
                textRole: "text"
                valueRole: "value"
                implicitContentWidthPolicy: ComboBox.WidestText
                onActivated: targetMaterial.shadingMode = currentValue
                Component.onCompleted: currentIndex = indexOfValue(targetMaterial.shadingMode)
                model: [
                    { value: CustomMaterial.Shaded, text: qsTr("Shaded") },
                    { value: CustomMaterial.Unshaded, text: qsTr("Unshaded") }
                ]
            }
        }
    }
}
