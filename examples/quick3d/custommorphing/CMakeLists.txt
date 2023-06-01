# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(custommorphing LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/custommorphing")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Quick3D)

qt_add_executable(custommorphing
    main.cpp
)

set_target_properties(custommorphing PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(custommorphing PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)

qt_add_qml_module(custommorphing
    URI Example
    VERSION 1.0
    QML_FILES
        main.qml
    SOURCES
        morphgeometry.cpp morphgeometry.h
    NO_RESOURCE_TARGET_PATH
    IMPORTS
        QtQuick3D
)

install(TARGETS custommorphing
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
