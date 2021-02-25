# Generated from sourceitem.pro.

#####################################################################
## sourceitem Binary:
#####################################################################

qt_internal_add_manual_test(sourceitem
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
        Qt::QuickControls2
)

# Resources:
set(qml_resource_files
    "main.qml"
    "qt_logo.png"
    "qt_logo_rect.png"
)

qt_internal_add_resource(sourceitem "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)


#### Keys ignored in scope 1:.:.:sourceitem.pro:<TRUE>:
# OTHER_FILES = "main.qml"
