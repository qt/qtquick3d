
#####################################################################
## qquick3dparticleemitter Test:
#####################################################################

qt_internal_add_test(tst_qquick3dparticleemitter
    SOURCES
        tst_qquick3dparticleemitter.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3D
        Qt::Quick3DPrivate
        Qt::Quick3DParticlesPrivate
)
