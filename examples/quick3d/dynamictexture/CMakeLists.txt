cmake_minimum_required(VERSION 3.16)
project(dynamictexture LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/dynamictexture")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Quick3D)

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

qt_add_qml_module(dynamictexture
    URI Example
    VERSION 1.0
    QML_FILES
        CorkBoards.qml
        Doors.qml
        main.qml
        content/Panel.qml
    RESOURCES
        meshes/door1.mesh
        meshes/door2.mesh
        meshes/wall.mesh
        content/cork.jpg
        content/note-yellow.png
        content/tack.png
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS dynamictexture
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
