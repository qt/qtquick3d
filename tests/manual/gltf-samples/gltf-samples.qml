// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Iterates a checkout of the Khronos glTF-Sample-Assets repository through
// the RuntimeLoader, logging load errors per asset. Run with:
//
//   qml gltf-samples.qml -- /path/to/glTF-Sample-Assets/Models
//
// Set QT_QUICK3D_DISABLE_NATIVE_GLTF to load through the Assimp importer.
// Use the left/right keys to step through the models manually, or press
// space to run through all of them automatically.

import QtQuick
import QtQuick3D
import QtQuick3D.Helpers
import QtQuick3D.AssetUtils
import Qt.labs.folderlistmodel

Window {
    id: window
    width: 1024
    height: 768
    visible: true
    title: loader.source + " — " + (loader.status === RuntimeLoader.Error ? loader.errorString : "OK")

    property int currentIndex: 0
    property var modelDirs: []

    FolderListModel {
        id: folders
        folder: "file://" + Qt.application.arguments[Qt.application.arguments.length - 1]
        showDirs: true
        showFiles: false
        onCountChanged: {
            let dirs = []
            for (let i = 0; i < count; ++i)
                dirs.push(get(i, "filePath"))
            window.modelDirs = dirs
        }
    }

    function sourceForIndex(index) {
        if (index < 0 || index >= modelDirs.length)
            return ""
        const dir = modelDirs[index]
        const name = dir.split("/").pop()
        // Only the plain glTF variant is loaded
        return "file://" + dir + "/glTF/" + name + ".gltf"
    }

    View3D {
        anchors.fill: parent
        environment: SceneEnvironment {
            clearColor: "dimgray"
            backgroundMode: SceneEnvironment.Color
            lightProbe: Texture { textureData: ProceduralSkyTextureData {} }
        }

        PerspectiveCamera { id: camera }

        RuntimeLoader {
            id: loader
            source: window.sourceForIndex(window.currentIndex)
            onBoundsChanged: {
                // Frame the model
                const c = Qt.vector3d((bounds.minimum.x + bounds.maximum.x) / 2,
                                      (bounds.minimum.y + bounds.maximum.y) / 2,
                                      (bounds.minimum.z + bounds.maximum.z) / 2)
                const ext = Math.max(bounds.maximum.x - bounds.minimum.x,
                                     bounds.maximum.y - bounds.minimum.y,
                                     bounds.maximum.z - bounds.minimum.z)
                camera.position = Qt.vector3d(c.x, c.y, c.z + ext * 2)
                camera.clipNear = ext / 100
                camera.clipFar = ext * 10
            }
            onStatusChanged: {
                if (status === RuntimeLoader.Error)
                    console.warn("FAILED:", source, "-", errorString)
                else if (status === RuntimeLoader.Success)
                    console.log("OK:", source)
            }
        }
    }

    Timer {
        id: autoAdvance
        interval: 1000
        repeat: true
        onTriggered: {
            if (window.currentIndex + 1 >= window.modelDirs.length) {
                running = false
                console.log("Done:", window.modelDirs.length, "models")
                return
            }
            window.currentIndex++
        }
    }

    Item {
        focus: true
        Keys.onLeftPressed: window.currentIndex = Math.max(0, window.currentIndex - 1)
        Keys.onRightPressed: window.currentIndex = Math.min(window.modelDirs.length - 1, window.currentIndex + 1)
        Keys.onSpacePressed: autoAdvance.running = !autoAdvance.running
    }

    Text {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 8
        color: "white"
        text: (window.currentIndex + 1) + " / " + window.modelDirs.length + "\n" + loader.source
              + (loader.status === RuntimeLoader.Error ? "\nERROR: " + loader.errorString : "")
    }
}
