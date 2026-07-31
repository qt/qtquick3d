# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## GltfImporterPlugin Plugin:
#####################################################################

qt_internal_add_plugin(GltfImporterPlugin
    OUTPUT_NAME gltf
    PLUGIN_TYPE assetimporters
    SOURCES
        gltfimporter.cpp gltfimporter.h
        gltfimporterplugin.cpp gltfimporterplugin.h
        gltfmeshbuilder.cpp gltfmeshbuilder.h
        gltfsceneconverter.cpp gltfsceneconverter.h
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick3DAssetImportPrivate
        Qt::Quick3DAssetUtilsPrivate
        Qt::Quick3DGltfPrivate
        Qt::Quick3DPrivate
        Qt::Quick3DUtilsPrivate
        Qt::Quick3DRuntimeRenderPrivate
)

# Resources:
set(gltf_resource_files
    "options.json"
)

qt_internal_extend_target(GltfImporterPlugin CONDITION QT_FEATURE_concurrent
    LIBRARIES
        Qt::Concurrent
)

qt_internal_add_resource(GltfImporterPlugin "gltf"
    PREFIX
        "/gltfimporter"
    FILES
        ${gltf_resource_files}
)
