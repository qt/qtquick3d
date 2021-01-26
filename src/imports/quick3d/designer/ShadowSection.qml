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
    caption: qsTr("Shadows")
    width: parent.width

    SectionLayout {

        Label {
            text: qsTr("Casts Shadow")
            tooltip: qsTr("Enables shadow casting for this light.")
        }
        SecondColumnLayout {
            CheckBox {
                id: shadowCheckBox
                text: backendValues.castsShadow.valueToString
                backendValue: backendValues.castsShadow
                Layout.fillWidth: true
            }
        }

        // ### all the following should only be shown when shadows are enabled
        Label {
            text: qsTr("Shadow Factor")
            tooltip: qsTr("Determines how dark the cast shadows should be.")
        }
        SecondColumnLayout {
            SpinBox {
                minimumValue: 0.0
                maximumValue: 100.0
                decimals: 0
                backendValue: backendValues.shadowFactor
                Layout.fillWidth: true
                enabled: shadowCheckBox.backendValue.value === true
            }
        }

        Label {
            text: qsTr("Shadow Filter")
            tooltip: qsTr("Sets how much blur is applied to the shadows.")
        }
        SecondColumnLayout {
            SpinBox {
                minimumValue: 1.0
                maximumValue: 100.0
                decimals: 0
                backendValue: backendValues.shadowFilter
                Layout.fillWidth: true
                enabled: shadowCheckBox.backendValue.value === true
            }
        }

        Label {
            text: qsTr("Shadow Map Quality")
            tooltip: qsTr("Sets the quality of the shadow map created for shadow rendering.")
        }
        SecondColumnLayout {
            ComboBox {
                scope: "Light"
                model: ["ShadowMapQualityLow", "ShadowMapQualityMedium", "ShadowMapQualityHigh", "ShadowMapQualityVeryHigh"]
                backendValue: backendValues.shadowMapQuality
                Layout.fillWidth: true
                enabled: shadowCheckBox.backendValue.value === true
            }
        }

        Label {
            text: qsTr("Shadow Bias")
            tooltip: qsTr("Sets a slight offset to avoid self-shadowing artifacts.")
        }
        SecondColumnLayout {
            SpinBox {
                minimumValue: -1.0
                maximumValue: 1.0
                decimals: 2
                stepSize: 0.1
                backendValue: backendValues.shadowBias
                Layout.fillWidth: true
                enabled: shadowCheckBox.backendValue.value === true
            }
        }

        Label {
            text: qsTr("Shadow Map Far")
            tooltip: qsTr("Determines the maximum distance for the shadow map.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 9999999
                minimumValue: -9999999
                realDragRange: 5000
                decimals: 0
                stepSize: 100
                backendValue: backendValues.shadowMapFar
                Layout.fillWidth: true
                enabled: shadowCheckBox.backendValue.value === true
            }
        }

    }
}

