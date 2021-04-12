
#####################################################################
## qquick3dparticleshape Test:
#####################################################################

qt_internal_add_test(tst_qquick3dparticleshape
    SOURCES
        tst_qquick3dparticleshape.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3D
        Qt::Quick3DPrivate
        Qt::Quick3DParticlesPrivate
)
