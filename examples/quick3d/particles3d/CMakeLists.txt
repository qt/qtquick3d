cmake_minimum_required(VERSION 3.16)
project(particles3d LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/particles3d")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Quick3D)

qt_add_executable(particles3d
    main.cpp
)

set_target_properties(particles3d PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(particles3d PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)

qt_add_qml_module(particles3d
    URI Example
    VERSION 1.0
    QML_FILES
        qml/AlignedParticles.qml
        qml/AnimatedSprite.qml
        qml/AttractorShapes.qml
        qml/ColorfulParticles.qml
        qml/CustomCheckBox.qml
        qml/CustomLabel.qml
        qml/CustomSlider.qml
        qml/CustomSelectionBox.qml
        qml/DynamicBursts.qml
        qml/EmitAndBurst.qml
        qml/EmitterCustomShapes.qml
        qml/EmitterShapes.qml
        qml/FadingInOut.qml
        qml/Fire.qml
        qml/HeartTrail.qml
        qml/Lights.qml
        qml/LoggingView.qml
        qml/ModelBlendParticles.qml
        qml/OceanSpider.qml
        qml/QtLogoAnimation.qml
        qml/SettingsView.qml
        qml/Snowing.qml
        qml/Sorting.qml
        qml/Speedometer.qml
        qml/StartupView.qml
        qml/SystemPlayPause.qml
        qml/TrailEmitterBurst.qml
        qml/ModelShape.qml
        qml/main.qml
        qml/LineParticles.qml
    RESOURCES
        qml/images/arrow_icon.png
        qml/images/bear_black.png
        qml/images/colorTable.png
        qml/images/color_table2.png
        qml/images/color_table3.png
        qml/images/color_table4.png
        qml/images/color_table5.png
        qml/images/dot.png
        qml/images/dust.png
        qml/images/explosion_01_strip13.png
        qml/images/icon_interval.png
        qml/images/icon_logging.png
        qml/images/icon_pause.png
        qml/images/icon_play.png
        qml/images/icon_settings.png
        qml/images/leather_n.png
        qml/images/qt_logo.png
        qml/images/qt_logo2.png
        qml/images/qt_logo2_n.png
        qml/images/smoke.png
        qml/images/smoke_sprite.png
        qml/images/snowflake.png
        qml/images/speedometer_labels.png
        qml/images/sphere.png
        qml/images/sprite_09.png
        qml/images/star.png
        qml/images/star2.png
        qml/images/star3.png
        qml/meshes/meter_background.mesh
        qml/meshes/meter_edge.mesh
        qml/meshes/oldqtlogo.mesh
        qml/meshes/suzanne.mesh
        qml/data/qt_logo_in_4096.cbor
        qml/data/qt_logo_out_4096.cbor
        qml/data/heart_4096.cbor
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS particles3d
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
