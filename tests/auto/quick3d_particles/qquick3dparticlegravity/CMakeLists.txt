
#####################################################################
## qquick3dparticlegravity Test:
#####################################################################

qt_internal_add_test(tst_qquick3dparticlegravity
    SOURCES
        tst_qquick3dparticlegravity.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3D
        Qt::Quick3DPrivate
        Qt::Quick3DParticlesPrivate
)
