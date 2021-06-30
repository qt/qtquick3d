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

Section {
    caption: qsTr("Particle Sprite Sequence")
    width: parent.width
    SectionLayout {

        Label {
            text: qsTr("Frame Count")
            tooltip: qsTr("This property defines the amount of image frames in sprite.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 999999
                minimumValue: 0
                realDragRange: 5000
                decimals: 0
                backendValue: backendValues.frameCount
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Frame Index")
            tooltip: qsTr("This property defines the initial index of the frame.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 999999
                minimumValue: 0
                realDragRange: 5000
                decimals: 0
                backendValue: backendValues.frameIndex
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Interpolate")
            tooltip: qsTr("This property defines if the sprites are interpolated (blended) between frames to make the animation appear smoother.")
        }
        SecondColumnLayout {
            CheckBox {
                id: interpolateCheckBox
                text: backendValues.interpolate.valueToString
                backendValue: backendValues.interpolate
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Duration")
            tooltip: qsTr("This property defines the duration in milliseconds how long it takes for the sprite sequence to animate.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 999999
                minimumValue: -1
                realDragRange: 5000
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
                minimumValue: -999999
                realDragRange: 5000
                decimals: 0
                backendValue: backendValues.durationVariation
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Random Start")
            tooltip: qsTr("This property defines if the animation should start from a random frame between 0 and frameCount - 1.")
        }
        SecondColumnLayout {
            CheckBox {
                id: randomStartCheckBox
                text: backendValues.randomStart.valueToString
                backendValue: backendValues.randomStart
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Animation Direction")
            tooltip: qsTr("This property defines the animation direction of the sequence.")
        }
        SecondColumnLayout {
            ComboBox {
                scope: "SpriteSequence3D"
                model: ["Normal", "Reverse", "Alternate", "AlternateReverse", "SingleFrame"]
                backendValue: backendValues.animationDirection
                Layout.fillWidth: true
            }
        }
    }

}
