# Generated from dynamiccreation.pro.

cmake_minimum_required(VERSION 3.14)
project(dynamiccreation LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/quick3d/dynamiccreation")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Quick3D)

add_qt_gui_executable(dynamiccreation
    main.cpp
)
target_link_libraries(dynamiccreation PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)


# Resources:
set(qml_resource_files
    "WeirdShape.qml"
    "main.qml"
    "weirdShape.mesh"
)

qt6_add_resources(dynamiccreation "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

install(TARGETS dynamiccreation
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
