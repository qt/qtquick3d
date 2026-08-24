// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

/*
    Shared chrome for the generated diagrams on the "Coordinate Systems and
    Units in Qt Quick 3D Xr" page. Each diagram is a real Qt Quick 3D scene
    built from real geometry at real coordinates, so the pictures cannot drift
    away from the numbers in the prose.

    Regenerating an image takes two steps, because Qt only writes WebP when the
    qtimageformats plugin is present. First render the PNG next to this file:

        qml qt3dxr-coordinate-systems.qml

    Then convert it to the WebP that the documentation includes. Each scene
    names its own width and quality, chosen to keep the file under 50 KB while
    leaving the annotations legible once QDoc scales the image into the text
    column. The intermediate PNGs are ignored by git.
*/
Window {
    id: win
    visible: true
    color: "white"

    // Base name of the image, without an extension, matching this scene.
    required property string imageName
    required property Item grabTarget

    // mapFrom3DScene() is a plain method call, so a binding on it has nothing
    // to depend on and would keep whatever it computed before the camera was
    // live. Scenes reference this counter inside those bindings; bumping it
    // once the scene has settled re-evaluates every label position.
    property int tick: 0

    Timer {
        interval: 700
        running: true
        onTriggered: win.tick++
    }

    Timer {
        interval: 1500
        running: true
        onTriggered: {
            const out = Qt.resolvedUrl(win.imageName + ".png")
                            .toString().replace(/^file:\/{3}/, "")
            // On a high-DPI screen this grabs more pixels than the window's
            // logical size, which is welcome: the conversion step scales the
            // result down, and the extra samples sharpen the text.
            win.grabTarget.grabToImage(function(result) {
                if (!result.saveToFile(out)) {
                    console.error("could not write", out)
                    Qt.exit(1)
                }
                console.log("wrote", out)
                Qt.exit(0)
            })
        }
    }
}
