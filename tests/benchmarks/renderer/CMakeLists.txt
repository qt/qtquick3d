# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from renderer.pro.

#####################################################################
## renderer Test:
#####################################################################

qt_internal_add_test(tst_qquick3drenderer
    SOURCES
        tst_renderer.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3DPrivate
        Qt::Quick3DRuntimeRenderPrivate
)

#### Keys ignored in scope 1:.:.:renderer.pro:<TRUE>:
# TEMPLATE = "app"
