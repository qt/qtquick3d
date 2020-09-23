# Generated from qquick3dfrustumcamera.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## qquick3dfrustumcamera Test:
#####################################################################

qt_internal_add_test(qquick3dfrustumcamera
    SOURCES
        tst_qquick3dfrustumcamera.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3DPrivate
        Qt::Quick3DRuntimeRenderPrivate
)

#### Keys ignored in scope 1:.:.:qquick3dfrustumcamera.pro:<TRUE>:
# TEMPLATE = "app"
# _REQUIREMENTS = "qtConfig(private_tests)"
