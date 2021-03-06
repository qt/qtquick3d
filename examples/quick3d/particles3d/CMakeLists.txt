# Generated from particles3d.pro.

cmake_minimum_required(VERSION 3.14)
project(particles3d LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/particles3d")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Quick3D)

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


# Resources:
set(qml_resource_files
    "qml/AlignedParticles.qml"
    "qml/AnimatedSprite.qml"
    "qml/AttractorShapes.qml"
    "qml/ColorfulParticles.qml"
    "qml/CustomCheckBox.qml"
    "qml/CustomLabel.qml"
    "qml/CustomSlider.qml"
    "qml/EmitAndBurst.qml"
    "qml/EmitterShapes.qml"
    "qml/FadingInOut.qml"
    "qml/Fire.qml"
    "qml/LoggingView.qml"
    "qml/QtLogoAnimation.qml"
    "qml/SettingsView.qml"
    "qml/StartupView.qml"
    "qml/Snowing.qml"
    "qml/SystemPlayPause.qml"
    "qml/TrailEmitterBurst.qml"
    "qml/HeartTrail.qml"
    "qml/OceanSpider.qml"
    "qml/images/arrow_icon.png"
    "qml/images/icon_interval.png"
    "qml/images/icon_logging.png"
    "qml/images/icon_settings.png"
    "qml/images/icon_pause.png"
    "qml/images/icon_play.png"
    "qml/images/qt_logo.png"
    "qml/images/qt_logo2.png"
    "qml/images/qt_logo2_n.png"
    "qml/images/smoke.png"
    "qml/images/snowflake.png"
    "qml/images/star.png"
    "qml/images/star2.png"
    "qml/images/dot.png"
    "qml/images/sphere.png"
    "qml/images/colorTable.png"
    "qml/images/color_table2.png"
    "qml/images/color_table3.png"
    "qml/images/smoke_sprite.png"
    "qml/images/dust.png"
    "qml/images/bear_black.png"
    "qml/images/explosion_01_strip13.png"
    "qml/images/leather_n.png"
    "qml/main.qml"
)

qt6_add_resources(particles3d "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

install(TARGETS particles3d
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
