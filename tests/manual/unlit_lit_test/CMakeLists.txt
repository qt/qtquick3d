# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(manual_test_unlit_lit LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(manual_test_unlit_lit
    GUI
    SOURCES
        rhitest.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3DPrivate
)

# Resources:
set_source_files_properties("../shared/teapot.mesh"
    PROPERTIES QT_RESOURCE_ALIAS "teapot.mesh"
)
set(rhitest_resource_files
    "../shared/teapot.mesh"
    "SomePointLight.qml"
    "qt_logo.png"
    "rhitest.qml"
)

qt_internal_add_resource(manual_test_unlit_lit "rhitest"
    PREFIX
        "/"
    FILES
        ${rhitest_resource_files}
)
