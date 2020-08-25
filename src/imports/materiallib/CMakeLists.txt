# Generated from materiallib.pro.

#####################################################################
## qtquick3dmaterialplugin Plugin:
#####################################################################

qt_add_qml_module(qtquick3dmaterialplugin
    URI "QtQuick3D.Materials"
    VERSION "${CMAKE_PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    CLASSNAME QtQuick3DMaterialPlugin
    DEPENDENCIES
        QtQuick3D/1.0
        QtQuick.Window/2.1
    SKIP_TYPE_REGISTRATION
    GENERATE_QMLTYPES
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
# QMLTYPES_FILENAME = "plugins.qmltypes"
# QMLTYPES_INSTALL_DIR = "$$[QT_INSTALL_QML]/QtQuick3D/Materials"
# QML_IMPORT_NAME = "QtQuick3D.Materials"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "QtQuick3D/Materials"

## Scopes:
#####################################################################
