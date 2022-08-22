# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


#####################################################################
## qquick3dparticletargetdirection Test:
#####################################################################

qt_internal_add_test(tst_qquick3dparticletargetdirection
    SOURCES
        tst_qquick3dparticletargetdirection.cpp
    LIBRARIES
        Qt::Quick3D
        Qt::Quick3DPrivate
        Qt::Quick3DParticlesPrivate
)
