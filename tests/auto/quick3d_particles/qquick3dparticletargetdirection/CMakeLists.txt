
#####################################################################
## qquick3dparticletargetdirection Test:
#####################################################################

qt_internal_add_test(tst_qquick3dparticletargetdirection
    SOURCES
        tst_qquick3dparticletargetdirection.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3D
        Qt::Quick3DPrivate
        Qt::Quick3DParticlesPrivate
)
