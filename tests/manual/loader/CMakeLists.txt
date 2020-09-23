# Generated from loader.pro.

#####################################################################
## loader Binary:
#####################################################################

qt_internal_add_manual_test(loader
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
    "TestModel.qml"
    "main.qml"
)

qt_internal_add_resource(loader "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)


#### Keys ignored in scope 1:.:.:loader.pro:<TRUE>:
# OTHER_FILES = "main.qml" "TestModel.qml"
