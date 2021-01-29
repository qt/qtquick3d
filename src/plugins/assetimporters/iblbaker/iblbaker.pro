TARGET = iblbaker
QT += quick3dassetimport-private
QT += quick3d-private

PLUGIN_TYPE = assetimporters
PLUGIN_CLASS_NAME = IblBakerAssetImporterPlugin

load(qt_plugin)

OTHER_FILES += iblbaker.json

HEADERS += \
    iblbakerassetimporterplugin.h \
    iblbakerimporter.h

SOURCES += \
    iblbakerassetimporterplugin.cpp \
    iblbakerimporter.cpp

RESOURCES += \
    iblbaker.qrc

DISTFILES += \
    options.json
