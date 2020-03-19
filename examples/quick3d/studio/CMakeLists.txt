# Generated from studio.pro.

cmake_minimum_required(VERSION 3.14)
project(studio LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/quick3d/studio")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Quick3D)

add_qt_gui_executable(studio
    main.cpp
)
target_link_libraries(studio PRIVATE
    Qt::Quick3DPrivate
)

target_link_libraries(studio PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)


# Resources:
set(qml_resource_files
    "OverlayLabel.qml"
    "gizmos/AbstractGizmo.qml"
    "gizmos/CameraGizmo.qml"
    "gizmos/GizmoScaleHelper.qml"
    "gizmos/RotateGizmo.qml"
    "gizmos/RotateGizmoTorus360.qml"
    "gizmos/RotateGizmoTorus90.qml"
    "gizmos/TranslateGizmo.qml"
    "gizmos/TranslateGizmoArrow.qml"
    "main.qml"
    "meshes/gizmoarrow.mesh"
    "meshes/gizmotorus90.mesh"
    "meshes/teapot.mesh"
)

qt6_add_resources(studio "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

install(TARGETS studio
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
