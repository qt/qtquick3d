
cmake_minimum_required(VERSION 3.14)
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


# Resources:
set(qml_resource_files
    "main.qml"
    "SettingsView.qml"
    "CustomSelectionBox.qml"
    "CustomSlider.qml"
    "CustomCheckBox.qml"
    "images/icon_pause.png"
    "images/icon_play.png"
    "images/icon_settings.png"
)

qt6_add_resources(modelblendparticles "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

install(TARGETS modelblendparticles
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(modelblendparticles PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI TestGeometryExample
)

qt6_qml_type_registration(modelblendparticles)
