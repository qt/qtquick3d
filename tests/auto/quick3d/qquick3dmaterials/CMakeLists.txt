# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qquick3dmaterials.pro.

#####################################################################
## qquick3dmaterials Test:
#####################################################################

qt_internal_add_test(tst_qquick3dmaterials
    SOURCES
        tst_qquick3dmaterials.cpp
    LIBRARIES
        Qt::Quick3DPrivate
        Qt::Quick3DRuntimeRenderPrivate
)

# Resources:
set(qquick3dmaterials_resource_files
    "Built_with_Qt.png"
    "Built_with_Qt_2.png"
)

qt_internal_add_resource(tst_qquick3dmaterials "qquick3dmaterials"
    PREFIX
        "/"
    FILES
        ${qquick3dmaterials_resource_files}
)


#### Keys ignored in scope 1:.:.:qquick3dmaterials.pro:<TRUE>:
# TEMPLATE = "app"
