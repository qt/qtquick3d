# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquick3dshadercache LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## shadercache Test:
#####################################################################

qt_internal_add_test(tst_qquick3dshadercache
    SOURCES
        tst_shadercache.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Quick3DRuntimeRenderPrivate
)

# Mirrors how the module itself is built: without QtShaderTools there is no
# baker, so the baking half of the test skips.
qt_internal_extend_target(tst_qquick3dshadercache CONDITION TARGET Qt::ShaderTools
    LIBRARIES
        Qt::ShaderTools
        Qt::ShaderToolsPrivate
    DEFINES
        QT_QUICK3D_HAS_RUNTIME_SHADERS
)
