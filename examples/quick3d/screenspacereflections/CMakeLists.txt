# Generated from screenspacereflections.pro.

cmake_minimum_required(VERSION 3.16)
project(custommaterial LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/screenspacereflections")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Quick3D)

qt_add_executable(screenspacereflections
    main.cpp
)
set_target_properties(screenspacereflections PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(screenspacereflections PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)

qt_add_qml_module(screenspacereflections
    URI Example
    VERSION 1.0
    QML_FILES
        main.qml
        ScreenSpaceReflections.qml
    RESOURCES
        maps/OpenfootageNET_lowerAustria01-1024.hdr
        qt_logo_rect.png
        material_screenspacereflections.frag
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS screenspacereflections
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
