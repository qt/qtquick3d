# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(manual_test_stenciloutline
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
        Qt::GuiPrivate
        Qt::Quick3DPrivate
)

# Resources:
set(qml_resource_files
    "main.qml"
    "ColorPicker.qml"
    "SectionLayout.qml"
    "images/"
    "models/"
)

set(outlineextension_uri "QtQuick3DTest.OutlineRenderExtension")

qt_internal_add_qml_module(manual_test_stenciloutline
    VERSION 1.0
    URI ${outlineextension_uri}
    SOURCES
        outlinerenderextension.cpp outlinerenderextension.h
    NO_PLUGIN
)

qt_add_shaders(manual_test_stenciloutline "baked-shaders"
    PREFIX
        "/"
    FILES
        "shaders/huesaturation.frag"
)

qt_internal_add_resource(manual_test_stenciloutline "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
