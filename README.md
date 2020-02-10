# Qt Quick 3D

This project contains everything necessary build Qt Quick 3D (against Qt 5.12 or greater)
Qt Quick 3D is a high level 3D API for Qt Quick. Qt Quick 3D enables anyone to introduce 3D content into their Qt Quick applications.  Rather than requiring the end user to know advanced details of the graphics rendering pipeline (building framegraphs and materials), it is now possible to simply build up a 3D scene using high level primitives.

## Clone 3rdparty modules

In the qtquick3d source folder run the following to get assimp source code
```
git submodule update --init --recursive
```

## Building
To build like any other Qt module:
```
qmake qtquick3d.pro
make
make install
```
You may also need the qtquicktimeline module if you want to convert existing projects or assets that use keyframe animations.  That is found [here](https://code.qt.io/cgit/qt/qtquicktimeline.git/)

```
import QtQuick 2.15
import QtQuick.Window 2.12
import QtQuick3D 1.15

Window {
    id: window
    visible: true
    width: 1280
    height: 720

    // Viewport for 3D content
    View3D {
        id: view
        anchors.fill: parent

        // Scene to view
        Node {
            id: scene

            // To render anything to a 3D viewport, you need 3 things
            // Light, Camera, Model
            Light {
                id: directionalLight
            }

            Camera {
                id: camera
                // It's important that your camera is not inside your model
                // So move it back a big along the z axis
                // The Camera is implicitly facing up the z axis, so we should be looking towards
                // (0, 0, 0)
                z: -600
            }

            Model {
                id: cubeModel
                // #Cube is one of the "built-in" primitive meshes
                // Other Options are #Cone, #Sphere, #Cylinder, #Rectangle
                source: "#Cube"

                // When using a Model, it is not enough to have a mesh source (ie "#Cube")
                // You also need to define what material to shade the mesh with. A Model can be
                // built up of multiple sub-meshes, so each mesh needs its own material
                // So materials are defined in an array, and order reflects which mesh to shade
                // All of the default primitive meshes contain one sub-mesh, so you only
                // need 1 material. 
                materials: [
                    DefaultMaterial {
                        // We are using the DefaultMaterial which dynamically generates a shader
                        // based on what properties are set.  This means you don't need to write
                        // any shader code yourself.  In this case we just want the cube to have
                        // a red color.
                        id: cubeMaterial
                        diffuseColor: "red"
                    }
                ]
            }
        }
    }
}
```
