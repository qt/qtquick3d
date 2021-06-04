#####################################################################
## qtquick3dhelpersplugin Plugin:
#####################################################################

set(qml_files
    "AxisHelper.qml"
    "DebugView.qml"
    "WasdController.qml"
)
set_source_files_properties(${qml_files} PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)

qt_internal_add_qml_module(Quick3DHelpers
    URI "QtQuick3D.Helpers"
    VERSION "${PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    CLASSNAME QtQuick3DHelpersPlugin
    PLUGIN_TARGET qtquick3dhelpersplugin
    DEPENDENCIES
        QtQuick3D/1.0
    SOURCES
        gridgeometry.cpp gridgeometry_p.h
        randominstancing.cpp randominstancing_p.h
        qtquick3dhelpersglobal_p.h
    QML_FILES
        ${qml_files}
    DEFINES
        QT_BUILD_QUICK3DHELPERS_LIB
    LIBRARIES
        Qt::Quick3D
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::Quick
        Qt::Quick3DPrivate
)

qt_internal_add_resource(Quick3DHelpersPrivate "Quick3DHelpers"
    PREFIX
        "/qt-project.org/imports/QtQuick3D/Helpers"
    FILES
        "meshes/axisGrid.mesh"
)
