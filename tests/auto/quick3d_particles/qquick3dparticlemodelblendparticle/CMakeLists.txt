
#####################################################################
## qquick3dparticlemodelblendparticle Test:
#####################################################################

qt_internal_add_test(tst_qquick3dparticlemodelblendparticle
    SOURCES
        tst_qquick3dparticlemodelblendparticle.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3D
        Qt::Quick3DPrivate
        Qt::Quick3DParticlesPrivate
)
