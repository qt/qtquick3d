# Generated from quick3d.pro.

#####################################################################
## qquick3dplugin Plugin:
#####################################################################

qt_internal_add_qml_module(qquick3dplugin
    URI "QtQuick3D"
    VERSION "${CMAKE_PROJECT_VERSION}"
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
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "QtQuick3D"

## Scopes:
#####################################################################
qt_internal_add_docs(qquick3dplugin
    doc/qtquick3d.qdocconf
)

