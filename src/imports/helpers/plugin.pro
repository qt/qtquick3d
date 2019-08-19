CXX_MODULE = qml
TARGET = qtquick3dhelpersplugin
TARGETPATH = QtQuick3D/Helpers
QT += quick qml
IMPORT_VERSION = 1.0

QML_FILES = \
    AxisHelper.qml \
    WasdController.qml

MESH_FILES = \
    meshes/axisGrid.mesh

QML_FILES += $$MESH_FILES

OTHER_FILES += $$QML_FILES

SOURCES += plugin.cpp

DESTFILES += qmldir

load(qml_plugin)
