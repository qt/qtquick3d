# Generated from qquick3dtexture.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## qquick3dtexture Test:
#####################################################################

qt_add_test(qquick3dtexture
    SOURCES
        tst_qquick3dtexture.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3DPrivate
        Qt::Quick3DRuntimeRenderPrivate
)

#### Keys ignored in scope 1:.:.:qquick3dtexture.pro:<TRUE>:
# TEMPLATE = "app"
# _REQUIREMENTS = "qtConfig(private_tests)"
