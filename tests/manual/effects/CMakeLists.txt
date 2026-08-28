# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(manual_test_effects LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(manual_test_effects
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
)

qt_add_qml_module(manual_test_effects
    URI Example
    VERSION 1.0
    QML_FILES
        CustomEffect.qml
        EffectBox.qml
        EffectColor.qml
        EffectSlider.qml
        FileEffect.qml
        SettingsGroup.qml
        main.qml
    RESOURCES
        cork.jpg
        texture.png
        default.vert
        default.frag
    NO_RESOURCE_TARGET_PATH
)
