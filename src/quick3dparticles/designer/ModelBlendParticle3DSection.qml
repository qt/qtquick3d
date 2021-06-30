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
    caption: qsTr("Particle Model Blend")
    width: parent.width
    SectionLayout {

        Label {
            text: qsTr("Delegate")
            tooltip: qsTr("The delegate provides a template defining the model for the ModelBlendParticle3D.")
        }
        SecondColumnLayout {
            IdComboBox {
                typeFilter: "Component"
                Layout.fillWidth: true
                backendValue: backendValues.delegate
            }
        }

        Label {
            text: qsTr("End Node")
            tooltip: qsTr("This property holds the node that specifies the transformation for the model at the end of particle effect.")
        }
        SecondColumnLayout {
            IdComboBox {
                typeFilter: "QtQuick3D.Node"
                Layout.fillWidth: true
                backendValue: backendValues.endNode
            }
        }

        Label {
            text: qsTr("Model Blend Mode")
            tooltip: qsTr("This property holds blending mode for the particle effect.")
        }
        SecondColumnLayout {
            ComboBox {
                scope: "ModelBlendParticle3D"
                model: ["Explode", "Construct", "Transfer"]
                backendValue: backendValues.modelBlendMode
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("End Time")
            tooltip: qsTr("This property holds the end time of the particle in milliseconds.")
        }
        SecondColumnLayout {
            SpinBox {
                maximumValue: 999999
                minimumValue: 0
                decimals: 0
                backendValue: backendValues.endTime
                Layout.fillWidth: true
            }
        }

        Label {
            text: qsTr("Activation Node")
            tooltip: qsTr("This property holds a node that activates particles and overrides the reqular emit routine.")
        }
        SecondColumnLayout {
            IdComboBox {
                typeFilter: "QtQuick3D.Node"
                Layout.fillWidth: true
                backendValue: backendValues.activationNode
            }
        }

        Label {
            text: qsTr("Random")
            tooltip: qsTr("This property holds whether the particles are emitted in random order instead of in the order they are specified in the model.")
        }
        SecondColumnLayout {
            CheckBox {
                id: randomCheckBox
                text: backendValues.random.valueToString
                backendValue: backendValues.random
                Layout.fillWidth: true
            }
        }
    }

}
