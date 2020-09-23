# Generated from cameralookat.pro.

#####################################################################
## cameralookat Binary:
#####################################################################

qt_internal_add_manual_test(cameralookat
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

qt_internal_add_resource(cameralookat "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)


#### Keys ignored in scope 1:.:.:cameralookat.pro:<TRUE>:
# OTHER_FILES = "main.qml"
