# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(runtimeloader LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/runtimeloader")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Quick3D Widgets)

qt_add_executable(runtimeloader
    main.cpp
)

set_target_properties(runtimeloader PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(runtimeloader PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)

if(TARGET Qt::Widgets)
    target_compile_definitions(runtimeloader PUBLIC
        HAS_MODULE_QT_WIDGETS
    )

    target_link_libraries(runtimeloader PUBLIC
        Qt::Widgets
    )
endif()

qt_add_qml_module(runtimeloader
    URI Example
    VERSION 1.0
    QML_FILES main.qml
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS runtimeloader
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
