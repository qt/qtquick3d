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
    caption: qsTr("Loader3D")
    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: qsTr("Active")
            tooltip: qsTr("This property is true if the Loader3D is currently active.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.active.valueToString
                backendValue: backendValues.active
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Source")
            tooltip: qsTr("This property holds the URL of the QML component to instantiate.")
        }

        SecondColumnLayout {
            UrlChooser {
                filter: "*.qml"
                backendValue:  backendValues.source
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Source Component")
            tooltip: qsTr("This property holds the component to instantiate.")
        }

        SecondColumnLayout {
            IdComboBox {
                typeFilter: "Component"
                backendValue: backendValues.sourceComponent
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Asynchronous")
            tooltip: qsTr("This property holds whether the component will be instantiated asynchronously.")
        }

        SecondColumnLayout {
            CheckBox {
                text: backendValues.asynchronous.valueToString
                backendValue: backendValues.asynchronous
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
