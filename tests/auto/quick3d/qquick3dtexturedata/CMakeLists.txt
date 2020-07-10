if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## qquick3dtexturedata Test:
#####################################################################

qt_add_test(qquick3dtexturedata
    SOURCES
        tst_qquick3dtexturedata.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3DPrivate
        Qt::Quick3DRuntimeRenderPrivate
)
