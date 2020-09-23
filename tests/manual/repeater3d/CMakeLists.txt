# Generated from repeater3d.pro.

#####################################################################
## repeater3d Binary:
#####################################################################

qt_internal_add_manual_test(repeater3d
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
)

# Resources:
set(qml_resource_files
    "main.qml"
)

qt_internal_add_resource(repeater3d "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)


#### Keys ignored in scope 1:.:.:repeater3d.pro:<TRUE>:
# OTHER_FILES = "main.qml"
