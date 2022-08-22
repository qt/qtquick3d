# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(quickball LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/quickball")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Quick3D)

qt_add_executable(quickball
    main.cpp
)

set_target_properties(quickball PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(quickball PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)

qt_add_qml_module(quickball
    URI Example
    VERSION 1.0
    QML_FILES
        main.qml
    RESOURCES
        images/ball.jpg
        images/ball_icon.png
        images/ball_n.jpg
        images/grass.jpg
        images/grass_n.jpg
        images/particle.png
        images/qt_logo.jpg
        images/qt_logo_n.jpg
        images/quickball.png
        images/sky.jpg
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS quickball
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
