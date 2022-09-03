
#####################################################################
## qquick3dparticlespriteparticle Test:
#####################################################################

qt_internal_add_test(tst_qquick3dparticlelineparticle
    SOURCES
        tst_qquick3dparticlelineparticle.cpp
    LIBRARIES
        Qt::Quick3D
        Qt::Quick3DPrivate
        Qt::Quick3DParticlesPrivate
)
