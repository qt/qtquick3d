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
    caption: qsTr("Particle Sprite Particle")
    width: parent.width
    SectionLayout {

        Label {
            text: qsTr("Blend Mode")
            tooltip: qsTr("This property defines the blending mode used for rendering the particles.")
        }
        SecondColumnLayout {
            ComboBox {
                scope: "SpriteParticle3D"
                model: ["SourceOver", "Screen", "Multiply"]
                backendValue: backendValues.blendMode
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Sprite")
            tooltip: qsTr("This property defines the Texture used for the particles.")
        }
        SecondColumnLayout {
            IdComboBox {
                typeFilter: "QtQuick3D.Texture"
                Layout.fillWidth: true
                backendValue: backendValues.sprite
            }
        }

        Label {
            text: qsTr("Sprite Sequence")
            tooltip: qsTr("This property defines the sprite sequence properties for the particle.")
        }
        SecondColumnLayout {
            IdComboBox {
                typeFilter: "QtQuick3D.Particles3D.SpriteSequence3D"
                Layout.fillWidth: true
                backendValue: backendValues.spriteSequence
            }
        }

        Label {
            text: qsTr("Billboard")
            tooltip: qsTr("This property defines if the particle texture should always be aligned face towards the screen.")
        }
        SecondColumnLayout {
            CheckBox {
                id: billboardCheckBox
                text: backendValues.billboard.valueToString
                backendValue: backendValues.billboard
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Particle Scale")
            tooltip: qsTr("This property defines the scale multiplier of the particles.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 999999
                minimumValue: -999999
                realDragRange: 5000
                decimals: 2
                backendValue: backendValues.particleScale
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Color Table")
            tooltip: qsTr("This property defines the Texture used for coloring the particles.")
        }
        SecondColumnLayout {
            IdComboBox {
                typeFilter: "QtQuick3D.Texture"
                Layout.fillWidth: true
                backendValue: backendValues.colorTable
            }
        }
    }
}
