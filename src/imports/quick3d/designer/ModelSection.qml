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

import QtQuick 2.12
import HelperWidgets 2.0
import QtQuick.Layouts 1.12

Section {
    caption: qsTr("Model")

    SectionLayout {

        Label {
            text: qsTr("Source")
            tooltip: qsTr("Set the source of the mesh data file")
        }
        SecondColumnLayout {
            UrlChooser {
                backendValue: backendValues.source
                filter: "*.mesh"
            }
        }

        Label {
            text: qsTr("Tesselation Mode")
        }
        SecondColumnLayout {
            ComboBox {
                scope: "Model."
                model: ["NoTess", "TessLinear", "TessPhong", "TessNPatch"]
                backendValue: backendValues.orientation
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Edge Tesselation")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 64.0
                minimumValue: 0.0
                decimals: 0
                backendValue: backendValues.edgeTess
                Layout.fillWidth: true
            }
        }
        Label {
            text: qsTr("Inner Tesselation")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 64.0
                minimumValue: 0.0
                decimals: 0
                backendValue: backendValues.innerTess
                Layout.fillWidth: true
            }
        }

        Label {
            text: "Enable Wireframe Mode"
        }
        SecondColumnLayout {
            CheckBox {
                text: backendValues.isWireframeMode.valueToString
                backendValue: backendValues.isWireframeMode
                Layout.fillWidth: true
            }
        }
    }
}
