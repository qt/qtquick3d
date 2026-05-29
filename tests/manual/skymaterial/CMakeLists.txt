# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(manual_test_skymaterial
    GUI
    SOURCES
        manual_test_skymaterial.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
        Qt::GuiPrivate
        Qt::Quick3DPrivate
)

# Resources:
set(skymaterial_resource_files
    "advancedsky.frag"
    "AdvancedSky.qml"
    "basicsky.frag"
    "BasicSky.qml"
    "ManualTestSkyMaterial.qml"
    "skytexture.frag"
    "transmittance.frag"
)

qt_internal_add_resource(manual_test_skymaterial "skymaterial"
    PREFIX
        "/"
    FILES
        ${skymaterial_resource_files}
)
