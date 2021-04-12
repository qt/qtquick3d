
#####################################################################
## qquick3dparticlemodelparticle Test:
#####################################################################

qt_internal_add_test(tst_qquick3dparticlemodelparticle
    SOURCES
        tst_qquick3dparticlemodelparticle.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3D
        Qt::Quick3DPrivate
        Qt::Quick3DParticlesPrivate
)
