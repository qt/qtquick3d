# Generated from offscreenrendermode.pro.

#####################################################################
## offscreenrendermode Binary:
#####################################################################

qt_internal_add_manual_test(offscreenrendermode
    GUI
    SOURCES
        offscreenrendermode.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3DPrivate
)

# Resources:
set_source_files_properties("../shared/teapot.mesh"
    PROPERTIES QT_RESOURCE_ALIAS "teapot.mesh"
)
set(offscreenrendermode_resource_files
    "../shared/teapot.mesh"
    "SomePointLight.qml"
    "offscreenrendermode.qml"
    "qt_logo.png"
)

qt_internal_add_resource(offscreenrendermode "offscreenrendermode"
    PREFIX
        "/"
    FILES
        ${offscreenrendermode_resource_files}
)


#### Keys ignored in scope 1:.:.:offscreenrendermode.pro:<TRUE>:
# OTHER_FILES = "offscreenrendermode.qml" "SomePointLight.qml"
