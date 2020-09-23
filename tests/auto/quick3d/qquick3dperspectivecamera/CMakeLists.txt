# Generated from qquick3dperspectivecamera.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## qquick3dperspectivecamera Test:
#####################################################################

qt_internal_add_test(qquick3dperspectivecamera
    SOURCES
        tst_qquick3dperspectivecamera.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3DPrivate
        Qt::Quick3DRuntimeRenderPrivate
)

#### Keys ignored in scope 1:.:.:qquick3dperspectivecamera.pro:<TRUE>:
# TEMPLATE = "app"
# _REQUIREMENTS = "qtConfig(private_tests)"
