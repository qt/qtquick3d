import QtQuick
import QtQuick3D

Item {
    id: root
    width: 640
    height: 480
    visible: true

    property bool swapped: false

    // The materials deliberately live outside the 3D scene, mimicking
    // materials provided by e.g. a QML singleton. Their only scene manager
    // reference comes from the model's materials assignment, so every swap
    // destroys the backend node of the outgoing material and recreates it
    // when it is assigned again.
    CustomMaterial {
        id: materialA
        shadingMode: CustomMaterial.Shaded
        property color baseColor: "red"
    }

    CustomMaterial {
        id: materialB
        shadingMode: CustomMaterial.Shaded
        property color baseColor: "blue"
    }

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "black"
        }

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
            materials: [ root.swapped ? materialB : materialA ]
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
}
