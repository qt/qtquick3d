# Generated from principledmaterial.pro.

cmake_minimum_required(VERSION 3.14)
project(principledmaterial LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/quick3d/principledmaterial")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Quick3D)

add_qt_gui_executable(principledmaterial
    main.cpp
)
target_link_libraries(principledmaterial PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)


# Resources:
set(qml_resource_files
    "MaterialControl.qml"
    "main.qml"
)

qt6_add_resources(principledmaterial "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
set(materials_resource_files
    "maps/OpenfootageNET_garage-1024.hdr"
    "maps/metallic/basecolor.jpg"
    "maps/metallic/metallic.jpg"
    "maps/metallic/normal.jpg"
    "maps/metallic/roughness.jpg"
)

qt6_add_resources(principledmaterial "materials"
    PREFIX
        "/"
    FILES
        ${materials_resource_files}
)

set(shader_gen_files
    "main.qml"
)

qt6_quick3d_build_shaders(principledmaterial "shaders"
    PREFIX
        "/"
    FILES
        ${shader_gen_files}
)

install(TARGETS principledmaterial
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
