import QtQuick
import QtQuick3D

Item {
    id: root
    width: 400
    height: 200

    // Transient view adopts the shared scene first, but then is destroyed the static view should then
    // adopt the shared scene.

    property bool loadTransientView: true
    property bool staticViewImports: false

    Node {
        id: sharedScene
        Model {
            objectName: "sharedModel"
            source: "#Cube"
            materials: PrincipledMaterial { baseColor: "white" }
        }
        DirectionalLight {}
    }

    Loader {
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
        width: parent.width / 2
        sourceComponent: root.loadTransientView ? transientView : null
    }
    Component {
        id: transientView
        View3D {
            importScene: sharedScene
            PerspectiveCamera { objectName: "transientViewCamera"; z: 300 }
        }
    }

    View3D {
        objectName: "staticView"
        anchors { right: parent.right; top: parent.top; bottom: parent.bottom }
        width: parent.width / 2
        importScene: root.staticViewImports ? sharedScene : null
        PerspectiveCamera { objectName: "staticViewCamera"; z: 300 }
    }
}
