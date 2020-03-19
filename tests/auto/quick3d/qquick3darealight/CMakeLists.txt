# Generated from qquick3darealight.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## qquick3darealight Test:
#####################################################################

qt_add_test(qquick3darealight
    SOURCES
        tst_qquick3darealight.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3DPrivate
        Qt::Quick3DRuntimeRenderPrivate
)

#### Keys ignored in scope 1:.:.:qquick3darealight.pro:<TRUE>:
# TEMPLATE = "app"
# _REQUIREMENTS = "qtConfig(private_tests)"
