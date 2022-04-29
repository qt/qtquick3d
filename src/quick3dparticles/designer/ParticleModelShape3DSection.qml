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
    caption: qsTr("Particle Model Shape")
    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: qsTr("Fill")
            tooltip: qsTr("This property defines if the shape should be filled or just use the shape outlines.")
        }

        SecondColumnLayout {
            CheckBox {
                id: fillCheckBox
                text: backendValues.fill.valueToString
                backendValue: backendValues.fill
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Delegate")
            tooltip: qsTr("The delegate provides a template defining the model for the ParticleModelShape3D.")
        }

        SecondColumnLayout {
            IdComboBox {
                typeFilter: "Component"
                backendValue: backendValues.delegate
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
