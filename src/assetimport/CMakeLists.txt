# Generated from assetimport.pro.

#####################################################################
## Quick3DAssetImport Module:
#####################################################################

qt_internal_add_module(Quick3DAssetImport
    PLUGIN_TYPES assetimporters
    SOURCES
        ../3rdparty/xatlas/xatlas.cpp ../3rdparty/xatlas/xatlas.h
        qssgmeshbvhbuilder.cpp qssgmeshbvhbuilder_p.h
        qssgmesh.cpp qssgmesh_p.h
        qssglightmapuvgenerator.cpp qssglightmapuvgenerator_p.h
        qtquick3dassetimportglobal_p.h
    DEFINES
        QT_BUILD_QUICK3DASSETIMPORT_LIB
    INCLUDE_DIRECTORIES
        ../3rdparty/xatlas
    LIBRARIES
        Qt::CorePrivate
        Qt::Quick3DUtilsPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick3DUtils
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::Quick3DUtilsPrivate
)

#### Keys ignored in scope 1:.:.:assetimport.pro:<TRUE>:
# MODULE = "quick3dassetimport"
# OTHER_FILES = "options-schema.json"

## Scopes:
#####################################################################

qt_internal_extend_target(Quick3DAssetImport CONDITION (android_app OR NOT ANDROID) AND (android_app OR NOT INTEGRITY) AND (NOT ANDROID OR NOT CMAKE_CROSSCOMPILING) AND (NOT ANDROID OR NOT WASM) AND (NOT CMAKE_CROSSCOMPILING OR NOT INTEGRITY) AND (NOT INTEGRITY OR NOT WASM)
    SOURCES
        qssgassetimporter_p.h
        qssgassetimporterfactory.cpp qssgassetimporterfactory_p.h
        qssgassetimporterplugin_p.h
        qssgassetimportmanager.cpp qssgassetimportmanager_p.h
        qssgqmlutilities.cpp qssgqmlutilities_p.h
)

# Silence warnings in 3rdparty code
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set_source_files_properties(../3rdparty/xatlas/xatlas.cpp PROPERTIES COMPILE_FLAGS "-Wno-type-limits")
endif()
