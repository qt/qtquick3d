CXX_MODULE = qml
TARGET = qtquick3dmaterialplugin
TARGETPATH = QtQuick3D/Materials
QT += quick qml quick3d-private
QML_IMPORT_VERSION = $$QT_VERSION

SOURCES += \
    plugin.cpp

DISTFILES += \
    qmldir

load(qml_plugin)

QMLTYPES_FILENAME = plugins.qmltypes
QMLTYPES_INSTALL_DIR = $$[QT_INSTALL_QML]/QtQuick3D/Materials
QML_IMPORT_NAME = QtQuick3D.Materials
QML_IMPORT_VERSION = $$QT_VERSION
CONFIG += qmltypes install_qmltypes install_metatypes
