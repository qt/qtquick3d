# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qt_add_library(VirtualAssistant STATIC)
qt_add_qml_module(VirtualAssistant
    URI "Quick3DAssets.VirtualAssistant"
    VERSION 1.0
    QML_FILES
        VirtualAssistant.qml
    RESOURCES
        meshes/body.mesh
        meshes/mesh_107.mesh
        meshes/mesh_108.mesh
        meshes/mesh_109.mesh
)

target_link_libraries(VirtualAssistant
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Quick3D
        Qt${QT_VERSION_MAJOR}::Quick
        RobotHeartplugin
)

if(TARGET Qt${QT_VERSION_MAJOR}::QuickTimeline)
    target_link_libraries(VirtualAssistant PRIVATE Qt${QT_VERSION_MAJOR}::QuickTimeline)
endif()
