// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma Singleton
import QtQuick

QtObject {
    readonly property int width: 1280
    readonly property int height: 720

    readonly property vector3d defaultCameraPosition: Qt.vector3d(0, 5, 15)
    readonly property int defaultFov: 80
    readonly property int defaultRotation: 180
    readonly property string sceneName: "colosseum_4k.hdr"
}
