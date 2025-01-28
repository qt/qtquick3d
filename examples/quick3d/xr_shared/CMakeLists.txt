# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qt_add_qml_module(${PROJECT_NAME}_shared
    URI xr_shared
    VERSION 1.0
    PLUGIN_TARGET ${PROJECT_NAME}_shared
    QML_FILES
        AimController.qml
        AxisGadget.qml
        TeleportBeam.qml
        FreeformTeleporter.qml
        PickRay.qml
        ResizeGadget.qml
        RotateGadget.qml
        TeleportTargetIndicator.qml
        TranslateGadget.qml
        ValueFader.qml
        XrCursor.qml
        XrGadget.qml
        XrItemHandle.qml
    RESOURCES
        shaders/target_indicator.frag
        shaders/target_indicator.vert
        shaders/cursor.frag
        shaders/cursor.vert
    RESOURCE_PREFIX /
    STATIC
)
