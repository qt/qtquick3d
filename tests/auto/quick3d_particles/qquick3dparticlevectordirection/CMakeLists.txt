# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


#####################################################################
## qquick3dparticlevectordirection Test:
#####################################################################

qt_internal_add_test(tst_qquick3dparticlevectordirection
    SOURCES
        tst_qquick3dparticlevectordirection.cpp
    LIBRARIES
        Qt::Quick3D
        Qt::Quick3DPrivate
        Qt::Quick3DParticlesPrivate
)
