# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qquick3dmesh Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquick3dmesh LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qquick3dmesh
    SOURCES
        tst_mesh.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick3DUtilsPrivate
)

#####################################################################
## tst_qssgmesh Test:
#####################################################################

file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qssgmesh
    SOURCES
        tst_qssgmesh.cpp
    LIBRARIES
        Qt::Quick3DUtilsPrivate
    TESTDATA ${test_data}
)
