# Generated from customshaders.pro.

cmake_minimum_required(VERSION 3.14)
project(customshaders LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/quick3d/customshaders")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Quick3D)

add_qt_gui_executable(customshaders
    main.cpp
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
    "main.qml"
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
