# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## exr Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qssgexrreader LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

# Deliberately no Qt::Gui: the reader parses bytes into a float buffer and
# should never need more than QtCore.
qt_internal_add_test(tst_qssgexrreader
    SOURCES
        tst_qssgexrreader.cpp
    LIBRARIES
        Qt::Quick3DTextureHelpersPrivate
    TESTDATA ${test_data}
)
