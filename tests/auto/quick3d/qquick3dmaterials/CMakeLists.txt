# Generated from qquick3dmaterials.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## qquick3dmaterials Test:
#####################################################################

qt_add_test(qquick3dmaterials
    SOURCES
        tst_qquick3dmaterials.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3DPrivate
        Qt::Quick3DRuntimeRenderPrivate
)

# Resources:
set(qquick3dmaterials_resource_files
    "Built_with_Qt.png"
    "Built_with_Qt_2.png"
)

qt_add_resource(qquick3dmaterials "qquick3dmaterials"
    PREFIX
        "/"
    FILES
        ${qquick3dmaterials_resource_files}
)


#### Keys ignored in scope 1:.:.:qquick3dmaterials.pro:<TRUE>:
# TEMPLATE = "app"
# _REQUIREMENTS = "qtConfig(private_tests)"
