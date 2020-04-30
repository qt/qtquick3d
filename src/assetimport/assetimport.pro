TARGET = QtQuick3DAssetImport
MODULE = quick3dassetimport

MODULE_PLUGIN_TYPES = assetimporters

QT += core-private gui qml quick3drender-private quick3dutils-private

SOURCES = \
    qssgmeshbvhbuilder.cpp \
    qssgmeshutilities.cpp

HEADERS = \
    qssgmeshbvhbuilder_p.h \
    qtquick3dassetimportglobal_p.h \
    qssgmeshutilities_p.h

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

load(qt_module)
