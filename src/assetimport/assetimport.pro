TARGET = QtQuick3DAssetImport
MODULE = quick3dassetimport

MODULE_PLUGIN_TYPES = assetimporters

QT += core-private gui qml quick3drender-private quick3dutils-private

SOURCES = \
    qssgassetimporterfactory.cpp \
    qssgassetimportmanager.cpp \
    qssgmeshutilities.cpp \
    qssgqmlutilities.cpp \
    qssgpathutilities.cpp

HEADERS = \
    qtquick3dassetimportglobal_p.h \
    qssgqmlutilities_p.h \
    qssgassetimporter_p.h \
    qssgassetimporterfactory_p.h \
    qssgassetimporterplugin_p.h \
    qssgassetimportmanager_p.h \
    qssgmeshutilities_p.h \
    qssgpathutilities_p.h

DEFINES += QT_BUILD_QUICK3DASSETIMPORT_LIB

load(qt_module)
