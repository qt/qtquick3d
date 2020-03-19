# Generated from helpers.pro.

#####################################################################
## qtquick3dhelpersplugin Plugin:
#####################################################################

qt_add_qml_module(qtquick3dhelpersplugin
    URI "QtQuick3D.Helpers"
    VERSION "1.${CMAKE_PROJECT_VERSION_MINOR}"
    DESIGNER_SUPPORTED
    CLASSNAME QtQuick3DHelpersPlugin
    DEPENDENCIES
        QtQuick3D/1.0
    SOURCES
        gridgeometry.cpp gridgeometry_p.h
        plugin.cpp
        pointerplane.cpp pointerplane.h
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::Quick
        Qt::Quick3DPrivate
)

#### Keys ignored in scope 2:.:.:plugin.pro:<TRUE>:
# CXX_MODULE = "qml"
# DESTFILES = "qmldir"
# IMPORT_VERSION = "1.$$QT_MINOR_VERSION"
# MESH_FILES = "meshes/axisGrid.mesh"
# OTHER_FILES = "$$QML_FILES"
# QML_FILES = "AxisHelper.qml" "DebugView.qml" "WasdController.qml" "$$MESH_FILES"
# TARGETPATH = "QtQuick3D/Helpers"

set(qml_files
    "AxisHelper.qml"
    "DebugView.qml"
    "WasdController.qml"
    "meshes/axisGrid.mesh"
)
set_source_files_properties(AxisHelper.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)
set_source_files_properties(DebugView.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)
set_source_files_properties(WasdController.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)
set_source_files_properties(meshes/axisGrid.mesh PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)

qt6_target_qml_files(qtquick3dhelpersplugin
    FILES
        ${qml_files}
)
