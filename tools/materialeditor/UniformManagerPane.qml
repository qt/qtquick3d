// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D
import QtQuick.Dialogs
import QtQuick3D.MaterialEditor
import Qt.labs.qmlmodels

Pane {
    id: uniformManagerPane
    property alias uniformModel: uniformModel
    required property MaterialAdapter materialAdapter
    SplitView {
        anchors.fill: parent
        ColumnLayout {
            clip: true
            RowLayout {
                id: tableControls

                function insertUniform() {
                    let rowCount = materialAdapter.uniformModel.rowCount;
                    if (materialAdapter.uniformModel.insertRow(rowCount, typeComboBox.currentIndex, uniformNameTextInput.text))
                        uniformNameTextInput.text = ""
                }

                Label {
                    text: "Type:"
                }

                ComboBox {
                    id: typeComboBox
                    textRole: "text"
                    valueRole: "value"
                    model: [
                        { value: uniformModel.Bool, text: "bool" },
                        { value: uniformModel.Int, text: "int" },
                        { value: uniformModel.Float, text: "float" },
                        { value: uniformModel.Vec2, text: "vec2" },
                        { value: uniformModel.Vec3, text: "vec3" },
                        { value: uniformModel.Vec4, text: "vec4" },
                        { value: uniformModel.Mat44, text: "mat44" },
                        { value: uniformModel.Sampler, text: "sampler" }
                    ]
                }

                TextField {
                    id: uniformNameTextInput
                    validator: RegularExpressionValidator {
                        regularExpression: /[a-zA-Z_][a-zA-Z0-9_]+/
                    }
                    Layout.fillWidth: true
                    placeholderText: "Uniform Name"
                    onAccepted: tableControls.insertUniform()
                }
                Button {
                    id: addButton
                    text: "Add"
                    enabled: uniformNameTextInput.text != ""
                    onClicked: tableControls.insertUniform()
                }
            }

            //          Column Header
            Row {
                id: columnsHeader
                Layout.fillWidth: true
                Label {
                    width: uniformTable.columnWidth(0)
                    text: "Type"
                    verticalAlignment: Text.AlignVCenter
                }
                Label {
                    width: uniformTable.columnWidth(1)
                    text: "Name"
                    verticalAlignment: Text.AlignVCenter
                }
                Label {
                    width: uniformTable.columnWidth(2)
                    text: "Value"
                    verticalAlignment: Text.AlignVCenter
                }
            }
            ListView {
                id: uniformTable
                Layout.fillHeight: true
                Layout.fillWidth: true
                flickableDirection: Flickable.VerticalFlick
                model: UniformModel {
                    id: uniformModel
                }
                clip: true
                ScrollBar.vertical: ScrollBar { }
                highlight: Rectangle {
                    color: palette.highlight
                }

                property var typeStrings: [
                    "bool",
                    "int",
                    "float",
                    "vec2",
                    "vec3",
                    "vec4",
                    "mat44",
                    "sampler"
                ]

                function convertValueToString(value, type)
                {
                    if (type === 0) {
                        // bool
                        return String(value);
                    } if (type === 1) {
                        // int
                        return String(value);
                    } if (type === 2) {
                        // float
                        return String(value);
                    } if (type === 3) {
                        // vec2
                        return "(" + value.x + ", " + value.y + ")"
                    } if (type === 4) {
                        // vec3
                        return "(" + value.x + ", " + value.y + ", " + value.z + ")"
                    } if (type === 5) {
                        // vec4
                        return "(" + value.x + ", " + value.y + ", " + value.z + ", " + value.w + ")"
                    } if (type === 6) {
                        // mat44
                        return value.toString()
                    } if (type === 7) {
                        // sampler
                        return "[Texture]"
                    }
                }

                function columnWidth(column) {
                    if (column === 0)
                        return 50;
                    if (column === 1)
                        return 100;
                    return 100;
                }

                delegate: Item {
                    width: ListView.view.width
                    height: typeLabel.implicitHeight
                    Row {
                        Label {
                            id: typeLabel
                            width: uniformTable.columnWidth(0)
                            text: uniformTable.typeStrings[type]
                        }
                        Label {
                            width: uniformTable.columnWidth(1)
                            text: name
                        }
                        Label {
                            width: uniformTable.columnWidth(2)
                            Layout.fillWidth: true
                            text: uniformTable.convertValueToString(value, type)
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            uniformTable.currentIndex = index
                        }
                    }
                }
            }
        }


        Item {
            id: uniformValueEditor
            width: parent.width * 0.5
            clip: true

            Label {
                id: emptyLabel
                visible: uniformTable.currentIndex == -1
                anchors.centerIn: parent
                text: "Select a uniform to edit"
            }

            Repeater {
                anchors.fill: parent
                model: uniformModel
                Item {
                    anchors.fill: parent
                    anchors.margins: 10
                    visible: index === uniformTable.currentIndex

                    Item {
                        id: header
                        width: parent.width
                        anchors.top: parent.top
                        height: removeButton.implicitHeight
                        RowLayout {
                            anchors.fill: parent
                            id: headerLayout
                            Label {
                                text: "Uniform: " + name
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                            Button {
                                id: removeButton
                                text: "Remove"
                                Layout.alignment: Qt.AlignRight
                                onClicked: {
                                    materialAdapter.uniformModel.removeRow(uniformTable.currentIndex, 1)
                                }
                            }
                        }
                    }

                    Loader {
                        id: editorLoader
                        anchors.top: header.bottom
                        anchors.right: parent.right
                        anchors.left: parent.left
                        anchors.bottom: parent.bottom
                        sourceComponent: editors[type]

                        property var editors: [
                            boolEditor,
                            intEditor,
                            floatEditor,
                            vec2Editor,
                            vec3Editor,
                            vec4Editor,
                            mat44Editor,
                            samplerEditor
                        ]

                        Component {
                            id: boolEditor
                            CheckBox {
                                text: "value"
                                checked: model.value
                                onCheckedChanged: model.value = checked
                            }
                        }

                        Component {
                            id: intEditor
                            TextField {
                                text: model.value
                                validator: IntValidator {
                                    locale: "C"
                                }
                                onEditingFinished:{
                                    if (acceptableInput)
                                        model.value = parseInt(text)
                                }
                            }
                        }

                        Component {
                            id: floatEditor
                            ColumnLayout {
                                TextField {
                                    Layout.fillWidth: true
                                    text: model.value
                                    validator: DoubleValidator {
                                        locale: "C"
                                    }
                                    onEditingFinished:{
                                        if (acceptableInput) {
                                            var floatValue = parseFloat(text);
                                            floatSlider.updateMinMax(floatValue);
                                            model.value = floatValue;
                                        }
                                    }
                                }
                                Slider {
                                    id: floatSlider
                                    // Grow slider min & max based on given values
                                    function updateMinMax(newValue) {
                                        if (from > newValue)
                                            from = newValue;
                                        if (to < newValue)
                                            to = newValue;
                                        value = newValue;
                                    }
                                    from: 0.0
                                    to: 1.0
                                    onValueChanged: {
                                        model.value = value;
                                    }
                                    Component.onCompleted: {
                                        updateMinMax(model.value);
                                    }
                                }
                            }
                        }

                        Component {
                            id: vec2Editor
                            ColumnLayout {
                                RowLayout {
                                    Label {
                                        text: "X:"
                                    }
                                    TextField {
                                        id: xField
                                        text: model.value.x
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished: {
                                            if (acceptableInput)
                                                model.value = Qt.vector2d(parseFloat(text), model.value.y)
                                        }
                                    }
                                }
                                RowLayout {
                                    Label {
                                        text: "Y:"
                                    }
                                    TextField {
                                        id: yField
                                        text: model.value.y
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished: {
                                            if (acceptableInput)
                                                model.value = Qt.vector2d(model.value.x, parseFloat(text))
                                        }
                                    }
                                }
                            }
                        }

                        Component {
                            id: vec3Editor
                            ColumnLayout {
                                RowLayout {
                                    Label {
                                        text: "X:"
                                    }
                                    TextField {
                                        id: xField
                                        text: model.value.x
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.vector3d(parseFloat(text), model.value.y, model.value.z)
                                        }
                                    }
                                }
                                RowLayout {
                                    Label {
                                        text: "Y:"
                                    }
                                    TextField {
                                        id: yField
                                        text: model.value.y
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.vector3d(model.value.x, parseFloat(text), model.value.z)
                                        }
                                    }
                                }
                                RowLayout {
                                    Label {
                                        text: "Z:"
                                    }
                                    TextField {
                                        id: zField
                                        text: model.value.z
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.vector3d(model.value.x, model.value.y, parseFloat(text))
                                        }
                                    }
                                }
                            }
                        }

                        Component {
                            id: vec4Editor
                            ColumnLayout {
                                RowLayout {
                                    Label {
                                        text: "X:"
                                    }
                                    TextField {
                                        id: xField
                                        text: model.value.x
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.vector4d(parseFloat(text), model.value.y, model.value.z, model.value.w)
                                        }
                                    }
                                }
                                RowLayout {
                                    Label {
                                        text: "Y:"
                                    }
                                    TextField {
                                        id: yField
                                        text: model.value.y
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.vector4d(model.value.x, parseFloat(text), model.value.z, model.value.w)
                                        }
                                    }
                                }
                                RowLayout {
                                    Label {
                                        text: "Z:"
                                    }
                                    TextField {
                                        id: zField
                                        text: model.value.z
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.vector4d(model.value.x, model.value.y, parseFloat(text), model.value.w)
                                        }
                                    }
                                }
                                RowLayout {
                                    Label {
                                        text: "W:"
                                    }
                                    TextField {
                                        id: wField
                                        text: model.value.w
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.vector4d(model.value.x, model.value.y, model.value.z, parseFloat(text))
                                        }
                                    }
                                }
                            }
                        }

                        Component {
                            id: mat44Editor
                            ColumnLayout {
                                RowLayout {
                                    TextField {
                                        text: model.value.m11
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.matrix4x4(parseFloat(text), model.value.m12, model.value.m13 , model.value.m14,
                                                                           model.value.m21, model.value.m22, model.value.m23, model.value.m24,
                                                                           model.value.m31, model.value.m32, model.value.m33, model.value.m34,
                                                                           model.value.m41, model.value.m42, model.value.m43, model.value.m44)
                                        }
                                    }
                                    TextField {
                                        text: model.value.m12
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.matrix4x4(model.value.m11, parseFloat(text), model.value.m13 , model.value.m14,
                                                                           model.value.m21, model.value.m22, model.value.m23, model.value.m24,
                                                                           model.value.m31, model.value.m32, model.value.m33, model.value.m34,
                                                                           model.value.m41, model.value.m42, model.value.m43, model.value.m44)
                                        }
                                    }
                                    TextField {
                                        text: model.value.m13
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.matrix4x4(model.value.m11, model.value.m12, parseFloat(text), model.value.m14,
                                                                           model.value.m21, model.value.m22, model.value.m23, model.value.m24,
                                                                           model.value.m31, model.value.m32, model.value.m33, model.value.m34,
                                                                           model.value.m41, model.value.m42, model.value.m43, model.value.m44)
                                        }
                                    }
                                    TextField {
                                        text: model.value.m14
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.matrix4x4(model.value.m11, model.value.m12, model.value.m13, parseFloat(text),
                                                                           model.value.m21, model.value.m22, model.value.m23, model.value.m24,
                                                                           model.value.m31, model.value.m32, model.value.m33, model.value.m34,
                                                                           model.value.m41, model.value.m42, model.value.m43, model.value.m44)
                                        }
                                    }
                                }
                                RowLayout {
                                    TextField {
                                        text: model.value.m21
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.matrix4x4(model.value.m11, model.value.m12, model.value.m13, model.value.m14,
                                                                           parseFloat(text), model.value.m22, model.value.m23, model.value.m24,
                                                                           model.value.m31, model.value.m32, model.value.m33, model.value.m34,
                                                                           model.value.m41, model.value.m42, model.value.m43, model.value.m44)
                                        }
                                    }
                                    TextField {
                                        text: model.value.m22
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.matrix4x4(model.value.m11, model.value.m12, model.value.m13, model.value.m14,
                                                                           model.value.m21, parseFloat(text), model.value.m23, model.value.m24,
                                                                           model.value.m31, model.value.m32, model.value.m33, model.value.m34,
                                                                           model.value.m41, model.value.m42, model.value.m43, model.value.m44)
                                        }
                                    }
                                    TextField {
                                        text: model.value.m23
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.matrix4x4(model.value.m11, model.value.m12, model.value.m13, model.value.m14,
                                                                           model.value.m21, model.value.m22, parseFloat(text), model.value.m24,
                                                                           model.value.m31, model.value.m32, model.value.m33, model.value.m34,
                                                                           model.value.m41, model.value.m42, model.value.m43, model.value.m44)
                                        }
                                    }
                                    TextField {
                                        text: model.value.m24
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.matrix4x4(model.value.m11, model.value.m12, model.value.m13, model.value.m14,
                                                                           model.value.m21, model.value.m22, model.value.m23, parseFloat(text),
                                                                           model.value.m31, model.value.m32, model.value.m33, model.value.m34,
                                                                           model.value.m41, model.value.m42, model.value.m43, model.value.m44)
                                        }
                                    }
                                }
                                RowLayout {
                                    TextField {
                                        text: model.value.m31
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.matrix4x4(model.value.m11, model.value.m12, model.value.m13, model.value.m14,
                                                                           model.value.m21, model.value.m22, model.value.m23, model.value.m24,
                                                                           parseFloat(text), model.value.m32, model.value.m33, model.value.m34,
                                                                           model.value.m41, model.value.m42, model.value.m43, model.value.m44)
                                        }
                                    }
                                    TextField {
                                        text: model.value.m32
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.matrix4x4(model.value.m11, model.value.m12, model.value.m13, model.value.m14,
                                                                           model.value.m21, model.value.m22, model.value.m23, model.value.m24,
                                                                           model.value.m31, parseFloat(text), model.value.m33, model.value.m34,
                                                                           model.value.m41, model.value.m42, model.value.m43, model.value.m44)
                                        }
                                    }
                                    TextField {
                                        text: model.value.m33
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.matrix4x4(model.value.m11, model.value.m12, model.value.m13, model.value.m14,
                                                                           model.value.m21, model.value.m22, model.value.m23, model.value.m24,
                                                                           model.value.m31, model.value.m32, parseFloat(text), model.value.m34,
                                                                           model.value.m41, model.value.m42, model.value.m43, model.value.m44)
                                        }
                                    }
                                    TextField {
                                        text: model.value.m34
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.matrix4x4(model.value.m11, model.value.m12, model.value.m13, model.value.m14,
                                                                           model.value.m21, model.value.m22, model.value.m23, model.value.m24,
                                                                           model.value.m31, model.value.m32, model.value.m33, parseFloat(text),
                                                                           model.value.m41, model.value.m42, model.value.m43, model.value.m44)
                                        }
                                    }
                                }
                                RowLayout {
                                    TextField {
                                        text: model.value.m41
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.matrix4x4(model.value.m11, model.value.m12, model.value.m13, model.value.m14,
                                                                           model.value.m21, model.value.m22, model.value.m23, model.value.m24,
                                                                           model.value.m31, model.value.m32, model.value.m33, model.value.m34,
                                                                           parseFloat(text), model.value.m42, model.value.m43, model.value.m44)
                                        }
                                    }
                                    TextField {
                                        text: model.value.m42
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.matrix4x4(model.value.m11, model.value.m12, model.value.m13, model.value.m14,
                                                                           model.value.m21, model.value.m22, model.value.m23, model.value.m24,
                                                                           model.value.m31, model.value.m32, model.value.m33, model.value.m34,
                                                                           model.value.m41, parseFloat(text), model.value.m43, model.value.m44)
                                        }
                                    }
                                    TextField {
                                        text: model.value.m43
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.matrix4x4(model.value.m11, model.value.m12, model.value.m13, model.value.m14,
                                                                           model.value.m21, model.value.m22, model.value.m23, model.value.m24,
                                                                           model.value.m31, model.value.m32, model.value.m33, model.value.m34,
                                                                           model.value.m41, model.value.m42, parseFloat(text), model.value.m44)
                                        }
                                    }
                                    TextField {
                                        text: model.value.m44
                                        validator: DoubleValidator {
                                            locale: "C"
                                        }
                                        onEditingFinished:{
                                            if (acceptableInput)
                                                model.value = Qt.matrix4x4(model.value.m11, model.value.m12, model.value.m13, model.value.m14,
                                                                           model.value.m21, model.value.m22, model.value.m23, model.value.m24,
                                                                           model.value.m31, model.value.m32, model.value.m33, model.value.m34,
                                                                           model.value.m41, model.value.m42, model.value.m43, parseFloat(text))
                                        }
                                    }
                                }
                            }
                        }

                        Component {
                            id: samplerEditor
                            ColumnLayout {
                                Image {
                                    id: previewImage
                                    sourceSize.width: 128
                                    sourceSize.height: 128
                                    fillMode: Image.PreserveAspectFit
                                }
                                Button {
                                    text: "Choose Image"
                                    onClicked: {
                                        textureSourceDialog.open()
                                    }
                                }
                                FileDialog {
                                    id: textureSourceDialog
                                    title: "Open an Image File"
                                    nameFilters: [ materialAdapter.getSupportedImageFormatsFilter()]
                                    onAccepted: {
                                        if (textureSourceDialog.selectedFile !== null) {
                                            model.value = textureSourceDialog.selectedFile
                                            previewImage.source = textureSourceDialog.selectedFile
                                        }
                                    }
                                }
                            }
                        }
                    }

                }
            }
        }
    }
}
