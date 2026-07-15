import QtQuick
import QtQuick3D

View3D {
    id: view
    objectName: "view"
    anchors.fill: parent
    camera: sceneCamera
    environment: SceneEnvironment {
        backgroundMode: SceneEnvironment.Color
        clearColor: "black"
    }

    // Orthographic, so a model at (x, y) is centered at (300 + x, 200 - y)
    OrthographicCamera {
        id: sceneCamera
        z: 600
    }

    component CullModeRectangle: Model {
        source: "#Rectangle"
        pickable: true
        property alias color: material.baseColor
        property alias cullMode: material.cullMode
        materials: PrincipledMaterial {
            id: material
            lighting: PrincipledMaterial.NoLighting
        }
    }

    // Rotated 180 degrees about Y, so the back face is toward the camera
    CullModeRectangle {
        objectName: "backCull"
        position: Qt.vector3d(-200, 100, 0)
        eulerRotation.y: 180
        color: "#ff0000"
        cullMode: Material.BackFaceCulling
    }
    CullModeRectangle {
        objectName: "frontCull"
        position: Qt.vector3d(0, 100, 0)
        eulerRotation.y: 180
        color: "#00ff00"
        cullMode: Material.FrontFaceCulling
    }
    CullModeRectangle {
        objectName: "noCull"
        position: Qt.vector3d(200, 100, 0)
        eulerRotation.y: 180
        color: "#0000ff"
        cullMode: Material.NoCulling
    }

    // Mirrored on X, which flips the winding the renderer sees
    CullModeRectangle {
        objectName: "mirrorBackCull"
        position: Qt.vector3d(-200, -100, 0)
        scale: Qt.vector3d(-1, 1, 1)
        color: "#ffff00"
        cullMode: Material.BackFaceCulling
    }
    CullModeRectangle {
        objectName: "mirrorFrontCull"
        position: Qt.vector3d(0, -100, 0)
        scale: Qt.vector3d(-1, 1, 1)
        color: "#00ffff"
        cullMode: Material.FrontFaceCulling
    }
    CullModeRectangle {
        objectName: "mirrorNoCull"
        position: Qt.vector3d(200, -100, 0)
        scale: Qt.vector3d(-1, 1, 1)
        color: "#ff00ff"
        cullMode: Material.NoCulling
    }
}
