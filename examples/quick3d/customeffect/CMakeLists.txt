# Generated from customeffect.pro.

cmake_minimum_required(VERSION 3.14)
project(customeffect LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/customeffect")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Quick3D)

qt_add_executable(customeffect
    main.cpp
)
set_target_properties(customeffect PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(customeffect PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)


# Resources:
set(qml_resource_files
    "checkers2.png"
    "effect.frag"
    "effect2.frag"
    "effect2.vert"
    "main.qml"
    "qt_logo_rect.png"
)

qt6_add_resources(customeffect "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

install(TARGETS customeffect
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
