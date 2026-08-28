# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(manual_test_modelblendparticles LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(manual_test_modelblendparticles
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
)

qt_add_qml_module(manual_test_modelblendparticles
    URI ModelBlendParticlesExample
    VERSION 1.0
    QML_FILES
        main.qml
        SettingsView.qml
        CustomSelectionBox.qml
        CustomSlider.qml
        CustomCheckBox.qml
    RESOURCES
        images/icon_pause.png
        images/icon_play.png
        images/icon_settings.png
    SOURCES
        testgeometry.h
    NO_RESOURCE_TARGET_PATH
    IMPORTS
        QtQuick3D
)

