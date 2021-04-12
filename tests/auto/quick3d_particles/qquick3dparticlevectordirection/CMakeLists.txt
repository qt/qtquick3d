
#####################################################################
## qquick3dparticlevectordirection Test:
#####################################################################

qt_internal_add_test(tst_qquick3dparticlevectordirection
    SOURCES
        tst_qquick3dparticlevectordirection.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3D
        Qt::Quick3DPrivate
        Qt::Quick3DParticlesPrivate
)
