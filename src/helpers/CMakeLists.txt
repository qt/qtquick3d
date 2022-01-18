#####################################################################
## qtquick3dhelpersplugin Plugin:
#####################################################################

qt_internal_add_qml_module(Quick3DHelpers
    URI "QtQuick3D.Helpers"
    VERSION "${PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    CLASSNAME QtQuick3DHelpersPlugin
    PLUGIN_TARGET qtquick3dhelpersplugin
    DEPENDENCIES
        QtQuick3D/auto
    PAST_MAJOR_VERSIONS 1
    SOURCES
        gridgeometry.cpp gridgeometry_p.h
        randominstancing.cpp randominstancing_p.h
        qtquick3dhelpersglobal_p.h
    QML_FILES
        AxisHelper.qml
        DebugView.qml
        WasdController.qml
    RESOURCES
        meshes/axisGrid.mesh
    DEFINES
        QT_BUILD_QUICK3DHELPERS_LIB
    LIBRARIES
        Qt::Quick3D
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::Quick
        Qt::Quick3DPrivate
)

if(QT_FEATURE_quick_designer AND QT_BUILD_SHARED_LIBS) # special case handle unconverted static
    add_subdirectory(designer)
endif()
