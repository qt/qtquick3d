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
    "qml/AttractorShapes.qml"
    "qml/ColorfulParticles.qml"
    "qml/CustomCheckBox.qml"
    "qml/CustomSlider.qml"
    "qml/EmitAndBurst.qml"
    "qml/EmitterShapes.qml"
    "qml/FadingInOut.qml"
    "qml/LoggingView.qml"
    "qml/QtLogoAnimation.qml"
    "qml/QtLogoAnimation2.qml"
    "qml/Snowing.qml"
    "qml/SystemPlayPause.qml"
    "qml/images/arrow_icon.png"
    "qml/images/icon_interval.png"
    "qml/images/icon_logging.png"
    "qml/images/icon_pause.png"
    "qml/images/icon_play.png"
    "qml/images/qt_logo.png"
    "qml/images/smoke.png"
    "qml/images/snowflake.png"
    "qml/images/star.png"
    "qml/images/dot.png"
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
