cmake_minimum_required(VERSION 3.16)
project(customgeometry LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/customgeometry")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Quick3D)

qt_add_executable(customgeometry
    examplegeometry.cpp examplegeometry.h
    main.cpp
)

set_target_properties(customgeometry PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(customgeometry PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)

qt_add_qml_module(customgeometry
    URI CustomGeometryExample
    VERSION 1.0
    QML_FILES main.qml
    RESOURCES qt_logo_rect.png
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS customgeometry
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
