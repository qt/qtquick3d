# Generated from ssao.pro.

#####################################################################
## ssao Binary:
#####################################################################

qt_add_manual_test(ssao
    GUI
    SOURCES
        ssao.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
)

# Resources:
set(ssao_resource_files
    "object1.mesh"
    "ssao.qml"
)

qt_add_resource(ssao "ssao"
    PREFIX
        "/"
    FILES
        ${ssao_resource_files}
)


#### Keys ignored in scope 1:.:.:ssao.pro:<TRUE>:
# OTHER_FILES = "ssao.qml"
