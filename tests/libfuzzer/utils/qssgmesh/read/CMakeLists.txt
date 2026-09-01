# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(read LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick3DUtilsPrivate)

qt_add_executable(read
    main.cpp
)

target_link_libraries(read PUBLIC
    Qt::Core
    Qt::Quick3DUtilsPrivate
)
if(DEFINED ENV{LIB_FUZZING_ENGINE})
    target_link_libraries(read PRIVATE
        $ENV{LIB_FUZZING_ENGINE}
    )
else()
    target_link_libraries(read PRIVATE
        -fsanitize=fuzzer
    )
endif()
