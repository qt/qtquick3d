# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qt_add_library(RobotHeart STATIC)
qt_add_qml_module(RobotHeart
    URI "Quick3DAssets.RobotHeart"
    VERSION 1.0
    QML_FILES
        RobotHeart.qml
    RESOURCES
        meshes/plane.mesh
)

target_link_libraries(RobotHeart PRIVATE Qt${QT_VERSION_MAJOR}::Quick3D)

if(TARGET Qt${QT_VERSION_MAJOR}::QuickTimeline)
    target_link_libraries(RobotHeart PRIVATE Qt${QT_VERSION_MAJOR}::QuickTimeline)
endif()
