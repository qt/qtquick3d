# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#####################################################################
## qquick3dparticlegravity Test:
#####################################################################

qt_internal_add_test(tst_qquick3dparticlegravity
    SOURCES
        tst_qquick3dparticlegravity.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3D
        Qt::Quick3DPrivate
        Qt::Quick3DParticlesPrivate
)
