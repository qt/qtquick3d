# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(manual_test_vertexAttributes LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(manual_test_vertexAttributes
    GUI
    SOURCES
        main.cpp
        testgeometry.cpp testgeometry.h
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
)

qt_internal_add_resource(manual_test_vertexAttributes "qml"
    PREFIX
        "/"
    FILES
        main.qml
        Exhibit.qml
)

qt_internal_add_resource(manual_test_vertexAttributes "maps"
    PREFIX
        "/"
    FILES
        maps/basemetal_small.png
        maps/normalrough_small.png
)
