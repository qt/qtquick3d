import QtQuick
import QtQuick3D

View3D {
    id: view
    objectName: "view"
    anchors.fill: parent
    environment: SceneEnvironment {
        backgroundMode: SceneEnvironment.Color
        clearColor: "black"
    }
    OrthographicCamera { z: 600 }
    DirectionalLight { }
    Model {
        id: model1
        objectName: "model1"
        source: "#Cube"
        pickable: true
        materials: PrincipledMaterial {
            baseColor: "red"
        }
    }
    Model {
        id: model2
        objectName: "model2"
        source: "#Cube"
        pickable: true
        position: Qt.vector3d(50.0, 50.0, -50.0)
        materials: PrincipledMaterial {
            baseColor: "green"
        }
    }
}
