/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.12
import StudioTheme 1.0 as StudioTheme

Column {
    width: parent.width
    Section {
        id: entrySection
        width: parent.width
        caption: qsTr("Instance List Entry")

        property int labelWidth: 10
        property int labelSpinBoxSpacing: 0
        property int spinBoxMinimumWidth: 120

        ColumnLayout {
            width: parent.width - 16
            Label {
                text: qsTr("Color")
                tooltip: qsTr("This property specifies the color for the instance.")
            }
            SecondColumnLayout {
                ColorEditor {
                    caption: qsTr("Color")
                    backendValue: backendValues.color
                    supportGradient: false
                    Layout.fillWidth: true
                }
            }

            GridLayout {
                columns: 2
                rows: 2
                columnSpacing: 24
                rowSpacing: 12
                width: parent.width - 16

                ColumnLayout {
                    Label {
                        width: 100
                        text: qsTr("Position")
                        tooltip: qsTr("This property specifies the position for the instance.")
                    }
                    RowLayout {
                        spacing: entrySection.labelSpinBoxSpacing

                        Label {
                            text: qsTr("X")
                            width: entrySection.labelWidth
                            color: StudioTheme.Values.theme3DAxisXColor
                        }
                        SpinBox {
                            maximumValue: 9999999
                            minimumValue: -9999999
                            realDragRange: 5000
                            decimals: 2
                            backendValue: backendValues.position_x
                            Layout.fillWidth: true
                            Layout.minimumWidth: entrySection.spinBoxMinimumWidth
                        }
                    }

                    RowLayout {
                        spacing: entrySection.labelSpinBoxSpacing
                        Label {
                            text: qsTr("Y")
                            width: entrySection.labelWidth
                            color: StudioTheme.Values.theme3DAxisYColor
                        }
                        SpinBox {
                            maximumValue: 9999999
                            minimumValue: -9999999
                            realDragRange: 5000
                            decimals: 2
                            backendValue: backendValues.position_y
                            Layout.fillWidth: true
                            Layout.minimumWidth: entrySection.spinBoxMinimumWidth
                        }
                    }
                    RowLayout {
                        spacing: entrySection.labelSpinBoxSpacing
                        Label {
                            text: qsTr("Z")
                            width: entrySection.labelWidth
                            color: StudioTheme.Values.theme3DAxisZColor
                        }
                        SpinBox {
                            maximumValue: 9999999
                            minimumValue: -9999999
                            realDragRange: 5000
                            decimals: 2
                            backendValue: backendValues.position_z
                            Layout.fillWidth: true
                            Layout.minimumWidth: entrySection.spinBoxMinimumWidth
                        }
                    }
                }

                ColumnLayout {
                    Label {
                        width: 100
                        text: qsTr("Scale")
                        tooltip: qsTr("This property specifies the scale for the instance.")
                    }
                    RowLayout {
                        spacing: entrySection.labelSpinBoxSpacing
                        Label {
                            text: qsTr("X")
                            width: entrySection.labelWidth
                            color: StudioTheme.Values.theme3DAxisXColor
                        }
                        SpinBox {
                            maximumValue: 9999999
                            minimumValue: -9999999
                            realDragRange: 5000
                            decimals: 2
                            backendValue: backendValues.scale_x
                            Layout.fillWidth: true
                            Layout.minimumWidth: entrySection.spinBoxMinimumWidth
                        }
                    }
                    RowLayout {
                        spacing: entrySection.labelSpinBoxSpacing
                        Label {
                            text: qsTr("Y")
                            width: entrySection.labelWidth
                            color: StudioTheme.Values.theme3DAxisYColor
                        }
                        SpinBox {
                            maximumValue: 9999999
                            minimumValue: -9999999
                            realDragRange: 5000
                            decimals: 2
                            backendValue: backendValues.scale_y
                            Layout.fillWidth: true
                            Layout.minimumWidth: entrySection.spinBoxMinimumWidth
                        }
                    }
                    RowLayout {
                        spacing: entrySection.labelSpinBoxSpacing
                        Label {
                            text: qsTr("Z")
                            width: entrySection.labelWidth
                            color: StudioTheme.Values.theme3DAxisZColor
                        }
                        SpinBox {
                            maximumValue: 9999999
                            minimumValue: -9999999
                            realDragRange: 5000
                            decimals: 2
                            backendValue: backendValues.scale_z
                            Layout.fillWidth: true
                            Layout.minimumWidth: entrySection.spinBoxMinimumWidth
                        }
                    }
                }
                ColumnLayout {
                    Label {
                        width: 150
                        text: qsTr("Rotation")
                        tooltip: qsTr("This property specifies the rotation for the instance.")
                    }
                    RowLayout {
                        spacing: entrySection.labelSpinBoxSpacing
                        Label {
                            text: qsTr("X")
                            width: entrySection.labelWidth
                            color: StudioTheme.Values.theme3DAxisXColor
                        }
                        SpinBox {
                            maximumValue: 9999999
                            minimumValue: -9999999
                            realDragRange: 5000
                            decimals: 2
                            backendValue: backendValues.eulerRotation_x
                            Layout.fillWidth: true
                            Layout.minimumWidth: entrySection.spinBoxMinimumWidth
                        }
                    }
                    RowLayout {
                        spacing: entrySection.labelSpinBoxSpacing
                        Label {
                            text: qsTr("Y")
                            width: entrySection.labelWidth
                            color: StudioTheme.Values.theme3DAxisYColor
                        }
                        SpinBox {
                            maximumValue: 9999999
                            minimumValue: -9999999
                            realDragRange: 5000
                            decimals: 2
                            backendValue: backendValues.eulerRotation_y
                            Layout.fillWidth: true
                            Layout.minimumWidth: entrySection.spinBoxMinimumWidth
                        }
                    }
                    RowLayout {
                        spacing: entrySection.labelSpinBoxSpacing
                        Label {
                            text: qsTr("Z")
                            width: entrySection.labelWidth
                            color: StudioTheme.Values.theme3DAxisZColor
                        }
                        SpinBox {
                            maximumValue: 9999999
                            minimumValue: -9999999
                            realDragRange: 5000
                            decimals: 2
                            backendValue: backendValues.eulerRotation_z
                            Layout.fillWidth: true
                            Layout.minimumWidth: entrySection.spinBoxMinimumWidth
                        }
                    }
                }
            }
        }
    }
}

