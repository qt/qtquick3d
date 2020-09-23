# Generated from stereoscopic.pro.

#####################################################################
## stereoscopic Binary:
#####################################################################

qt_internal_add_manual_test(stereoscopic
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
    "SceneNode.qml"
    "anaglyph.frag.qsb"
    "main.qml"
)

qt_internal_add_resource(stereoscopic "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)


#### Keys ignored in scope 1:.:.:stereoscopic.pro:<TRUE>:
# OTHER_FILES = "main.qml" "SceneNode.qml"
