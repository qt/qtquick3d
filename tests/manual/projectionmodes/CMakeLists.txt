# Generated from projectionmodes.pro.

#####################################################################
## projectionmodes Binary:
#####################################################################

qt_internal_add_manual_test(projectionmodes
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

qt_internal_add_resource(projectionmodes "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)


#### Keys ignored in scope 1:.:.:projectionmodes.pro:<TRUE>:
# OTHER_FILES = "main.qml"
