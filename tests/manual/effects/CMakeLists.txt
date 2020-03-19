# Generated from effects.pro.

#####################################################################
## effects Binary:
#####################################################################

qt_add_manual_test(effects
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
    "CustomEffect.qml"
    "EffectBox.qml"
    "EffectColor.qml"
    "EffectSlider.qml"
    "SettingsGroup.qml"
    "cork.jpg"
    "main.qml"
    "texture.png"
)

qt_add_resource(effects "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)


#### Keys ignored in scope 1:.:.:effects.pro:<TRUE>:
# DISTFILES = "EffectBox.qml" "EffectSlider.qml" "EffectColor.qml" "SettingsGroup.qml"
# OTHER_FILES = "main.qml"
