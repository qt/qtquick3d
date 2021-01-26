/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
****************************************************************************/

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.12

Column {
    width: parent.width

    Section {
        caption: qsTr("Buffer")
        width: parent.width

        SectionLayout {
            Label {
                text: qsTr("Name")
                tooltip: qsTr("Buffer name.")
            }
            SecondColumnLayout {
                LineEdit {
                    backendValue: backendValues.name
                    Layout.fillWidth: true
                    showTranslateCheckBox: false
                }
            }

            Label {
                text: qsTr("Format")
                tooltip: qsTr("Format of the buffer.")
            }
            ComboBox {
                scope: "Buffer"
                model: ["Unknown", "R8", "R16", "R16F", "R32I", "R32UI", "R32F", "RG8", "RGBA8", "RGB8", "SRGB8", "SRGB8A8", "RGB565", "RGBA16F", "RG16F", "RG32F", "RGB32F", "RGBA32F", "R11G11B10", "RGB9E5", "Depth16", "Depth24", "Depth32", "Depth24Stencil8"]
                backendValue: backendValues.format
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Filter")
                tooltip: qsTr("Texture filter for the buffer.")
            }
            ComboBox {
                scope: "Buffer"
                model: ["Unknown", "Nearest", "Linear"]
                backendValue: backendValues.textureFilterOperation
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Coordinate Operation")
                tooltip: qsTr("Texture coordinate operation for the buffer.")
            }
            ComboBox {
                scope: "Buffer"
                model: ["Unknown", "ClampToEdge", "MirroredRepeat", "Repeat"]
                backendValue: backendValues.textureCoordOperation
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Allocation Flags")
                tooltip: qsTr("Allocation flags for the buffer.")
            }
            ComboBox {
                scope: "Buffer"
                model: ["None", "SceneLifetime"]
                backendValue: backendValues.bufferFlags
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Size Multiplier")
                tooltip: qsTr("Defines the size multiplier for the buffer.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 10000
                    minimumValue: 0
                    decimals: 2
                    realDragRange: 30
                    backendValue: backendValues.sizeMultiplier
                    Layout.fillWidth: true
                }
            }
        }
    }
}
