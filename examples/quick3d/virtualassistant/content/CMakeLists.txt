# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qt_add_library(robotassistant STATIC)

qt_add_qml_module(robotassistant
    URI "robotassistant"
    VERSION 1.0
    QML_FILES
        App.qml
        ControlPanel.qml
        Screen01.ui.qml
        SettingsPanel.qml
    RESOURCES
        colosseum_4k.hdr
)

target_link_libraries(robotassistant
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Quick
        Qt${QT_VERSION_MAJOR}::Quick3D
        Qt${QT_VERSION_MAJOR}::Quick3DHelpers
        Constantsplugin
        VirtualAssistantplugin
)

if(TARGET Qt${QT_VERSION_MAJOR}::QuickControls2)
    target_link_libraries(robotassistant PRIVATE Qt${QT_VERSION_MAJOR}::QuickControls2)
endif()

if(TARGET Qt${QT_VERSION_MAJOR}::QuickLayouts)
    target_link_libraries(robotassistant PRIVATE Qt${QT_VERSION_MAJOR}::QuickLayouts)
endif()
