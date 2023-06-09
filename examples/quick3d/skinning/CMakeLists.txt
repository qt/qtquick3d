# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(skinning LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/skinning")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Quick3D)

qt_add_executable(skinning
    main.cpp
)

set_target_properties(skinning PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(skinning PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)

qt_add_qml_module(skinning
    URI Example
    VERSION 1.0
    QML_FILES
        main.qml
        SimpleSkinning.qml
        SimpleSkinningNew.qml
    SOURCES
        skingeometry.cpp skingeometry.h
    NO_RESOURCE_TARGET_PATH
    IMPORTS
        QtQuick3D
)

install(TARGETS skinning
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
