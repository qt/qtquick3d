# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from shadercollection.pro.

if(NOT TARGET Qt::ShaderTools)
    return()
endif()

#####################################################################
## shadercollection Test:
#####################################################################

qt_internal_add_test(tst_qquick3dshadercollection
    SOURCES
        tst_shadercollection.cpp
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Quick3DUtilsPrivate
        Qt::ShaderToolsPrivate
)

#### Keys ignored in scope 1:.:.:shadercollection.pro:<TRUE>:
# TEMPLATE = "app"
# _REQUIREMENTS = "qtHaveModule(shadertools)"
