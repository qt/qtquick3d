import QtQuick
import QtQuick3D

Item {
    id: root
    width: 640
    height: 480
    visible: true

    property bool swapped: false

    property string skyFragmentShaderCode: `
        void MAIN()
        {
            vec3 d = normalize(qt_eyeDir);
            FRAGCOLOR = vec4(d * 0.5 + 0.5, 1.0);
        }
    `

    // The sky materials deliberately live outside the 3D scene, mimicking
    // materials provided by e.g. a QML singleton. Their only scene manager
    // reference comes from the SceneEnvironment.skyMaterial assignment, so
    // every swap destroys the backend node of the outgoing material and
    // recreates it when it is assigned again.
    SkyMaterial {
        id: skyMaterialA
        skyboxMode: SkyMaterial.Cubemap
        enableIBL: true
        radianceMapSize: 256
        fragmentShaderCode: root.skyFragmentShaderCode
    }

    SkyMaterial {
        id: skyMaterialB
        skyboxMode: SkyMaterial.Cubemap
        enableIBL: true
        radianceMapSize: 256
        fragmentShaderCode: root.skyFragmentShaderCode
    }

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.SkyMaterial
            skyMaterial: root.swapped ? skyMaterialB : skyMaterialA
        }

        PerspectiveCamera {
            z: 300
        }

        DirectionalLight {
        }

        Model {
            source: "#Cube"
            materials: PrincipledMaterial { baseColor: "white" }
        }
    }
}
