# Generated from principledmaterial.pro.

cmake_minimum_required(VERSION 3.16)
project(principledmaterial LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/principledmaterial")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Quick3D)

qt_add_executable(principledmaterial
    main.cpp
)
set_target_properties(principledmaterial PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(principledmaterial PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)

qt_add_qml_module(principledmaterial
    URI Example
    VERSION 1.0
    QML_FILES
        MaterialControl.qml
        main.qml
    RESOURCES
        maps/OpenfootageNET_garage-1024.hdr
        maps/metallic/basecolor.jpg
        maps/metallic/metallic.jpg
        maps/metallic/normal.jpg
        maps/metallic/roughness.jpg
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS principledmaterial
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
