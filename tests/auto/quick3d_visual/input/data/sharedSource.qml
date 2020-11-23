import QtQuick
import QtQuick3D

View3D {
    width: 1024; height: 480

    environment: SceneEnvironment {
        clearColor: "#111"
        backgroundMode: SceneEnvironment.Color
    }

    PerspectiveCamera {
        z: 600
    }

    DirectionalLight {
        eulerRotation.y: -5
    }

    DirectionalLight {
        eulerRotation.y: 2
    }

    Node {
        objectName: "left object"
        x: -120
        z: 400
        Model {
            source: "#Cube"
            pickable: true
            materials: DefaultMaterial {
                diffuseMap: Texture {
                    sourceItem: BusyBox {
                        objectName: "left busybox"
                        layer.enabled: true
                        layer.textureSize: Qt.size(512, 512)
                        id: sharedItem
                        x: 10; y: 10
                    }
                }
            }
        }
    }

    Node {
        objectName: "right object"
        x: 120
        z: 400
        eulerRotation.x: -25
        eulerRotation.y: 5
        Model {
            source: "#Cube"
            pickable: true
            materials: DefaultMaterial {
                diffuseMap: Texture {
                    sourceItem: BusyBox {
                        objectName: "right busybox"
                        layer.enabled: true
                        layer.textureSize: Qt.size(512, 512)
                        id: sharedItem2
                        x: 10; y: 10
                    }
                }
            }
        }
    }
}
