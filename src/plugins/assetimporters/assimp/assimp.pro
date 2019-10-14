TARGET = assimp
QT += quick3dassetimport-private

PLUGIN_TYPE = assetimporters
PLUGIN_CLASS_NAME = AssimpImporterPlugin

load(qt_plugin)

QT_FOR_CONFIG += assetimporters-private
include($$OUT_PWD/../qtassetimporters-config.pri)

qtConfig(system-assimp):!if(cross_compile:host_build) {
    QMAKE_USE_PRIVATE += assimp
} else {
    include(../../../3rdparty/assimp/assimp.pri)
}

OTHER_FILES += assimp.json

HEADERS += \
    assimpimporter.h \
    assimpimporterplugin.h

SOURCES += \
    assimpimporter.cpp \
    assimpimporterplugin.cpp

RESOURCES += \
    assimp.qrc

DISTFILES += \
    options.json

