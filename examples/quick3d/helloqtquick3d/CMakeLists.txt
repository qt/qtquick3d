# Generated from helloqtquick3d.pro.

cmake_minimum_required(VERSION 3.14)
project(helloqtquick3d LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/helloqtquick3d")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Quick3D)

qt_add_executable(helloqtquick3d
    imageinstancetable.cpp imageinstancetable.h
    main.cpp
)
set_target_properties(helloqtquick3d PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(helloqtquick3d PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)


# Resources:
set(qml_resource_files
    "main.qml"
    "oldqtlogo.mesh"
    "qt_logo.png"
)

qt6_add_resources(helloqtquick3d "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

install(TARGETS helloqtquick3d
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

set_target_properties(helloqtquick3d PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI InstancingExample
)

qt6_qml_type_registration(helloqtquick3d)
