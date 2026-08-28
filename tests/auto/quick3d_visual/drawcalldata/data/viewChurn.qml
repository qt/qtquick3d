import QtQuick
import QtQuick3D

Item {
    id: root
    width: 640
    height: 480
    visible: true

    // Drives the Loader below. Toggled from the test to destroy and recreate
    // the importing View3D.
    property bool loadView: true

    // The shared scene lives outside any View3D, so it is owned by the QML
    // tree and outlives the views that import it. Draw call data created for
    // this content is keyed on the pass objects of the importing view's layer
    // and must be released when that layer goes away.
    Node {
        id: sharedScene

        PerspectiveCamera {
            z: 300
        }

        DirectionalLight {
            castsShadow: true
            eulerRotation.x: -45
        }

        Model {
            source: "#Cube"
            castsShadows: true
            receivesShadows: true
            materials: PrincipledMaterial { baseColor: "white" }
        }

        Model {
            source: "#Rectangle"
            y: -80
            scale: Qt.vector3d(5, 5, 5)
            eulerRotation.x: -90
            receivesShadows: true
            materials: PrincipledMaterial { baseColor: "gray" }
        }
    }

    Loader {
        id: viewLoader
        anchors.fill: parent
        sourceComponent: root.loadView ? viewComponent : null
    }

    Component {
        id: viewComponent
        View3D {
            anchors.fill: parent
            importScene: sharedScene

            environment: SceneEnvironment {
                backgroundMode: SceneEnvironment.Color
                clearColor: "black"
            }
        }
    }
}
