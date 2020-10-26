# Generated from dynamictexture.pro.

cmake_minimum_required(VERSION 3.14)
project(dynamictexture LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/dynamictexture")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Quick3D)

qt_add_executable(dynamictexture
    main.cpp
)
set_target_properties(dynamictexture PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(dynamictexture PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)


# Resources:
set(qml_resource_files
    "CorkBoards.qml"
    "Doors.qml"
    "main.qml"
    "meshes/door1.mesh"
    "meshes/door2.mesh"
    "meshes/wall.mesh"
)

qt6_add_resources(dynamictexture "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
set(qmake_immediate_resource_files
    "content/Panel.qml"
    "content/cork.jpg"
    "content/note-yellow.png"
    "content/tack.png"
    "meshes/door1.mesh"
    "meshes/door2.mesh"
    "meshes/wall.mesh"
)

qt6_add_resources(dynamictexture "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)

install(TARGETS dynamictexture
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
