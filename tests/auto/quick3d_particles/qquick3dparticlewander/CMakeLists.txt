
#####################################################################
## qquick3dparticlewander Test:
#####################################################################

qt_internal_add_test(tst_qquick3dparticlewander
    SOURCES
        tst_qquick3dparticlewander.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3D
        Qt::Quick3DPrivate
        Qt::Quick3DParticlesPrivate
)
