CXX_MODULE = qml
TARGET = qtquick3dmaterialplugin
TARGETPATH = QtQuick3D/Materials
QT += quick qml quick3d-private
IMPORT_VERSION = 1.$$QT_MINOR_VERSION

SOURCES += \
    plugin.cpp

DISTFILES += \
    qmldir

!static: qtConfig(quick-designer): include(designer/designer.pri)

load(qml_plugin)
