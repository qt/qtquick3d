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
import QtQuick3D
import MaterialEditorHelpers 1.0
import Qt.labs.qmlmodels
import Qt.labs.settings

SplitView {
    id: editorView
    orientation: Qt.Vertical
    property alias vertexEditor: vertEdit
    property alias fragmentEditor: fragEdit
    property QtObject uniformTable2
    property alias outputTextItem: outputTextItem
    property alias outputView: outputView
    property alias vertexTabText: vertexTabText.text
    property alias fragmentTabText: fragTabtext.text
    property alias infoStack: infoStack
    property alias tabBarInfoView: tabBarInfoView
    property alias tabButtonShaderOutput: tabButtonShaderOutput
    property alias uniformModel: uniformManagerPane.uniformModel
    required property MaterialAdapter materialAdapter

    ColumnLayout {
        SplitView.preferredHeight: parent.height * .8
        TabBar {
            id: tabBarEditors
            Layout.fillWidth: true
            readonly property string defVertText: qsTr("Vertex")
            readonly property string defFragText: qsTr("Fragment")
            TabButton {
                id: vertexTabText
                onTextChanged: {
                    if (text === "")
                        text = tabBarEditors.defVertText
                }
            }
            TabButton {
                id: fragTabtext
                onTextChanged: {
                    if (text === "")
                        text = tabBarEditors.defFragText
                }
            }
            TabButton {
                id: propertiesTabText
                text: qsTr("Material Properties")
            }
        }

        // Editors
        StackLayout {
            id: editorStack
            currentIndex: tabBarEditors.currentIndex
            Layout.fillWidth: true

            ShaderEditor {
                id: vertEdit
                Layout.fillHeight: true
                Layout.fillWidth: true
            }
            ShaderEditor {
                id: fragEdit
                Layout.fillHeight: true
                Layout.fillWidth: true
            }

            MaterialPropertiesPane {
                id: propertiesPane
                targetMaterial: editorView.materialAdapter
                Layout.fillHeight: true
                Layout.fillWidth: true
            }
        }
    }

    ColumnLayout {
        TabBar {
            id: tabBarInfoView
            Layout.fillWidth: true
            TabButton {
                id: tabButtonUniforms
                text: qsTr("Uniforms")
            }
            TabButton {
                id: tabButtonShaderOutput
                text: qsTr("Shader Output")
            }
        }

        // Uniform, compile output etc.
        StackLayout {
            id: infoStack
            currentIndex: tabBarInfoView.currentIndex
//            Layout.preferredHeight: parent.height * .2
            Layout.fillWidth: true
            UniformManagerPane {
                id: uniformManagerPane
                materialAdapter: editorView.materialAdapter
                Layout.fillHeight: true
                Layout.fillWidth: true
            }
            Rectangle {
                id: outputView
                Layout.fillHeight: true
                Layout.fillWidth: true
                color: palette.base
                TextEdit {
                    id: outputTextItem
                    anchors.fill: parent
                    padding: 2
                    color: palette.text
                    wrapMode: Text.WordWrap
                    readOnly: true
                }
            }
        }
    }
}
