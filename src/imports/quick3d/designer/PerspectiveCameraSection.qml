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
    caption: qsTr("Perspective Camera")

    SectionLayout {
        Label {
            text: qsTr("Clip Near")
            tooltip: qsTr("Sets the near value of the view frustum of the camera.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                realDragRange: 5000
                decimals: 0
                backendValue: backendValues.clipNear
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Clip Far")
            tooltip: qsTr("Sets the far value of the view frustum of the camera.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                realDragRange: 5000
                decimals: 0
                stepSize: 100
                backendValue: backendValues.clipFar
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Field of View")
            tooltip: qsTr("Sets the field of view of the camera in degrees.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 1
                minimumValue: 180
                decimals: 2
                backendValue: backendValues.fieldOfView
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("FOV Orientation")
            tooltip: qsTr("Determines if the field of view property reflects the vertical or the horizontal field of view.")
        }
        ComboBox {
            scope: "PerspectiveCamera"
            model: ["Vertical", "Horizontal"]
            backendValue: backendValues.fieldOfViewOrientation
            Layout.fillWidth: true
        }
    }
}
