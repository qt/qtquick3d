
#####################################################################
## qquick3dparticlesystem Test:
#####################################################################

qt_internal_add_test(tst_qquick3dparticlesystem
    SOURCES
        tst_qquick3dparticlesystem.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3D
        Qt::Quick3DPrivate
        Qt::Quick3DParticlesPrivate
)
