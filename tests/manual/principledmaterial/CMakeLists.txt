# Generated from principledmaterial.pro.

#####################################################################
## principledmaterial Binary:
#####################################################################

qt_internal_add_manual_test(principledmaterial
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3DPrivate
)

# Resources:
set(qml_resource_files
    "MaterialControl.qml"
    "main.qml"
)

qt_internal_add_resource(principledmaterial "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
set(materials_resource_files
    "maps/OpenfootageNET_garage-1024.hdr"
    "maps/metallic/basemetal.astc"
    "maps/metallic/normalrough.astc"
)

qt_internal_add_resource(principledmaterial "materials"
    PREFIX
        "/"
    FILES
        ${materials_resource_files}
)


#### Keys ignored in scope 1:.:.:principledmaterial.pro:<TRUE>:
# INSTALLS = "target"
# OTHER_FILES = "doc/src/*.*"
# target.path = "$$[QT_INSTALL_EXAMPLES]/quick3d/principledmaterial"
