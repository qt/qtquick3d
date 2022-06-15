// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers
import QtQuick.Controls

Window {
    visible: true
    width: 1280
    height: 720
    title: qsTr("Instanced Picking")

    View3D {
        id: viewport
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "gray"
            backgroundMode: SceneEnvironment.Color
        }

        Node {
            PerspectiveCamera {
                z: 1000
                SequentialAnimation  on z {
                    running: false
                    loops: -1
                    PauseAnimation {
                        duration: 4000
                    }
                    NumberAnimation {
                        from: 5000
                        to: 0
                        duration: 15000
                    }
                    PauseAnimation {
                        duration: 300
                    }
                    NumberAnimation {
                        from: 0
                        to: 5000
                        duration: 5000
                    }
                }
            }

            PropertyAnimation on eulerRotation.y {
                from: 0
                to: 360
                running: false
                duration: 17000
                loops: -1
            }
        }

        SpotLight {
            position: Qt.vector3d(0, 800, 0)
            eulerRotation.x: -90
            coneAngle: 60
            color: Qt.rgba(1.0, 1.0, 0.1, 1.0)
            brightness: 50
            castsShadow: true
            shadowMapQuality: Light.ShadowMapQualityHigh
            ambientColor: "#777"
            shadowFactor: 90
        }

        InstanceList {
            id: manualInstancing
            instances: [
                InstanceListEntry {
                    position: Qt.vector3d(0, 0, 0)
                    color: "green"
                },
                InstanceListEntry {
                    position: Qt.vector3d(-300, 0, 200)
                    color: "red"
                    customData: Qt.vector4d(1.0, 0.0, 1.0, 1.0)
                },
                InstanceListEntry {
                    position: Qt.vector3d(200, 200, 100)
                    color: "blue"
                    customData: Qt.vector4d(0.0, 0.3, 1.0, 1.0)
                },
                InstanceListEntry {
                    position: Qt.vector3d(300, 0, 00)
                    eulerRotation: Qt.vector3d(-10, 0, 30)
                    color: "orange"
                    customData: Qt.vector4d(0.0, 0.5, 1.0, 1.0)
                }

            ]
        }

        RandomInstancing {
            id: randomWithData
            instanceCount: 100

            position: InstanceRange {
                from: Qt.vector3d(-500, -400, -500)
                to: Qt.vector3d(500, 400, 500)
            }
            scale: InstanceRange {
                from: Qt.vector3d(0.1, 0.1, 0.1)
                to: Qt.vector3d(0.7, 0.7, 0.7)
                proportional: true
            }
            rotation: InstanceRange {
                from: Qt.vector3d(0, 0, 0)
                to: Qt.vector3d(360, 360, 360)
            }
            color: InstanceRange {
                from: Qt.rgba(0.1, 0.1, 0.1, 1.0)
                to: Qt.rgba(1, 1, 1, 1.0)
            }
            // instancedMaterial custom data:  METALNESS, ROUGHNESS, FRESNEL_POWER, SPECULAR_AMOUNT
            customData: InstanceRange {
                from: Qt.vector4d(0, 0, 0, 0)
                to: Qt.vector4d(1, 1, 5, 1)
            }
        }

        FileInstancing {
            id: fileInstancing
            source: "test.xml"
        }

        CustomMaterial {
            id: instancedMaterial
            shadingMode: CustomMaterial.Shaded
            fragmentShader: "material.frag"
            vertexShader: "material.vert"
            property color uDiffuse: "lightgray"
        }

        Model {
            id: cubeModel
            source: "#Cube"
            instancing: randomWithData

            materials: DefaultMaterial {
                diffuseColor: "white"
            }

            Model {
                id: sphereModel
                source: "#Sphere"
                instancing: parent.instancing
                instanceRoot: parent

                property real scaleFactor: 0.5
                scale: Qt.vector3d(scaleFactor, scaleFactor, scaleFactor);
                position: Qt.vector3d(-50, -50, -50)
                materials: instancedMaterial

                SequentialAnimation on scaleFactor {
                    PauseAnimation {duration: 3000}
                    NumberAnimation {from: 0.5; to: 1.5; duration: 200}
                    NumberAnimation {from: 1.5; to: 0.5; duration: 200}
                    loops: -1
                }
            } // sphereModel
        } // cubeModel


// For reference: this is how to do it using the InstanceModel

//        Repeater3D {
//            model: TestModel { instancingTable: randomWithData }
//        }
//            Model {
//                id: delegate
//                source: "#Cube"
//                pickable: true
//                property int instanceIndex: index
//                position: modelPosition
//                scale: modelScale
//                rotation: modelRotation
//                materials: [
//                DefaultMaterial {
//                    diffuseColor: modelColor
//                }
//                ]
//                opacity: 0.5
//            }
//        }


        InstanceRepeater {
            instancingTable: randomWithData
            Model {
                id: delegate
                source: "#Cube"
                pickable: true
                property int instanceIndex: index // used for indexLabel.text
                property color instanceColor: modelColor
                opacity: 0
            }
        }



    } // View3D


    MouseArea {
        anchors.fill: viewport

        //! [mouse area]

        onClicked: {
            // Get screen coordinates of the click
            positionLabel.text = "(" + mouse.x + ", " + mouse.y + ")"
            //! [pick result]
            var result = viewport.pick(mouse.x, mouse.y);
            //! [pick result]
            //! [pick specifics]
            if (result.objectHit) {
                var pickedObject = result.objectHit;


                // Toggle the isPicked property for the model
///////                pickedObject.isPicked = !pickedObject.isPicked;



                // Get picked model index
                indexLabel.text = pickedObject.instanceIndex
                // Get other pick specifics
                uvPosition.text = "("
                        + result.uvPosition.x.toFixed(2) + ", "
                        + result.uvPosition.y.toFixed(2) + ")";
                distance.text = result.distance.toFixed(2);
                scenePosition.text = "("
                        + result.scenePosition.x.toFixed(2) + ", "
                        + result.scenePosition.y.toFixed(2) + ", "
                        + result.scenePosition.z.toFixed(2) + ")";
                localPosition.text = "("
                        + result.position.x.toFixed(2) + ", "
                        + result.position.y.toFixed(2) + ", "
                        + result.position.z.toFixed(2) + ")";
                colorRect.color = pickedObject.instanceColor
                //! [pick specifics]
            } else {
                indexLabel.text = "None";
                uvPosition.text = "";
                distance.text = "";
                scenePosition.text = "";
                localPosition.text = "";
                colorRect.color = "transparent"
            }
        }
    }

    Row {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 8
        spacing: 10
        Column {
            Label {
                color: "white"
                font.pointSize: 14
                text: "Screen Position:"
            }
            Label {
                color: "white"
                font.pointSize: 14
                text: "Index:"
            }
            Label {
                color: "white"
                font.pointSize: 14
                text: "Color:"
            }
            Label {
                color: "white"
                font.pointSize: 14
                text: "UV Position:"
            }
            Label {
                color: "white"
                font.pointSize: 14
                text: "Distance:"
            }
            Label {
                color: "white"
                font.pointSize: 14
                text: "World Position:"
            }
            Label {
                color: "white"
                font.pointSize: 14
                text: "Local Position:"
            }

        }
        Column {
            Label {
                id: positionLabel
                color: "white"
                font.pointSize: 14
            }
            Label {
                id: indexLabel
                color: "white"
                font.pointSize: 14
            }
            Rectangle {
                id: colorRect
                height: indexLabel.height
                width: indexLabel.width
                color: "transparent"
            }
            Label {
                id: uvPosition
                color: "white"
                font.pointSize: 14
            }
            Label {
                id: distance
                color: "white"
                font.pointSize: 14
            }
            Label {
                id: scenePosition
                color: "white"
                font.pointSize: 14
            }
            Label {
                id: localPosition
                color: "white"
                font.pointSize: 14
            }
        }
    }
}
