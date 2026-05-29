// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Node {
    id: object_1_mesh

    PrincipledMaterial {
        id: material_002_material8
        objectName: "Material.002"
        baseColor: "#ffcccccc"
        roughness: 0.5
    }

    Node {
        id: rootNode1
        objectName: "RootNode"
        eulerRotation: Qt.vector3d(0, 20, 0)

        Model {
            scale: Qt.vector3d(5, 1, 5)
            y: -50
            source: "#Cylinder"
            materials: [ material_002_material8 ]
        }
        Model {
            scale: Qt.vector3d(10, 1, 10)
            y: -100
            source: "#Cylinder"
            materials: [ material_002_material8 ]
        }
        Model {
            scale: Qt.vector3d(15, 1, 15)
            y: -150
            source: "#Cylinder"
            materials: [ material_002_material8 ]
        }
        Model {
            scale: Qt.vector3d(25, 1, 25)
            y: -200
            source: "#Cylinder"
            materials: [ material_002_material8 ]
        }
        Model {
            scale: Qt.vector3d(30, 1, 30)
            y: -250
            source: "#Cylinder"
            materials: [ material_002_material8 ]
        }

        Model {
            scale: Qt.vector3d(50, 1, 50)
            y: -300
            source: "#Cylinder"
            materials: [ material_002_material8 ]
        }

        Model {
            scale: Qt.vector3d(200, 200, 230)
            eulerRotation.x: -90
            source: "assets/tree.mesh"
            materials: [ material_002_material8 ]
        }

        Model {
            objectName: "torusRing"
            y: 800
            geometry: TorusGeometry {
                radius: 120
                tubeRadius: 20
                segments: 64
                rings: 32
            }
            scale: Qt.vector3d(5, 2, 5)
            materials: [ material_002_material8 ]
        }

        Repeater3D {
            model: 8
            delegate: Model {
                required property int index
                source: "#Cylinder"
                x: Math.cos(index * Math.PI / 4) * 600
                z: Math.sin(index * Math.PI / 4) * 600
                y: 300
                scale: Qt.vector3d(0.5, 10, 0.5)
                materials: [ material_002_material8 ]
            }
        }
    }
}
