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

Section {
    caption: qsTr("Model")

    SectionLayout {
        id: tessellationSection

        Label {
            text: qsTr("Source")
            tooltip: qsTr("Defines the location of the mesh file containing the geometry of this model.")
        }
        SecondColumnLayout {
            UrlChooser {
                backendValue: backendValues.source
                filter: "*.mesh"
            }
        }

        function hasTessellationMode(mode) {
            if (tessellationModeComboBox.backendValue.valueToString !== "" &&
                tessellationModeComboBox.backendValue.valueToString !== mode)
                return false

            if (tessellationModeComboBox.backendValue.enumeration !== "" &&
                tessellationModeComboBox.backendValue.enumeration !== mode)
                return false

            return true
        }

        Label {
            text: qsTr("Tessellation Mode")
            tooltip: qsTr("Defines what method to use to dynamically generate additional geometry for the model.")
        }
        SecondColumnLayout {
            ComboBox {
                id: tessellationModeComboBox
                scope: "Model"
                model: ["NoTessellation", "Linear", "Phong", "NPatch"]
                backendValue: backendValues.tessellationMode
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Edge Tessellation")
            tooltip: qsTr("Defines the edge multiplier to the tessellation generator.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 64.0
                minimumValue: 0.0
                decimals: 0
                backendValue: backendValues.edgeTessellation
                Layout.fillWidth: true
                enabled: !tessellationSection.hasTessellationMode("NoTessellation")
            }
        }
        Label {
            text: qsTr("Inner Tessellation")
            tooltip: qsTr("Defines the inner multiplier to the tessellation generator.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 64.0
                minimumValue: 0.0
                decimals: 0
                backendValue: backendValues.innerTessellation
                Layout.fillWidth: true
                enabled: !tessellationSection.hasTessellationMode("NoTessellation")
            }
        }

        Label {
            text: qsTr("Enable Wireframe Mode")
            tooltip: qsTr("Enables the wireframe mode if tesselation is enabled.")
        }
        SecondColumnLayout {
            CheckBox {
                text: backendValues.isWireframeMode.valueToString
                backendValue: backendValues.isWireframeMode
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Casts Shadows")
            tooltip: qsTr("Enables the geometry of this model to be rendered to the shadow maps.")
        }
        SecondColumnLayout {
            CheckBox {
                text: backendValues.castsShadows.valueToString
                backendValue: backendValues.castsShadows
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Receives Shadows")
            tooltip: qsTr("Enables the geometry of this model to receive shadows.")
        }
        SecondColumnLayout {
            CheckBox {
                text: backendValues.receivesShadows.valueToString
                backendValue: backendValues.receivesShadows
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Pickable")
            tooltip: qsTr("Controls whether the model is pickable or not.")
        }
        SecondColumnLayout {
            CheckBox {
                text: backendValues.pickable.valueToString
                backendValue: backendValues.pickable
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Materials")
        }
        SecondColumnLayout {
            EditableListView {
                backendValue: backendValues.materials
                model: backendValues.materials.expressionAsList
                Layout.fillWidth: true

                onAdd: function(value) { backendValues.materials.idListAdd(value) }
                onRemove: function(idx) { backendValues.materials.idListRemove(idx) }
                onReplace: function (idx, value) { backendValues.materials.idListReplace(idx, value) }
            }
        }
    }
}
