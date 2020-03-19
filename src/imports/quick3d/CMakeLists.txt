# Generated from quick3d.pro.

#####################################################################
## qquick3dplugin Plugin:
#####################################################################

qt_add_qml_module(qquick3dplugin
    URI "QtQuick3D"
    VERSION "1.${CMAKE_PROJECT_VERSION_MINOR}"
    DESIGNER_SUPPORTED
    CLASSNAME QQuick3DPlugin
    SKIP_TYPE_REGISTRATION
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::QmlPrivate
        Qt::Quick3DPrivate
        Qt::QuickPrivate
)

#### Keys ignored in scope 1:.:.:quick3d.pro:<TRUE>:
# CXX_MODULE = "qml"
# IMPORT_VERSION = "1.$$QT_MINOR_VERSION"
# OTHER_FILES = "$$PWD/images/*.qml" "$$PWD/src/*.qdoc" "$$PWD/src/asset-conditioning/*.qdoc" "qmldir"
# TARGETPATH = "QtQuick3D"

## Scopes:
#####################################################################
qt_add_docs(qquick3dplugin
    doc/qtquick3d.qdocconf
)

