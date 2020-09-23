# Generated from qquick3dorthographiccamera.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## qquick3dorthographiccamera Test:
#####################################################################

qt_internal_add_test(qquick3dorthographiccamera
    SOURCES
        tst_qquick3dorthographiccamera.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3DPrivate
        Qt::Quick3DRuntimeRenderPrivate
)

#### Keys ignored in scope 1:.:.:qquick3dorthographiccamera.pro:<TRUE>:
# TEMPLATE = "app"
# _REQUIREMENTS = "qtConfig(private_tests)"
