import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick3D 1.0

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello World")
    color: "black"

    Node {
        id: sceneRoot

        Light {

        }

        Model {
            source: "#Cone"

            materials: DefaultMaterial {

            }
        }

        Model {
            source: "#Sphere"
            z: 100
            x: -100
            materials: DefaultMaterial {

            }
        }

        Model {
            source: "#Cube"
            z: -100
            x: 100
            materials: DefaultMaterial {

            }
        }
    }

    View3D {
        anchors.top: parent.top
        anchors.left: parent.left
        width: parent.width * 0.5
        height: parent.height * 0.5
        scene: sceneRoot
        Camera {
            id: perspectiveCamera
            z: -600
            projectionMode: Camera.Perspective
        }
    }
    View3D {
        anchors.top: parent.top
        anchors.right: parent.right
        width: parent.width * 0.5
        height: parent.height * 0.5
        scene: sceneRoot

        Camera {
            id: orthgraphicCamera
            z: -600
            projectionMode: Camera.Orthographic
        }
    }
    View3D {
        id: frustumView
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: parent.width * 0.5
        height: parent.height * 0.5
        scene: sceneRoot
        Camera {
            id: frustumCamera
            z: -600
            projectionMode: Camera.Frustum
            frustumTop: frustumView.height * 0.05
            frustumBottom: frustumView.height * -0.05
            frustumRight: frustumView.width * 0.05
            frustumLeft: frustumView.width * -0.05
        }
    }
    View3D {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: parent.width * 0.5
        height: parent.height * 0.5
        scene: sceneRoot
        Camera {
            id: customCamera
            z: -600
            projectionMode: Camera.Custom
            customProjection: Qt.matrix4x4(1.299, 0, 0, 0,
                                           0, 1.732, 0, 0,
                                           0, 0, -1, -20,
                                           0, 0, -1, 0);
        }
    }

}
