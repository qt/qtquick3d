// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// Doc image: what each XrView reference space makes the scene origin mean.
// The origin stays at the render origin in all three panels; the floor and the
// user move, which is precisely the effect of choosing a reference space.
Diagram {
    id: win
    width: 1290
    height: 430
    imageName: "qt3dxr-reference-spaces"
    grabTarget: root

    Row {
        id: root
        width: win.width
        height: win.height
        spacing: 0

        RefSpacePanel {
            width: win.width / 3
            height: win.height
            tick: win.tick
            title: "ReferenceSpaceLocal"
            subtitle: "origin at the default view position"
            footnote: "A model at y = 0 floats at eye height."
            floorY: -163
            showEyeLabel: false
            originDx: -56
            userX: -60
        }

        Rectangle { width: 1; height: win.height; color: "#e2e6ea" }

        RefSpacePanel {
            width: win.width / 3 - 2
            height: win.height
            tick: win.tick
            title: "ReferenceSpaceLocalFloor"
            subtitle: "origin at floor height, below the view"
            footnote: "A model at y = 0 rests on the floor."
            floorY: 0
        }

        Rectangle { width: 1; height: win.height; color: "#e2e6ea" }

        RefSpacePanel {
            width: win.width / 3
            height: win.height
            tick: win.tick
            title: "ReferenceSpaceStage"
            subtitle: "origin at the center of the play area"
            footnote: "The origin is fixed; the user may move within the area."
            floorY: 0
            userX: -95
            showPlayArea: true
        }
    }
}
