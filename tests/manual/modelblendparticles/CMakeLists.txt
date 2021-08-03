
cmake_minimum_required(VERSION 3.16)
project(modelblendparticles LANGUAGES CXX)

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

qt_add_executable(modelblendparticles
    main.cpp testgeometry.h
)
set_target_properties(modelblendparticles PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(modelblendparticles PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)

qt_add_qml_module(modelblendparticles
    URI ModelBlendParticlesExample
    VERSION 1.0
    QML_FILES
        main.qml
        SettingsView.qml
        CustomSelectionBox.qml
        CustomSlider.qml
        CustomCheckBox.qml
    RESOURCES
        images/icon_pause.png
        images/icon_play.png
        images/icon_settings.png
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS modelblendparticles
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

