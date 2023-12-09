// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D

import QtQuick.Timeline

Node {
    id: node

    property alias heartTimeline: heartTimeline
    property alias heartAnimation: heartAnimation

    scale.x: 0.4
    scale.y: 0.4
    scale.z: 0.4

    // Resources
    Skin {
        id: skin
        joints: base
        inverseBindPoses: Qt.matrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
    }

    // Nodes:
    Node {
        id: heart_Arm
        Model {
            id: plane
            source: "meshes/plane.mesh"
            skin: skin
            materials: heartMaterial_material
        }
        Node {
            id: base
        }
    }

    // Animations:
    Timeline {
        id: heartTimeline
        startFrame: 0
        endFrame: 1334
        currentFrame: 0
        enabled: false
        animations: TimelineAnimation {
            id: heartAnimation
            duration: 1334
            from: 0
            to: 1334
            running: heartTimeline.enabled
            loops: Animation.Infinite
            alwaysRunToEnd: true
        }
        KeyframeGroup {
            target: base
            property: "scale"
            Keyframe {
                frame: 0
                value: Qt.vector3d(0.897612, 0.897612, 0.897612)
            }
            Keyframe {
                frame: 33.3333
                value: Qt.vector3d(0.898435, 0.898435, 0.898435)
            }
            Keyframe {
                frame: 66.6667
                value: Qt.vector3d(0.900828, 0.900828, 0.900828)
            }
            Keyframe {
                frame: 100
                value: Qt.vector3d(0.904679, 0.904679, 0.904679)
            }
            Keyframe {
                frame: 133.333
                value: Qt.vector3d(0.909876, 0.909876, 0.909876)
            }
            Keyframe {
                frame: 166.667
                value: Qt.vector3d(0.916307, 0.916307, 0.916307)
            }
            Keyframe {
                frame: 200
                value: Qt.vector3d(0.92386, 0.92386, 0.92386)
            }
            Keyframe {
                frame: 233.333
                value: Qt.vector3d(0.932423, 0.932423, 0.932423)
            }
            Keyframe {
                frame: 266.667
                value: Qt.vector3d(0.941883, 0.941883, 0.941883)
            }
            Keyframe {
                frame: 300
                value: Qt.vector3d(0.952128, 0.952128, 0.952128)
            }
            Keyframe {
                frame: 333.333
                value: Qt.vector3d(0.963046, 0.963046, 0.963046)
            }
            Keyframe {
                frame: 366.667
                value: Qt.vector3d(0.974525, 0.974525, 0.974525)
            }
            Keyframe {
                frame: 400
                value: Qt.vector3d(0.986453, 0.986453, 0.986453)
            }
            Keyframe {
                frame: 433.333
                value: Qt.vector3d(0.998717, 0.998717, 0.998717)
            }
            Keyframe {
                frame: 466.667
                value: Qt.vector3d(1.01121, 1.01121, 1.01121)
            }
            Keyframe {
                frame: 500
                value: Qt.vector3d(1.02381, 1.02381, 1.02381)
            }
            Keyframe {
                frame: 533.333
                value: Qt.vector3d(1.03641, 1.03641, 1.03641)
            }
            Keyframe {
                frame: 566.667
                value: Qt.vector3d(1.0489, 1.0489, 1.0489)
            }
            Keyframe {
                frame: 600
                value: Qt.vector3d(1.06116, 1.06116, 1.06116)
            }
            Keyframe {
                frame: 633.333
                value: Qt.vector3d(1.07309, 1.07309, 1.07309)
            }
            Keyframe {
                frame: 666.667
                value: Qt.vector3d(1.08457, 1.08457, 1.08457)
            }
            Keyframe {
                frame: 700
                value: Qt.vector3d(1.09548, 1.09548, 1.09548)
            }
            Keyframe {
                frame: 733.333
                value: Qt.vector3d(1.10573, 1.10573, 1.10573)
            }
            Keyframe {
                frame: 766.667
                value: Qt.vector3d(1.11519, 1.11519, 1.11519)
            }
            Keyframe {
                frame: 800
                value: Qt.vector3d(1.12375, 1.12375, 1.12375)
            }
            Keyframe {
                frame: 833.333
                value: Qt.vector3d(1.1313, 1.1313, 1.1313)
            }
            Keyframe {
                frame: 866.667
                value: Qt.vector3d(1.13774, 1.13774, 1.13774)
            }
            Keyframe {
                frame: 900
                value: Qt.vector3d(1.14293, 1.14293, 1.14293)
            }
            Keyframe {
                frame: 933.333
                value: Qt.vector3d(1.14678, 1.14678, 1.14678)
            }
            Keyframe {
                frame: 966.667
                value: Qt.vector3d(1.14918, 1.14918, 1.14918)
            }
            Keyframe {
                frame: 1000
                value: Qt.vector3d(1.15, 1.15, 1.15)
            }
            Keyframe {
                frame: 1033.33
                value: Qt.vector3d(1.14293, 1.14293, 1.14293)
            }
            Keyframe {
                frame: 1066.67
                value: Qt.vector3d(1.12375, 1.12375, 1.12375)
            }
            Keyframe {
                frame: 1100
                value: Qt.vector3d(1.09548, 1.09548, 1.09548)
            }
            Keyframe {
                frame: 1133.33
                value: Qt.vector3d(1.06116, 1.06116, 1.06116)
            }
            Keyframe {
                frame: 1166.67
                value: Qt.vector3d(1.02381, 1.02381, 1.02381)
            }
            Keyframe {
                frame: 1200
                value: Qt.vector3d(0.986453, 0.986453, 0.986453)
            }
            Keyframe {
                frame: 1233.33
                value: Qt.vector3d(0.952128, 0.952128, 0.952128)
            }
            Keyframe {
                frame: 1266.67
                value: Qt.vector3d(0.92386, 0.92386, 0.92386)
            }
            Keyframe {
                frame: 1300
                value: Qt.vector3d(0.904679, 0.904679, 0.904679)
            }
            Keyframe {
                frame: 1333.33
                value: Qt.vector3d(0.897612, 0.897612, 0.897612)
            }
        }
    }

    Node {
        id: __materialLibrary__

        PrincipledMaterial {
            id: heartMaterial_material
            objectName: "heartMaterial_material"
            baseColor: "#ff197c1a"
            roughness: 0.09734514355659485
            cullMode: PrincipledMaterial.NoCulling
            alphaMode: PrincipledMaterial.Opaque
        }
    }
}
