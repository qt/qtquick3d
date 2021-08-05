# Generated from uip.pro.

#####################################################################
## UipAssetImporterPlugin Plugin:
#####################################################################

qt_internal_add_plugin(UipAssetImporterPlugin
    OUTPUT_NAME uip
    PLUGIN_TYPE assetimporters
    SOURCES
        abstractxmlparser.cpp abstractxmlparser.h
        datamodelparser.cpp datamodelparser.h
        enummaps.cpp enummaps.h
        keyframegroupgenerator.cpp keyframegroupgenerator.h
        propertymap.cpp propertymap.h
        uiaparser.cpp uiaparser.h
        uipassetimporterplugin.cpp uipassetimporterplugin.h
        uipimporter.cpp uipimporter.h
        uipparser.cpp uipparser.h
        uippresentation.cpp uippresentation.h
        uniqueidmapper.cpp uniqueidmapper.h
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Quick3DAssetImportPrivate
        Qt::Quick3DAssetUtilsPrivate
)

# Resources:
set(metadata_resource_files
    "MetaData.xml"
    "options.json"
)

qt_internal_add_resource(UipAssetImporterPlugin "metadata"
    PREFIX
        "/uipimporter"
    FILES
        ${metadata_resource_files}
)


#### Keys ignored in scope 1:.:.:uip.pro:<TRUE>:
# DISTFILES = <EMPTY>
# OTHER_FILES = "uip.json"
