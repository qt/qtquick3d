# Generated from shadowCasting.pro.

#####################################################################
## shadowCasting Binary:
#####################################################################

qt_internal_add_manual_test(shadowCasting
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

qt_internal_add_resource(shadowCasting "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)


#### Keys ignored in scope 1:.:.:shadowCasting.pro:<TRUE>:
# OTHER_FILES = "main.qml"
