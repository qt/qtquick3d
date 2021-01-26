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
        caption: qsTr("Top Color")
        width: parent.width
        ColorEditor {
            caption: qsTr("Top Color")
            backendValue: backendValues.topColor
            supportGradient: false
            isVector3D: true
            Layout.fillWidth: true
        }
    }

    Section {
        caption: qsTr("Bottom Color")
        width: parent.width
        ColorEditor {
            caption: qsTr("Bottom Color")
            backendValue: backendValues.bottomColor
            supportGradient: false
            isVector3D: true
            Layout.fillWidth: true
        }
    }
}
