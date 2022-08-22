# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause



#### Inputs

# input quick3d-assimp
set(INPUT_quick3d_assimp "undefined" CACHE STRING "")
set_property(CACHE INPUT_quick3d_assimp PROPERTY STRINGS undefined no qt system)



#### Libraries

qt_find_package(WrapQuick3DAssimp 5.1.6 PROVIDED_TARGETS WrapQuick3DAssimp::WrapQuick3DAssimp MODULE_NAME assetimporters QMAKE_LIB quick3d_assimp)

qt_config_compile_test("quick3d_assimp"
                   LABEL "Assimp"
                   PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../config.tests/quick3d_assimp"
                   LIBRARIES WrapQuick3DAssimp::WrapQuick3DAssimp
                   PACKAGES PACKAGE WrapQuick3DAssimp 5.1.6)


#### Tests



#### Features

qt_feature("quick3d-assimp" PUBLIC PRIVATE
    LABEL "Assimp"
)
qt_feature_definition("quick3d-assimp" "QT_NO_QUICK3D_ASSIMP" NEGATE VALUE "1")
qt_feature("system-assimp" PRIVATE
    LABEL "System Assimp"
    CONDITION QT_FEATURE_quick3d_assimp AND TEST_quick3d_assimp
    ENABLE INPUT_quick3d_assimp STREQUAL 'system'
    DISABLE INPUT_quick3d_assimp STREQUAL 'qt'
)
qt_configure_add_summary_section(NAME "QtQuick3D")
qt_configure_add_summary_entry(ARGS "quick3d-assimp")
qt_configure_add_summary_entry(ARGS "system-assimp")
qt_configure_end_summary_section() # end of "QtQuick3D" section
