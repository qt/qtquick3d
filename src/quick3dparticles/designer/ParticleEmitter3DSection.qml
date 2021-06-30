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
        caption: qsTr("Particle Emitter")
        width: parent.width
        SectionLayout {

            Label {
                text: qsTr("System")
                tooltip: qsTr("This property defines the ParticleSystem3D for the emitter. If system is direct parent of the emitter, this property does not need to be defined.")
            }
            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QtQuick3D.Particles3D.ParticleSystem3D"
                    Layout.fillWidth: true
                    backendValue: backendValues.system
                }
            }

            Label {
                text: qsTr("Emit Bursts")
                tooltip: qsTr("This property takes a list of EmitBurst3D elements, to declaratively define bursts.")
            }
            SecondColumnLayout {
                EditableListView {
                    backendValue: backendValues.emitBursts
                    model: backendValues.emitBursts.expressionAsList
                    Layout.fillWidth: true
                    typeFilter: "QtQuick3D.Particles3D.EmitBurst3D"

                    onAdd: function(value) { backendValues.emitBursts.idListAdd(value) }
                    onRemove: function(idx) { backendValues.emitBursts.idListRemove(idx) }
                    onReplace: function (idx, value) { backendValues.emitBursts.idListReplace(idx, value) }
                }
            }

            Label {
                text: qsTr("Velocity")
                tooltip: qsTr("This property can be used to set a starting velocity for emitted particles.")
            }
            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QQuick3DParticleDirection"
                    Layout.fillWidth: true
                    backendValue: backendValues.velocity
                }
            }

            Label {
                text: qsTr("Particle")
                tooltip: qsTr("This property defines the scale multiplier of the particles at the beginning.")
            }
            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QtQuick3D.Particles3D.Particle3D"
                    Layout.fillWidth: true
                    backendValue: backendValues.particle
                }
            }

            Label {
                text: qsTr("Enabled")
                tooltip: qsTr("If enabled is set to false, this emitter will not emit any particles.")
            }
            SecondColumnLayout {
                CheckBox {
                    id: enabledCheckBox
                    text: backendValues.enabled.valueToString
                    backendValue: backendValues.enabled
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Shape")
                tooltip: qsTr("This property defines optional shape for the emitting area.")
            }
            SecondColumnLayout {
                IdComboBox {
                    typeFilter: "QQuick3DParticleAbstractShape"
                    Layout.fillWidth: true
                    backendValue: backendValues.shape
                }
            }

            Label {
                text: qsTr("Emit Rate")
                tooltip: qsTr("This property defines the constant emitting rate in particles per second.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 999999
                    minimumValue: 0
                    realDragRange: 5000
                    decimals: 2
                    backendValue: backendValues.emitRate
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Life Span")
                tooltip: qsTr("This property defines the lifespan of a single particle in milliseconds.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 999999
                    minimumValue: 0
                    realDragRange: 5000
                    decimals: 0
                    backendValue: backendValues.lifeSpan
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Life Span Variation")
                tooltip: qsTr("This property defines the lifespan variation of a single particle in milliseconds.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 999999
                    minimumValue: -999999
                    realDragRange: 5000
                    decimals: 0
                    backendValue: backendValues.lifeSpanVariation
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Particle Scale")
                tooltip: qsTr("This property defines the scale multiplier of the particles at the beginning")
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
                text: qsTr("Particle End Scale")
                tooltip: qsTr("This property defines the scale multiplier of the particles at the end of particle lifeSpan.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 999999
                    minimumValue: -999999
                    realDragRange: 5000
                    decimals: 2
                    backendValue: backendValues.particleEndScale
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Particle Scale Variation")
                tooltip: qsTr("This property defines the scale variation of the particles.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 999999
                    minimumValue: -999999
                    realDragRange: 5000
                    decimals: 2
                    backendValue: backendValues.particleScaleVariation
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Particle End Scale Variation")
                tooltip: qsTr("This property defines the scale variation of the particles in the end.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 999999
                    minimumValue: -999999
                    realDragRange: 5000
                    decimals: 2
                    backendValue: backendValues.particleEndScaleVariation
                    Layout.fillWidth: true
                }
            }

            Label {
                text: qsTr("Depth Bias")
                tooltip: qsTr("Holds the depth bias of the emitter. Depth bias is added to the object distance from camera when sorting objects.")
            }
            SecondColumnLayout {
                SpinBox {
                    maximumValue: 999999
                    minimumValue: -999999
                    realDragRange: 5000
                    decimals: 2
                    backendValue: backendValues.depthBias
                    Layout.fillWidth: true
                }
            }
        }
    }
    Section {
        width: parent.width
        caption: qsTr("Particle Rotation")

        SectionLayout {
            GridLayout {
                columns: 2
                rows: 1
                columnSpacing: 24
                rowSpacing: 12
                width: parent.parent.width
                ColumnLayout {
                    Label {
                        text: qsTr("Rotation")
                        tooltip: qsTr("This property defines the rotation of the particles in the beginning. Rotation is defined as degrees in euler angles.")
                    }
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
                            backendValue: backendValues.particleRotation_x
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
                            backendValue: backendValues.particleRotation_y
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
                            backendValue: backendValues.particleRotation_z
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                        }
                    }
                }

                ColumnLayout {
                    Label {
                        text: qsTr("Variation")
                        tooltip: qsTr("This property defines the rotation variation of the particles in the beginning. Rotation variation is defined as degrees in euler angles.")
                    }
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
                            backendValue: backendValues.particleRotationVariation_x
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
                            backendValue: backendValues.particleRotationVariation_y
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
                            backendValue: backendValues.particleRotationVariation_z
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                        }
                    }
                }
            }
        }
    }
    Section {
        width: parent.width
        caption: qsTr("Particle Velocity")

        SectionLayout {
            GridLayout {
                columns: 2
                rows: 1
                columnSpacing: 24
                rowSpacing: 12
                width: parent.parent.width
                ColumnLayout {
                    Label {
                        text: qsTr("Velocity")
                        tooltip: qsTr("This property defines the rotation velocity of the particles in the beginning. Rotation velocity is defined as degrees per second in euler angles.")
                    }
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
                            backendValue: backendValues.particleRotationVelocity_x
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
                            backendValue: backendValues.particleRotationVelocity_y
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
                            backendValue: backendValues.particleRotationVelocity_z
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                        }
                    }
                }
                ColumnLayout {
                    Label {
                        text: qsTr("Variation")
                        tooltip: qsTr("This property defines the rotation velocity variation of the particles. Rotation velocity variation is defined as degrees per second in euler angles.")
                    }
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
                            backendValue: backendValues.particleRotationVelocityVariation_x
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
                            backendValue: backendValues.particleRotationVelocityVariation_y
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
                            backendValue: backendValues.particleRotationVelocityVariation_z
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                        }
                    }
                }
            }
        }
    }
}
