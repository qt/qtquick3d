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

Section {
    caption: qsTr("Particle Attractor")
    width: parent.width
    SectionLayout {
        Label {
            text: qsTr("Position Variation")
            tooltip: qsTr("This property defines the variation on attract position.")
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
                    backendValue: backendValues.positionVariation_x
                    Layout.fillWidth: true
                    Layout.minimumWidth: 120
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
                    backendValue: backendValues.positionVariation_y
                    Layout.fillWidth: true
                    Layout.minimumWidth: 120
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
                    backendValue: backendValues.positionVariation_z
                    Layout.fillWidth: true
                    Layout.minimumWidth: 120
                }
            }
        }

        Label {
            text: qsTr("Shape")
            tooltip: qsTr("This property defines a ParticleAbstractShape3D for particles attraction.")
        }
        SecondColumnLayout {
            IdComboBox {
                typeFilter: "QQuick3DParticleAbstractShape"
                Layout.fillWidth: true
                backendValue: backendValues.shape
            }
        }

        Label {
            text: qsTr("Duration")
            tooltip: qsTr("This property defines the duration in milliseconds how long it takes for particles to reach the attaction position.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 999999
                minimumValue: -1
                decimals: 0
                backendValue: backendValues.duration
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Duration Variation")
            tooltip: qsTr("This property defines the duration variation in milliseconds.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 999999
                minimumValue: 0
                decimals: 0
                backendValue: backendValues.durationVariation
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Hide At End")
            tooltip: qsTr("This property defines if the particle should disappear when it reaches the attractor.")
        }
        SecondColumnLayout {
            CheckBox {
                text: backendValues.hideAtEnd.valueToString
                backendValue: backendValues.hideAtEnd
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Use Cached Positions")
            tooltip: qsTr("This property defines if the attractor caches possible positions within its shape. Cached positions give less random results but are better for performance.")
        }
        SecondColumnLayout {
            CheckBox {
                text: backendValues.useCachedPositions.valueToString
                backendValue: backendValues.useCachedPositions
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Positions Amount")
            tooltip: qsTr("This property defines the amount of possible positions stored within the attractor shape.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 999999
                minimumValue: 0
                decimals: 0
                backendValue: backendValues.positionsAmount
                Layout.fillWidth: true
            }
        }
    }
}
