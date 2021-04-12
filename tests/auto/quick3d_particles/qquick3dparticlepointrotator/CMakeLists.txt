
#####################################################################
## qquick3dparticlepointrotator Test:
#####################################################################

qt_internal_add_test(tst_qquick3dparticlepointrotator
    SOURCES
        tst_qquick3dparticlepointrotator.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3D
        Qt::Quick3DPrivate
        Qt::Quick3DParticlesPrivate
)
