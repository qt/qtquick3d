# Generated from materiallib.pro.

#####################################################################
## qtquick3dmaterialplugin Plugin:
#####################################################################

qt_add_qml_module(qtquick3dmaterialplugin
    URI "QtQuick3D.Materials"
    VERSION "1.${CMAKE_PROJECT_VERSION_MINOR}"
    DESIGNER_SUPPORTED
    CLASSNAME QtQuick3DMaterialPlugin
    DEPENDENCIES
        QtQuick3D/1.0
        QtQuick.Window/2.1
    SKIP_TYPE_REGISTRATION
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::Quick
        Qt::Quick3DPrivate
)

#### Keys ignored in scope 2:.:.:plugin.pro:<TRUE>:
# CXX_MODULE = "qml"
# DISTFILES = "qmldir"
# IMPORT_VERSION = "1.$$QT_MINOR_VERSION"
# TARGETPATH = "QtQuick3D/Materials"

## Scopes:
#####################################################################
