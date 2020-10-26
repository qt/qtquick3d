# Generated from customshaders.pro.

cmake_minimum_required(VERSION 3.14)
project(customshaders LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/customshaders")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Quick3D)

qt_add_executable(customshaders
    main.cpp
)
set_target_properties(customshaders PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(customshaders PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)


# Resources:
set(resources_resource_files
    "ExampleMaterial.qml"
    "MaterialControl.qml"
    "example.frag"
    "example.vert"
    "example_tex.frag"
    "main.qml"
    "qt_logo.png"
)

qt6_add_resources(customshaders "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)

install(TARGETS customshaders
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
