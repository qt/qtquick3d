# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(manual_test_proceduralgeometry LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_standard_project_setup(REQUIRES 6.8)

qt_internal_add_manual_test(manual_test_proceduralgeometry
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
)

qt_add_qml_module(manual_test_proceduralgeometry
    URI io.qt.tests.manual.proceduralgeometry
    VERSION 1.0
    QML_FILES
        Main.qml
        OriginGizmo.qml
        ExtrudedTextSettings.qml
        TorusSettings.qml
        ConeSettings.qml
        CylinderSettings.qml
        SphereSettings.qml
        PlaneSettings.qml
        CuboidSettings.qml
        CapsuleSettings.qml
        UVChecker.qml
)
