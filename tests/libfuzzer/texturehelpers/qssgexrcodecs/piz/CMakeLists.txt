# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(piz LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick3DTextureHelpersPrivate)

qt_add_executable(piz
    main.cpp
)

target_link_libraries(piz PUBLIC
    Qt::Core
    Qt::Quick3DTextureHelpersPrivate
)
if(DEFINED ENV{LIB_FUZZING_ENGINE})
    target_link_libraries(piz PRIVATE
        $ENV{LIB_FUZZING_ENGINE}
    )
else()
    target_link_libraries(piz PRIVATE
        -fsanitize=fuzzer
    )
endif()
