
#####################################################################
## qquick3dparticle Test:
#####################################################################

qt_internal_add_test(tst_qquick3dparticle
    SOURCES
        tst_qquick3dparticle.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3D
        Qt::Quick3DPrivate
        Qt::Quick3DParticlesPrivate
)
