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
    caption: qsTr("Joint")
    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: qsTr("Index")
            tooltip: qsTr("Specifies the index of this joint.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: -9999999
                maximumValue: 9999999
                decimals: 0
                backendValue: backendValues.index
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Skeleton Root")
            tooltip: qsTr("Specifies the skeleton that contains this joint.")
        }

        SecondColumnLayout {
            IdComboBox {
                typeFilter: "QtQuick3D.Skeleton"
                backendValue: backendValues.skeletonRoot
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
