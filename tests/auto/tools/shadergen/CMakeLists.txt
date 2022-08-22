# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from shadergen.pro.

#####################################################################
## shadergen Test:
#####################################################################

qt_internal_add_test(tst_qquick3dshadergen # special case
    SOURCES
        ../../../../tools/shadergen/parser.cpp ../../../../tools/shadergen/parser.h
        tst_shadergen.cpp
    INCLUDE_DIRECTORIES
        ../../../../tools/shadergen
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::Quick3DPrivate
        Qt::Quick3DRuntimeRenderPrivate
)

# Resources:
set(qml_resource_files
    "qml/CustomMaterialA.qml"
    "qml/DefaultMaterialA.qml"
    "qml/PrincipledMaterialA.qml"
    "qml/EffectA.qml"
    "qml/ModelA.qml"
    "qml/main.qml"
    "qml/instancing.qml"
)

qt_internal_add_resource(tst_qquick3dshadergen "qml" # special case
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)


#### Keys ignored in scope 1:.:.:shadergen.pro:<TRUE>:
# TEMPLATE = "app"
