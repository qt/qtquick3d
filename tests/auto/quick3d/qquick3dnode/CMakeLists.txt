# Generated from qquick3dnode.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## qquick3dnode Test:
#####################################################################

qt_internal_add_test(qquick3dnode
    SOURCES
        tst_qquick3dnode.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3DPrivate
        Qt::Quick3DRuntimeRenderPrivate
)

#### Keys ignored in scope 1:.:.:qquick3dnode.pro:<TRUE>:
# TEMPLATE = "app"
# _REQUIREMENTS = "qtConfig(private_tests)"
