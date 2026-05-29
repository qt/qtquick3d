# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(VolumetricFog VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Quick3D)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(volumetricfog
    main.cpp
)

qt_add_qml_module(volumetricfog
    URI VF
    VERSION 1.0
    QML_FILES
        Main.qml
        SettingsPane.qml
        Room.qml
    RESOURCES
        qtquickcontrols2.conf
        assets/l0.ies
        assets/l1.ies
        assets/l2.ies
        assets/l3.ies
        assets/l4.ies
        assets/l5.ies
        assets/tree.mesh
        assets/sliders.svg
    NO_RESOURCE_TARGET_PATH
)

add_subdirectory(VFExtension)

target_link_libraries(volumetricfog
    PRIVATE
    VFExtension
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Quick3D
)


set_target_properties(volumetricfog PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS volumetricfog
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
