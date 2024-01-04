# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_find_package(WrapOpenXR PROVIDED_TARGETS WrapOpenXR::WrapOpenXR MODULE_NAME quick3dxr QMAKE_LIB quick3dxr_openxr)

qt_config_compile_test("quick3dxr_openxr"
                   LABEL "Quick3D OpenXR"
                   PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../config.tests/quick3dxr_openxr"
                   LIBRARIES WrapOpenXR::WrapOpenXR
                   PACKAGES PACKAGE WrapOpenXR)

qt_feature("quick3dxr-openxr" PUBLIC PRIVATE
    LABEL "OpenXR"
    CONDITION TEST_quick3dxr_openxr
)

qt_configure_add_summary_section(NAME "QtQuick3D XR")
qt_configure_add_summary_entry(ARGS "quick3dxr-openxr")
qt_configure_end_summary_section()
