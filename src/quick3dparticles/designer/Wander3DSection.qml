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
    Section {
        caption: qsTr("Particle Wander")
        width: parent.width
        SectionLayout {
            Label {
                text: qsTr("Unique Amount Variation")
                tooltip: qsTr("This property defines variation for uniqueAmount between 0.0 and 1.0.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 1
                    minimumValue: 0
                    decimals: 2
                    stepSize: 0.01
                    backendValue: backendValues.uniqueAmountVariation
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Unique Pace Variation")
                tooltip: qsTr("This property defines the unique pace (frequency) variation for each particle between 0.0 and 1.0.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 1
                    minimumValue: 0
                    decimals: 2
                    stepSize: 0.01
                    backendValue: backendValues.uniquePaceVariation
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Fade In Duration")
                tooltip: qsTr("This property defines the duration in milliseconds for fading in the affector.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 999999
                    minimumValue: 0
                    realDragRange: 5000
                    decimals: 0
                    backendValue: backendValues.fadeInDuration
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Fade Out Duration")
                tooltip: qsTr("This property defines the duration in milliseconds for fading out the affector.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 999999
                    minimumValue: 0
                    realDragRange: 5000
                    decimals: 0
                    backendValue: backendValues.fadeOutDuration
                    Layout.fillWidth: true
                }
            }
        }
    }
    Section {
        width: parent.width
        caption: qsTr("Global")
        SectionLayout {
            GridLayout {
                columns: 2
                rows: 2
                columnSpacing: 24
                rowSpacing: 12
                width: parent.parent.width

                ColumnLayout {
                    Label {
                        text: qsTr("Amount")
                        tooltip: qsTr("This property defines how long distance each particle moves at the ends of curves.")
                    }
                    ColumnLayout {
                        RowLayout {
                            spacing: 0

                            Label {
                                text: qsTr("X")
                                width: 100
                                color: StudioTheme.Values.theme3DAxisXColor
                            }
                            SpinBox {
                                maximumValue: 9999999
                                minimumValue: -9999999
                                realDragRange: 5000
                                decimals: 2
                                backendValue: backendValues.globalAmount_x
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                            }
                        }
                        RowLayout {
                            spacing: 0

                            Label {
                                text: qsTr("Y")
                                width: 100
                                color: StudioTheme.Values.theme3DAxisYColor
                            }
                            SpinBox {
                                maximumValue: 9999999
                                minimumValue: -9999999
                                realDragRange: 5000
                                decimals: 2
                                backendValue: backendValues.globalAmount_y
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                            }
                        }
                        RowLayout {
                            spacing: 0

                            Label {
                                text: qsTr("Z")
                                width: 100
                                color: StudioTheme.Values.theme3DAxisZColor
                            }
                            SpinBox {
                                maximumValue: 9999999
                                minimumValue: -9999999
                                realDragRange: 5000
                                decimals: 2
                                backendValue: backendValues.globalAmount_z
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                            }
                        }
                    }

                    Label {
                        text: qsTr("Pace")
                        tooltip: qsTr("This property defines the pace (frequency) each particle wanders in curves per second.")
                    }
                    ColumnLayout {
                        RowLayout {
                            spacing: 0

                            Label {
                                text: qsTr("X")
                                width: 100
                                color: StudioTheme.Values.theme3DAxisXColor
                            }
                            SpinBox {
                                maximumValue: 9999999
                                minimumValue: -9999999
                                realDragRange: 5000
                                decimals: 2
                                backendValue: backendValues.globalPace_x
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                            }
                        }
                        RowLayout {
                            spacing: 0

                            Label {
                                text: qsTr("Y")
                                width: 100
                                color: StudioTheme.Values.theme3DAxisYColor
                            }
                            SpinBox {
                                maximumValue: 9999999
                                minimumValue: -9999999
                                realDragRange: 5000
                                decimals: 2
                                backendValue: backendValues.globalPace_y
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                            }
                        }
                        RowLayout {
                            spacing: 0

                            Label {
                                text: qsTr("Z")
                                width: 100
                                color: StudioTheme.Values.theme3DAxisZColor
                            }
                            SpinBox {
                                maximumValue: 9999999
                                minimumValue: -9999999
                                realDragRange: 5000
                                decimals: 2
                                backendValue: backendValues.globalPace_z
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                            }
                        }
                    }
                }
                ColumnLayout {
                    Label {
                        text: qsTr("Pace Start")
                        tooltip: qsTr("This property defines the starting point for the pace (frequency).")
                    }
                    ColumnLayout {
                        RowLayout {
                            spacing: 0

                            Label {
                                text: qsTr("X")
                                width: 100
                                color: StudioTheme.Values.theme3DAxisXColor
                            }
                            SpinBox {
                                maximumValue: 9999999
                                minimumValue: -9999999
                                realDragRange: 5000
                                decimals: 2
                                backendValue: backendValues.globalPaceStart_x
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                            }
                        }
                        RowLayout {
                            spacing: 0

                            Label {
                                text: qsTr("Y")
                                width: 100
                                color: StudioTheme.Values.theme3DAxisYColor
                            }
                            SpinBox {
                                maximumValue: 9999999
                                minimumValue: -9999999
                                realDragRange: 5000
                                decimals: 2
                                backendValue: backendValues.globalPaceStart_y
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                            }
                        }
                        RowLayout {
                            spacing: 0

                            Label {
                                text: qsTr("Z")
                                width: 100
                                color: StudioTheme.Values.theme3DAxisZColor
                            }
                            SpinBox {
                                maximumValue: 9999999
                                minimumValue: -9999999
                                realDragRange: 5000
                                decimals: 2
                                backendValue: backendValues.globalPaceStart_z
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                            }
                        }
                    }
                    Label {
                        text: qsTr("Unique Pace")
                        tooltip: qsTr("This property defines the unique pace (frequency) each particle wanders in curves per second.")
                    }
                    ColumnLayout {
                        RowLayout {
                            spacing: 0

                            Label {
                                text: qsTr("X")
                                width: 100
                                color: StudioTheme.Values.theme3DAxisXColor
                            }
                            SpinBox {
                                maximumValue: 9999999
                                minimumValue: -9999999
                                realDragRange: 5000
                                decimals: 2
                                backendValue: backendValues.uniquePace_x
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                            }
                        }
                        RowLayout {
                            spacing: 0

                            Label {
                                text: qsTr("Y")
                                width: 100
                                color: StudioTheme.Values.theme3DAxisYColor
                            }
                            SpinBox {
                                maximumValue: 9999999
                                minimumValue: -9999999
                                realDragRange: 5000
                                decimals: 2
                                backendValue: backendValues.uniquePace_y
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                            }
                        }
                        RowLayout {
                            spacing: 0

                            Label {
                                text: qsTr("Z")
                                width: 100
                                color: StudioTheme.Values.theme3DAxisZColor
                            }
                            SpinBox {
                                maximumValue: 9999999
                                minimumValue: -9999999
                                realDragRange: 5000
                                decimals: 2
                                backendValue: backendValues.uniquePace_z
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                            }
                        }
                    }
                }
            }
        }
    }
}
