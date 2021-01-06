TARGET = QtQuick3DAssetImport
MODULE = quick3dassetimport

MODULE_PLUGIN_TYPES = assetimporters

QT += core-private gui qml quick3dutils-private

SOURCES = \
    qssgmeshbvhbuilder.cpp \
    qssgmesh.cpp

HEADERS = \
    qssgmeshbvhbuilder_p.h \
    qtquick3dassetimportglobal_p.h \
    qssgmesh_p.h

!integrity:!android|android_app:!wasm:!cross_compile {
SOURCES += \
    qssgassetimporterfactory.cpp \
    qssgassetimportmanager.cpp \
    qssgqmlutilities.cpp

HEADERS += \
    qssgqmlutilities_p.h \
    qssgassetimporter_p.h \
    qssgassetimporterfactory_p.h \
    qssgassetimporterplugin_p.h \
    qssgassetimportmanager_p.h
}

DEFINES += QT_BUILD_QUICK3DASSETIMPORT_LIB

OTHER_FILES += options-schema.json

include(../3rdparty/xatlas/xatlas.pri)

load(qt_module)
