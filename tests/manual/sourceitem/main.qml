import QtQuick 2.15
import QtQuick.Window 2.12
import QtQuick3D 1.15
import QtQuick.Controls 2.12

Window {
    id: window
    width: 1280
    height: 720
    visible: true
    View3D {
        id: sceneView
        anchors.fill: parent
        focus: true

        environment: SceneEnvironment {
            id: environment
            probeBrightness: 5
            clearColor: "#cceeff"
            backgroundMode: SceneEnvironment.Color
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 0, 0)
        }

        Model {
            position: Qt.vector3d(0, 0, -300)
            source: "#Rectangle"
            materials: [
                DefaultMaterial {
                    lighting: DefaultMaterial.NoLighting
                    diffuseMap: Texture {
                        id: texture

                        sourceItem: Rectangle {
                            width: 256
                            height: 256
                            color: "lightgreen"
                            Rectangle {
                                anchors.centerIn: parent
                                width: 140
                                height: 140
                                color: "yellow"
                                NumberAnimation on rotation {
                                    running: animationCheckbox.checked
                                    from: 0
                                    to: 90
                                    loops: Animation.Infinite
                                }
                            }
                        }
                    }
                }
            ]
        }

        // References Texture
        Model {
            position: Qt.vector3d(-150, 0, -300)
            source: "#Sphere"
            NumberAnimation on eulerRotation.y {
                running: animation3DCheckbox.checked
                duration: 4000
                from: 0
                to: 360
                loops: Animation.Infinite
            }
            NumberAnimation on eulerRotation.z {
                running: animation3DCheckbox.checked
                duration: 8000
                from: 0
                to: 360
                loops: Animation.Infinite
            }
            materials: [
                DefaultMaterial {
                    lighting: DefaultMaterial.NoLighting
                    diffuseMap: texture
                }
            ]
        }

        // References another Texture's sourceItem
        Model {
            position: Qt.vector3d(150, 0, -300)
            source: "#Cube"
            NumberAnimation on eulerRotation.y {
                running: animation3DCheckbox.checked
                duration: 1000
                from: 0
                to: 90
                loops: Animation.Infinite
            }
            materials: [
                DefaultMaterial {
                    lighting: DefaultMaterial.NoLighting
                    diffuseMap: Texture {
                        sourceItem: texture.sourceItem
                    }
                }
            ]
        }
    }
    Component {
        id: sourceItemComponent
        Rectangle {
            id: img
            width: 500
            height: 500
            gradient: "NightFade"
            CheckBox {
                text: "Some quick control in a texture :D"
                checked: true
            }
            Rectangle {
                anchors.centerIn: parent
                color: "blue"
                width: 140
                height: 140
                NumberAnimation on rotation {
                    running: animationCheckbox.checked
                    from: 0
                    to: 90
                    loops: Animation.Infinite
                }
                NumberAnimation on width {
                    running: sizeAnimationCheckbox.checked
                    duration: 3000
                    from: 250
                    to: 500
                    loops: Animation.Infinite
                }
                NumberAnimation on height {
                    running: sizeAnimationCheckbox.checked
                    duration: 3000
                    from: 250
                    to: 500
                    loops: Animation.Infinite
                }
            }
        }
    }
    Column {
        Button {
            text: "Assign new sourceItem"
            onClicked: {
                console.log("assigning new sourceItem");
                const item = sourceItemComponent.createObject(texture);
                texture.sourceItem = item;
            }
        }
        Button {
            text: "Set sourceItem null"
            onClicked: {
                texture.sourceItem.destroy();
                texture.sourceItem = null;
            }
        }
        Button {
            text: "Destroy sourceItem"
            onClicked: texture.sourceItem.destroy()
        }
        CheckBox {
            id: animationCheckbox
            text: "Animate quick content"
        }
        CheckBox {
            id: sizeAnimationCheckbox
            text: "Animate texture size"
        }
        CheckBox {
            id: animation3DCheckbox
            text: "Animate 3D objects"
        }
    }
}
