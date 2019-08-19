CXX_MODULE = qml
TARGET = qquick3dplugin
TARGETPATH = QtQuick3D
IMPORT_VERSION = 1.$$QT_MINOR_VERSION

QT += qml-private quick-private quick3d-private

include(doc/doc.pri)

OTHER_FILES += \
    qmldir

!static: qtConfig(quick-designer): include(designer/designer.pri)

load(qml_plugin)

SOURCES += \
    plugin.cpp
