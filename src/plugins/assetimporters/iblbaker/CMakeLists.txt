# Generated from iblbaker.pro.

#####################################################################
## IblBakerAssetImporterPlugin Plugin:
#####################################################################

qt_internal_add_plugin(IblBakerAssetImporterPlugin
    OUTPUT_NAME iblbaker
    TYPE assetimporters
    SOURCES
        iblbakerassetimporterplugin.cpp iblbakerassetimporterplugin.h
        iblbakerimporter.cpp iblbakerimporter.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Quick3DAssetImportPrivate
        Qt::Quick3DPrivate
)

qt_internal_add_resource(IblBakerAssetImporterPlugin "iblbaker"
    PREFIX
        "/"
    FILES
        ${iblbaker_resource_files}
)


#### Keys ignored in scope 1:.:.:iblbaker.pro:<TRUE>:
# DISTFILES = "options.json"
# OTHER_FILES = "iblbaker.json"
