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
    caption: qsTr("Material")

    SectionLayout {

        // Baked Lighting properties (may be internal eventually)
        // ### lightmapIndirect
        // ### lightmapRadiosity
        // ### lightmapShadow

        // ### iblProbe override

        PropertyLabel {
            text: qsTr("Light Probe")
            tooltip: qsTr("Defines a texture for overriding or setting an image based lighting texture for use with this material.")
        }

        SecondColumnLayout {
            IdComboBox {
                typeFilter: "QtQuick3D.Texture"
                backendValue: backendValues.lightProbe
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Culling Mode")
            tooltip: qsTr("Defines whether culling is enabled and which mode is actually enabled.")
        }

        SecondColumnLayout {
            ComboBox {
                scope: "Material"
                model: ["BackFaceCulling", "FrontFaceCulling", "NoCulling"]
                backendValue: backendValues.cullMode
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Depth Draw Mode")
            tooltip: qsTr("This property determines if and when depth rendering takes place for this material.")
        }

        SecondColumnLayout {
            ComboBox {
                scope: "Material"
                model: ["OpaqueOnlyDepthDraw", "AlwaysDepthDraw", "NeverDepthDraw", "OpaquePrePassDepthDraw"]
                backendValue: backendValues.depthDrawMode
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
