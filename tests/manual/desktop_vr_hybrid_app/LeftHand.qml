// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D
import QtQuick3D.Xr

XrController {
    handInput.poseSpace: XrHandInput.AimPose
    controller: XrController.ControllerLeft
    Lazer {
        enableBeam: true
    }
}
