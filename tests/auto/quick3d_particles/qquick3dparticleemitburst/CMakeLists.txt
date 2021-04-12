
#####################################################################
## qquick3dparticleemitburst Test:
#####################################################################

qt_internal_add_test(tst_qquick3dparticleemitburst
    SOURCES
        tst_qquick3dparticleemitburst.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3D
        Qt::Quick3DPrivate
        Qt::Quick3DParticlesPrivate
)
