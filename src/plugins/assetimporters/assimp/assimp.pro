TARGET = assimp
QT += quick3dassetimport-private

PLUGIN_TYPE = assetimporters
PLUGIN_CLASS_NAME = AssimpImporterPlugin

load(qt_plugin)

include(../../../3rdparty/assimp/assimp.pri)

OTHER_FILES += assimp.json

HEADERS += \
    assimpimporter.h \
    assimpimporterplugin.h

SOURCES += \
    assimpimporter.cpp \
    assimpimporterplugin.cpp

