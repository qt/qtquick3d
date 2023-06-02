# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(helloqtquick3d LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/helloqtquick3d")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Quick3D)

qt_add_executable(helloqtquick3d
    imageinstancetable.cpp imageinstancetable.h
    main.cpp
)

set_target_properties(helloqtquick3d PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(helloqtquick3d PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)

qt_add_qml_module(helloqtquick3d
    URI HelloExample
    VERSION 1.0
    QML_FILES
        main.qml
    RESOURCES
        oldqtlogo.mesh
        qt_logo.png
    NO_RESOURCE_TARGET_PATH
    IMPORTS
        QtQuick3D
)

install(TARGETS helloqtquick3d
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
