# Generated from helpers.pro.

#####################################################################
## qtquick3dhelpersplugin Plugin:
#####################################################################

qt_internal_add_qml_module(qtquick3dhelpersplugin
    URI "QtQuick3D.Helpers"
    VERSION "${CMAKE_PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    CLASSNAME QtQuick3DHelpersPlugin
    DEPENDENCIES
        QtQuick3D/1.0
    GENERATE_QMLTYPES
    INSTALL_QMLTYPES
    SOURCES
        gridgeometry.cpp gridgeometry_p.h
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::Quick
        Qt::Quick3DPrivate
)

#### Keys ignored in scope 2:.:.:plugin.pro:<TRUE>:
# CXX_MODULE = "qml"
# DESTFILES = "qmldir"
# MESH_FILES = "meshes/axisGrid.mesh"
# OTHER_FILES = "$$QML_FILES"
# QMLTYPES_FILENAME = "plugins.qmltypes"
# QMLTYPES_INSTALL_DIR = "$$[QT_INSTALL_QML]/QtQuick3D/Helpers"
# QML_FILES = "AxisHelper.qml" "DebugView.qml" "WasdController.qml" "$$MESH_FILES"
# QML_IMPORT_NAME = "QtQuick3D.Helpers"
# QML_IMPORT_VERSION = "$$QT_VERSION"
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
