
#####################################################################
## qquick3dparticlespriteparticle Test:
#####################################################################

qt_internal_add_test(qquick3dparticlespriteparticle
    SOURCES
        tst_qquick3dparticlespriteparticle.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3D
        Qt::Quick3DPrivate
        Qt::Quick3DParticlesPrivate
)
