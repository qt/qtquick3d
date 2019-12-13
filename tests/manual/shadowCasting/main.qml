import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick3D 1.0
import QtQuick3D.Helpers 1.0

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Shadow Casting Test")

    View3D {
        id: view
        anchors.fill: parent
        camera: camera1

        DirectionalLight {
            castsShadow: true
            shadowFactor: 25
            rotation: Qt.vector3d(45, 0, 0)

        }


        Model {
            id: ground
            source: "#Cube"

            scale: Qt.vector3d(10, 0.01, 10)

            materials: DefaultMaterial {
                //diffuseColor: "green"
            }

            castsShadows: false
        }

        Model {
            source: "#Sphere"

            y: 50

            materials: DefaultMaterial {

            }

            castsShadows: false
        }

        Model {
            source: "#Cylinder"
            y: 200

            x: -250
            scale: Qt.vector3d(1, 5, 1)

            materials: DefaultMaterial {

            }

            castsShadows: true
        }

        Model {
            source: "#Cube"

            x: -250
            z: 250
            y: 50

            materials: DefaultMaterial {

            }

            receivesShadows: false
        }

        PerspectiveCamera {
            id: camera1
            z: -300
            y: 200
            clipFar: 1000

            rotation: Qt.vector3d(45, 0, 0)

        }



    }

    WasdController {

        controlledObject: view.camera
    }

}
