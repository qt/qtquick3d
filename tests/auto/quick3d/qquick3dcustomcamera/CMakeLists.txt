# Generated from qquick3dcustomcamera.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## qquick3dcustomcamera Test:
#####################################################################

qt_add_test(qquick3dcustomcamera
    SOURCES
        tst_qquick3dcustomcamera.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3DPrivate
        Qt::Quick3DRuntimeRenderPrivate
)

#### Keys ignored in scope 1:.:.:qquick3dcustomcamera.pro:<TRUE>:
# TEMPLATE = "app"
# _REQUIREMENTS = "qtConfig(private_tests)"
