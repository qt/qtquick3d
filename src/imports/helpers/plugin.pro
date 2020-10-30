CXX_MODULE = qml
TARGET = qtquick3dhelpersplugin
TARGETPATH = QtQuick3D/Helpers
QT += quick qml quick3d-private
QML_IMPORT_VERSION = $$QT_VERSION

QML_FILES = \
    AxisHelper.qml \
    DebugView.qml \
    WasdController.qml

MESH_FILES = \
    meshes/axisGrid.mesh

QML_FILES += $$MESH_FILES

OTHER_FILES += $$QML_FILES

SOURCES += \
    plugin.cpp \
    gridgeometry.cpp

DESTFILES += qmldir

load(qml_plugin)

HEADERS += \
    gridgeometry_p.h

QMLTYPES_FILENAME = plugins.qmltypes
QMLTYPES_INSTALL_DIR = $$[QT_INSTALL_QML]/QtQuick3D/Helpers
QML_IMPORT_NAME = QtQuick3D.Helpers
QML_IMPORT_VERSION = $$QT_VERSION
CONFIG += qmltypes install_qmltypes install_metatypes
