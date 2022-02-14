cmake_minimum_required(VERSION 3.16)
project(proceduraltexture LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/proceduraltexture")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Quick3D)

qt_add_executable(proceduraltexture
    gradienttexture.cpp gradienttexture.h
    main.cpp
)

set_target_properties(proceduraltexture PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(proceduraltexture PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)

qt_add_qml_module(proceduraltexture
    URI ProceduralTextureExample
    VERSION 1.0
    QML_FILES main.qml
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS proceduraltexture
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
