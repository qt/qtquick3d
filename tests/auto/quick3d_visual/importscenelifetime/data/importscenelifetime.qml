import QtQuick
import QtQuick3D

Item {
    id: root
    width: 200
    height: 200

    // Drives the Loader below. Toggled from the test to destroy and recreate
    // the importing View3D.
    property bool loadView: true

    // The shared scene lives outside any View3D, so it is owned by the QML tree
    // and outlives the views that import it. The InstanceList is both a child of
    // the Node and referenced by the Model.
    Node {
        id: sharedScene

        InstanceList {
            id: sharedInstancing
            instances: [
                InstanceListEntry { position: Qt.vector3d(-50, 0, 0); color: "red" },
                InstanceListEntry { position: Qt.vector3d(50, 0, 0); color: "blue" }
            ]
        }

        Model {
            source: "#Cube"
            instancing: sharedInstancing
            materials: PrincipledMaterial { baseColor: "white" }
        }

        DirectionalLight {}
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
            PerspectiveCamera { z: 300 }
        }
    }
}
