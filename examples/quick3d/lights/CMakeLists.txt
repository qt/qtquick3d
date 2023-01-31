# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(lights LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/lights")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Quick3D)

qt_add_executable(lights
    main.cpp
)

set_target_properties(lights PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(lights PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)

qt_add_qml_module(lights
    URI Example
    VERSION 1.0
    QML_FILES
        CustomCheckBox.qml
        CustomSlider.qml
        RotatingTeaPot.qml
        main.qml
    RESOURCES
        custom.vert
        teapot.mesh
        white.png
        icon_settings.png
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS lights
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
