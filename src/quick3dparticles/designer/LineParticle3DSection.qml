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
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    caption: qsTr("Line Particle")
    width: parent.width

    SectionLayout {
        PropertyLabel {
            text: qsTr("Segments")
            tooltip: qsTr("This property defines the segment count of the line.")
        }
        SecondColumnLayout {
            SpinBox {
                minimumValue: 1
                maximumValue: 999999
                decimals: 0
                backendValue: backendValues.segmentCount
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Alpha Fade")
            tooltip: qsTr("This property defines the line fade amount per segment.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0.0
                maximumValue: 1.0
                decimals: 2
                backendValue: backendValues.alphaFade
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Scale Multiplier")
            tooltip: qsTr("This property defines the scale multiplier per segment.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0.0
                maximumValue: 2.0
                decimals: 2
                backendValue: backendValues.scaleMultiplier
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Texcoord Multiplier")
            tooltip: qsTr("This property defines the texture coordinate multiplier of the line.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: -99999.0
                maximumValue: 99999.0
                decimals: 2
                backendValue: backendValues.texcoordMultiplier
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Texcoord Mode")
            tooltip: qsTr("This property defines the texture coordinate mode of the line.")
        }

        SecondColumnLayout {
            ComboBox {
                scope: "LineParticle3D"
                model: ["Absolute", "Relative", "Fill"]
                backendValue: backendValues.texcoordMode
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Line Length")
            tooltip: qsTr("This property defines the length of the line.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0.0
                maximumValue: 99999.0
                decimals: 2
                backendValue: backendValues.length
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Line Length Variation")
            tooltip: qsTr("This property defines the length variation of the line.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0.0
                maximumValue: 99999.0
                decimals: 2
                backendValue: backendValues.lengthVariation
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Minimum Segment Length")
            tooltip: qsTr("This property defines the minimum length between line segments.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0.0
                maximumValue: 99999.0
                decimals: 2
                backendValue: backendValues.lengthDeltaMin
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("EOL Fade Out Duration")
            tooltip: qsTr("This property defines the fade out duration after the end of particle lifetime.")
        }

        SecondColumnLayout {
            SpinBox {
                minimumValue: 0.0
                maximumValue: 99999.0
                decimals: 2
                backendValue: backendValues.eolFadeOutDuration
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
            }

            ExpandingSpacer {}
        }
    }
}
