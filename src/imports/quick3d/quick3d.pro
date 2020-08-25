CXX_MODULE = qml
TARGET = qquick3dplugin
TARGETPATH = QtQuick3D
QML_IMPORT_VERSION = $$QT_VERSION

QT += qml-private quick-private quick3d-private

include(doc/doc.pri)

!static: qtConfig(quick-designer): include(designer/designer.pri)

load(qml_plugin)

SOURCES += \
    plugin.cpp
