# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(manual_test_lightmap_concept LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(manual_test_lightmap_concept
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
)

set(qml_resource_files
    "main.qml"
    "lightmapgen.vert"
    "lightmapgen.frag"
    "lightmapuse.vert"
    "lightmapuse.frag"
    "hskin.png"
    "meshes/cube.mesh"
    "meshes/sphere.mesh"
    "meshes/torus.mesh"
    "meshes/suzanne.mesh"
    "meshes/animal.mesh"
)

qt_internal_add_resource(manual_test_lightmap_concept "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
