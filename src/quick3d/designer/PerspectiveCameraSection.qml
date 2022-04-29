/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Perspective Camera")

    SectionLayout {
        PropertyLabel {
            text: qsTr("Clip Near")
            tooltip: qsTr("Sets the near value of the view frustum of the camera.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: -9999999
                maximumValue: 9999999
                decimals: 0
                backendValue: backendValues.clipNear
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Clip Far")
            tooltip: qsTr("Sets the far value of the view frustum of the camera.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: -9999999
                maximumValue: 9999999
                decimals: 0
                stepSize: 100
                backendValue: backendValues.clipFar
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Field of View")
            tooltip: qsTr("Sets the field of view of the camera in degrees.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 1
                maximumValue: 180
                decimals: 2
                backendValue: backendValues.fieldOfView
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("FOV Orientation")
            tooltip: qsTr("Determines if the field of view property reflects the vertical or the horizontal field of view.")
        }

        SecondColumnLayout {
            ComboBox {
                scope: "PerspectiveCamera"
                model: ["Vertical", "Horizontal"]
                backendValue: backendValues.fieldOfViewOrientation
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Frustum Culling Enabled")
            tooltip: qsTr("When this property is true, objects outside the camera frustum will be culled, meaning they will not be passed to the renderer.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.frustumCullingEnabled.valueToString
                backendValue: backendValues.frustumCullingEnabled
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
