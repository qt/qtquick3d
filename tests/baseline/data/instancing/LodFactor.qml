// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick3D
import QtQuick
import QtQuick3D.Helpers

Rectangle {
    id: primitives
    width: 1200
    height: 720
    color: Qt.rgba(0, 0, 0, 1)

    View3D {
        id: layer
        anchors.fill: parent
        visible: true

        environment: SceneEnvironment {
            clearColor:  Qt.rgba(1, 1, 1, 1)
            depthPrePassEnabled: true
            backgroundMode: SceneEnvironment.Color
        }

        DirectionalLight {
            id: light
            shadowFactor: 10
        }

        camera: PerspectiveCamera {
            position.z: 0
            position.y: 1.5

            eulerRotation.x: -20

            clipNear: 0.1
        }

        RandomInstancing {
            id: instanceTable
            instanceCount: 1000
            randomSeed: 0
            position: InstanceRange {
                from: Qt.vector3d(-10, 0, 0)
                to: Qt.vector3d(10, 0, -100)
            }
        }

        Model {
            //lod configuration
            instancingLodFactor: 0.0
            instancingLodMin: 0
            instancingLodMax: 5


            objectName: "Retopo_Mesh1.0"
            source: "mesh_005_mesh.mesh"
            materials: [
                white_painted_metal_material
            ]

            instancing: instanceTable

        }

        Model {

            //lod configuration
            instancingLodFactor: 0.5
            instancingLodMin: 5
            instancingLodMax: 10

            objectName: "Retopo_Mesh1.0"
            source: "mesh_005_mesh.mesh"
            materials: [
                white_painted_metal_material
            ]

            instancing: instanceTable

        }
        Model {
            //lod configuration
            instancingLodFactor: 1
            instancingLodMin: 10
            instancingLodMax: 100

            objectName: "Retopo_Mesh1.0"
            source: "mesh_005_mesh.mesh"

            instancing: instanceTable

            materials: [
                white_painted_metal_material
            ]
        }


        // Resources
        property url textureData: "textureData.png"
        property url textureData7: "textureData7.png"
        property url textureData9: "textureData9.png"
        Texture {
            id: _0_texture
            generateMipmaps: true
            mipFilter: Texture.Linear
            source: layer.textureData
        }

        Texture {
            id: _1_texture
            generateMipmaps: true
            mipFilter: Texture.Linear
            source: layer.textureData7
        }

        Texture {
            id: _2_texture
            generateMipmaps: true
            mipFilter: Texture.Linear
            source: layer.textureData9
        }


        PrincipledMaterial {
            id: white_painted_metal_material
            objectName: "White painted metal"
            baseColorMap: _0_texture
            metalnessMap: _1_texture
            roughnessMap: _1_texture

            normalMap: _2_texture
            alphaMode: PrincipledMaterial.Opaque
            indexOfRefraction: 5
        }
    }

}
