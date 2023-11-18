# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(volumetric LANGUAGES CXX)
find_package(OpenMP)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/volumetric")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Quick3D)

qt_add_executable(volumetric
    main.cpp
    volumetexturedata.cpp
    volumetexturedata.h
    lineboxgeometry.cpp
    lineboxgeometry.h
)

set_target_properties(volumetric PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(volumetric PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(volumetric PUBLIC
        OpenMP::OpenMP_CXX
    )
endif()

qt_add_qml_module(volumetric
    URI VolumetricExample
    VERSION 1.0
    QML_FILES
        Main.qml
        ArcballController.qml
        OriginGizmo.qml
        Spinner.qml
    RESOURCES
        images/circle.png
        images/colormap-coolwarm.png
        images/colormap-gist_rainbow.png
        images/colormap-gnuplot.png
        images/colormap-plasma.png
        images/colormap-rainbow.png
        images/colormap-viridis.png
        alpha_blending.frag
        alpha_blending.vert
        SpacingMap.mjs
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS volumetric
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
