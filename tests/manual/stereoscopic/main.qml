import QtQuick 2.11
import QtQuick.Window 2.11

import QtQuick3D 1.0

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Stereoscopic Rendering Example")


    // Scene
    Node {
        id: sceneRoot

        Light {

        }

        Model {
            id: cube
            source: "#Cube"
            materials: DefaultMaterial {
            }


            NumberAnimation {
                target: cube
                property: "rotation.x"
                duration: 2000
                easing.type: Easing.InOutQuad
                loops: -1
                running: true
                from: 0
                to: 360
            }
        }

        Model {
            id: cone
            source: "#Cone"
            z: 250
            x: -150
            materials: DefaultMaterial {
            }
            SequentialAnimation {
                running: true
                loops: -1

                NumberAnimation {
                    target: cone
                    property: "x"
                    duration: 5000
                    easing.type: Easing.InOutQuad
                    from: -150
                    to: 150
                }
                NumberAnimation {
                    target: cone
                    property: "x"
                    duration: 5000
                    easing.type: Easing.InOutQuad
                    from: 150
                    to: -150
                }
            }
        }

        Model {
            id: cylinder
            source: "#Cylinder"
            z: -250
            x: 150
            materials: DefaultMaterial {

            }

            SequentialAnimation {
                running: true
                loops: -1

                NumberAnimation {
                    target: cylinder
                    property: "x"
                    duration: 5000
                    easing.type: Easing.InOutQuad
                    from: 150
                    to: -150
                }
                NumberAnimation {
                    target: cylinder
                    property: "x"
                    duration: 5000
                    easing.type: Easing.InOutQuad
                    from: -150
                    to: 150
                }
            }
        }

        Node {
            id: stereoCamera

            property real convergence: 600.0
            property real eyeSeparation: 35.0
            property real fieldOfView: 60.0
            property real nearPlane: 10.0
            property real farPlane: 2000.0
            property real aspectRatio: leftEyeView.width / leftEyeView.height

            property real _fov2: Math.tan(fieldOfView * Math.PI / 180 * 0.5)
            property real top: nearPlane * _fov2
            property real a: aspectRatio * _fov2 * convergence

            z: -600

            Camera {
                id: leftEyeCamera
                x: -stereoCamera.eyeSeparation * 0.5
                clipNear: stereoCamera.nearPlane
                clipFar: stereoCamera.farPlane
                //                fieldOfView: stereoCamera.fieldOfView
                //                isFieldOfViewHorizontal: true
                projectionMode: Camera.Frustum
                frustumLeft: -(stereoCamera.a - stereoCamera.eyeSeparation * 0.5) * stereoCamera.nearPlane / stereoCamera.convergence
                frustumRight: (stereoCamera.a + stereoCamera.eyeSeparation * 0.5) * stereoCamera.nearPlane / stereoCamera.convergence
                frustumTop: stereoCamera.top
                frustumBottom: -stereoCamera.top
            }
            Camera {
                id: rightEyeCamera
                x: -stereoCamera.eyeSeparation * 0.5
                clipNear: stereoCamera.nearPlane
                clipFar: stereoCamera.farPlane
                projectionMode: Camera.Frustum
                //                fieldOfView: stereoCamera.fieldOfView
                //                isFieldOfViewHorizontal: true
                frustumLeft: -(stereoCamera.a + stereoCamera.eyeSeparation * 0.5) * stereoCamera.nearPlane / stereoCamera.convergence
                frustumRight: (stereoCamera.a - stereoCamera.eyeSeparation * 0.5) * stereoCamera.nearPlane / stereoCamera.convergence
                frustumTop: stereoCamera.top
                frustumBottom: -stereoCamera.top
            }
        }
    }

    View3D {
        id: leftEyeView
        anchors.fill: parent

        scene: sceneRoot
        camera: leftEyeCamera
        layer.enabled: true
        layer.format: ShaderEffectSource.RGBA
        visible: false
    }

    View3D {
        id: rightEyeView
        anchors.fill: parent
        scene: sceneRoot
        camera: rightEyeCamera
        layer.enabled: true
        layer.format: ShaderEffectSource.RGBA
        visible: false
    }

    ShaderEffect {
        id: outputView
        anchors.fill: parent
        property variant leftEye: leftEyeView
        property variant rightEye: rightEyeView
        blending: true
        fragmentShader: "
                            varying highp vec2 coord;
                            uniform sampler2D leftEye;
                            uniform sampler2D rightEye;
                            uniform lowp float qt_Opacity;
                            void main() {
                                lowp vec4 tex1 = texture2D(leftEye, coord);
                                lowp vec4 tex2 = texture2D(rightEye, coord);
                                gl_FragColor = vec4(tex1.r, tex2.gb, 1);
                                //gl_FragColor = vec4(tex1.r, tex2.gb, tex1.a * qt_Opacity);
                            }"
    }
}
