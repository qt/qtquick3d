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
        caption: qsTr("Render State")
        width: parent.width

        SectionLayout {
            Label {
                text: qsTr("State")
                tooltip: qsTr("Render state to set for a pass.")
            }
            ComboBox {
                scope: "RenderState"
                model: ["Unknown", "Blend", "CullFace", "DepthTest", "StencilTest", "ScissorTest", "DepthWrite", "Multisample"]
                backendValue: backendValues.renderState
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("Enabled")
                tooltip: qsTr("Render state enable state.")
            }
            SecondColumnLayout {
                CheckBox {
                    text: backendValues.enabled.valueToString
                    backendValue: backendValues.enabled
                    Layout.fillWidth: true
                }
            }
        }
    }
}
