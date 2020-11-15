# Generated from assimp.pro.

#####################################################################
## AssimpImporterPlugin Plugin:
#####################################################################

# special case begin
if(NOT QT_FEATURE_system_zlib)
    find_package(Qt6 COMPONENTS Zlib)
elseif(NOT TARGET ZLIB::ZLIB)
    qt_find_package(ZLIB PROVIDED_TARGETS ZLIB::ZLIB)
endif()
# special case end

qt_internal_add_plugin(AssimpImporterPlugin
    OUTPUT_NAME assimp
    TYPE assetimporters
    EXCEPTIONS # special case
    SOURCES
        assimpimporter.cpp assimpimporter.h
        assimpimporterplugin.cpp assimpimporterplugin.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Quick3DAssetImportPrivate
)

# Resources:
set(assimp_resource_files
    "options.json"
)

qt_internal_add_resource(AssimpImporterPlugin "assimp"
    PREFIX
        "/assimpimporter"
    FILES
        ${assimp_resource_files}
)


#### Keys ignored in scope 1:.:.:assimp.pro:<TRUE>:
# DISTFILES = "options.json"
# OTHER_FILES = "assimp.json"
# QT_FOR_CONFIG = "assetimporters-private"

## Scopes:
#####################################################################

qt_internal_extend_target(AssimpImporterPlugin CONDITION QT_FEATURE_system_assimp AND (NOT CMAKE_CROSSCOMPILING OR NOT host_build)
    LIBRARIES
        WrapQuick3DAssimp::WrapQuick3DAssimp
)

qt_internal_extend_target(AssimpImporterPlugin CONDITION NOT QT_FEATURE_system_assimp OR (CMAKE_CROSSCOMPILING AND host_build)
    SOURCES
        ../../../3rdparty/assimp/config.h
        ../../../3rdparty/assimp/revision.h
        ../../../3rdparty/assimp/src/code/CApi/CInterfaceIOWrapper.cpp ../../../3rdparty/assimp/src/code/CApi/CInterfaceIOWrapper.h
        ../../../3rdparty/assimp/src/code/Collada/ColladaHelper.h
        ../../../3rdparty/assimp/src/code/Collada/ColladaLoader.cpp ../../../3rdparty/assimp/src/code/Collada/ColladaLoader.h
        ../../../3rdparty/assimp/src/code/Collada/ColladaParser.cpp ../../../3rdparty/assimp/src/code/Collada/ColladaParser.h
        ../../../3rdparty/assimp/src/code/Common/Assimp.cpp
        ../../../3rdparty/assimp/src/code/Common/BaseImporter.cpp
        ../../../3rdparty/assimp/src/code/Common/BaseProcess.cpp ../../../3rdparty/assimp/src/code/Common/BaseProcess.h
        ../../../3rdparty/assimp/src/code/Common/Bitmap.cpp
        ../../../3rdparty/assimp/src/code/Common/CreateAnimMesh.cpp
        ../../../3rdparty/assimp/src/code/Common/DefaultIOStream.cpp
        ../../../3rdparty/assimp/src/code/Common/DefaultIOSystem.cpp
        ../../../3rdparty/assimp/src/code/Common/DefaultLogger.cpp
        ../../../3rdparty/assimp/src/code/Common/DefaultProgressHandler.h
        ../../../3rdparty/assimp/src/code/Common/FileLogStream.h
        ../../../3rdparty/assimp/src/code/Common/IFF.h
        ../../../3rdparty/assimp/src/code/Common/Importer.cpp ../../../3rdparty/assimp/src/code/Common/Importer.h
        ../../../3rdparty/assimp/src/code/Common/ImporterRegistry.cpp
        ../../../3rdparty/assimp/src/code/Common/PolyTools.h
        ../../../3rdparty/assimp/src/code/Common/PostStepRegistry.cpp
        ../../../3rdparty/assimp/src/code/Common/RemoveComments.cpp
        ../../../3rdparty/assimp/src/code/Common/SGSpatialSort.cpp
        ../../../3rdparty/assimp/src/code/Common/SceneCombiner.cpp
        ../../../3rdparty/assimp/src/code/Common/ScenePreprocessor.cpp ../../../3rdparty/assimp/src/code/Common/ScenePreprocessor.h
        ../../../3rdparty/assimp/src/code/Common/ScenePrivate.h
        ../../../3rdparty/assimp/src/code/Common/SkeletonMeshBuilder.cpp
        ../../../3rdparty/assimp/src/code/Common/SpatialSort.cpp
        ../../../3rdparty/assimp/src/code/Common/SplitByBoneCountProcess.cpp ../../../3rdparty/assimp/src/code/Common/SplitByBoneCountProcess.h
        ../../../3rdparty/assimp/src/code/Common/StandardShapes.cpp
        ../../../3rdparty/assimp/src/code/Common/StdOStreamLogStream.h
        ../../../3rdparty/assimp/src/code/Common/Subdivision.cpp
        ../../../3rdparty/assimp/src/code/Common/TargetAnimation.cpp ../../../3rdparty/assimp/src/code/Common/TargetAnimation.h
        ../../../3rdparty/assimp/src/code/Common/Version.cpp
        ../../../3rdparty/assimp/src/code/Common/VertexTriangleAdjacency.cpp ../../../3rdparty/assimp/src/code/Common/VertexTriangleAdjacency.h
        ../../../3rdparty/assimp/src/code/Common/ZipArchiveIOSystem.cpp
        ../../../3rdparty/assimp/src/code/Common/scene.cpp
        ../../../3rdparty/assimp/src/code/Common/simd.cpp ../../../3rdparty/assimp/src/code/Common/simd.h
        ../../../3rdparty/assimp/src/code/FBX/FBXAnimation.cpp
        ../../../3rdparty/assimp/src/code/FBX/FBXBinaryTokenizer.cpp
        ../../../3rdparty/assimp/src/code/FBX/FBXCommon.h
        ../../../3rdparty/assimp/src/code/FBX/FBXCompileConfig.h
        ../../../3rdparty/assimp/src/code/FBX/FBXConverter.cpp ../../../3rdparty/assimp/src/code/FBX/FBXConverter.h
        ../../../3rdparty/assimp/src/code/FBX/FBXDeformer.cpp
        ../../../3rdparty/assimp/src/code/FBX/FBXDocument.cpp ../../../3rdparty/assimp/src/code/FBX/FBXDocument.h
        ../../../3rdparty/assimp/src/code/FBX/FBXDocumentUtil.cpp
        ../../../3rdparty/assimp/src/code/FBX/FBXImportSettings.h
        ../../../3rdparty/assimp/src/code/FBX/FBXImporter.cpp ../../../3rdparty/assimp/src/code/FBX/FBXImporter.h
        ../../../3rdparty/assimp/src/code/FBX/FBXMaterial.cpp
        ../../../3rdparty/assimp/src/code/FBX/FBXMeshGeometry.cpp ../../../3rdparty/assimp/src/code/FBX/FBXMeshGeometry.h
        ../../../3rdparty/assimp/src/code/FBX/FBXModel.cpp
        ../../../3rdparty/assimp/src/code/FBX/FBXNodeAttribute.cpp
        ../../../3rdparty/assimp/src/code/FBX/FBXParser.cpp ../../../3rdparty/assimp/src/code/FBX/FBXParser.h
        ../../../3rdparty/assimp/src/code/FBX/FBXProperties.cpp ../../../3rdparty/assimp/src/code/FBX/FBXProperties.h
        ../../../3rdparty/assimp/src/code/FBX/FBXTokenizer.cpp ../../../3rdparty/assimp/src/code/FBX/FBXTokenizer.h
        ../../../3rdparty/assimp/src/code/FBX/FBXUtil.cpp ../../../3rdparty/assimp/src/code/FBX/FBXUtil.h
        ../../../3rdparty/assimp/src/code/Material/MaterialSystem.cpp ../../../3rdparty/assimp/src/code/Material/MaterialSystem.h
        ../../../3rdparty/assimp/src/code/Obj/ObjFileData.h
        ../../../3rdparty/assimp/src/code/Obj/ObjFileImporter.cpp ../../../3rdparty/assimp/src/code/Obj/ObjFileImporter.h
        ../../../3rdparty/assimp/src/code/Obj/ObjFileMtlImporter.cpp ../../../3rdparty/assimp/src/code/Obj/ObjFileMtlImporter.h
        ../../../3rdparty/assimp/src/code/Obj/ObjFileParser.cpp ../../../3rdparty/assimp/src/code/Obj/ObjFileParser.h
        ../../../3rdparty/assimp/src/code/Obj/ObjTools.h
        ../../../3rdparty/assimp/src/code/PostProcessing/CalcTangentsProcess.cpp ../../../3rdparty/assimp/src/code/PostProcessing/CalcTangentsProcess.h
        ../../../3rdparty/assimp/src/code/PostProcessing/ComputeUVMappingProcess.cpp ../../../3rdparty/assimp/src/code/PostProcessing/ComputeUVMappingProcess.h
        ../../../3rdparty/assimp/src/code/PostProcessing/ConvertToLHProcess.cpp ../../../3rdparty/assimp/src/code/PostProcessing/ConvertToLHProcess.h
        ../../../3rdparty/assimp/src/code/PostProcessing/DeboneProcess.cpp ../../../3rdparty/assimp/src/code/PostProcessing/DeboneProcess.h
        ../../../3rdparty/assimp/src/code/PostProcessing/DropFaceNormalsProcess.cpp ../../../3rdparty/assimp/src/code/PostProcessing/DropFaceNormalsProcess.h
        ../../../3rdparty/assimp/src/code/PostProcessing/EmbedTexturesProcess.cpp ../../../3rdparty/assimp/src/code/PostProcessing/EmbedTexturesProcess.h
        ../../../3rdparty/assimp/src/code/PostProcessing/FindDegenerates.cpp ../../../3rdparty/assimp/src/code/PostProcessing/FindDegenerates.h
        ../../../3rdparty/assimp/src/code/PostProcessing/FindInstancesProcess.cpp ../../../3rdparty/assimp/src/code/PostProcessing/FindInstancesProcess.h
        ../../../3rdparty/assimp/src/code/PostProcessing/FindInvalidDataProcess.cpp ../../../3rdparty/assimp/src/code/PostProcessing/FindInvalidDataProcess.h
        ../../../3rdparty/assimp/src/code/PostProcessing/FixNormalsStep.cpp ../../../3rdparty/assimp/src/code/PostProcessing/FixNormalsStep.h
        ../../../3rdparty/assimp/src/code/PostProcessing/GenBoundingBoxesProcess.cpp ../../../3rdparty/assimp/src/code/PostProcessing/GenBoundingBoxesProcess.h
        ../../../3rdparty/assimp/src/code/PostProcessing/GenFaceNormalsProcess.cpp ../../../3rdparty/assimp/src/code/PostProcessing/GenFaceNormalsProcess.h
        ../../../3rdparty/assimp/src/code/PostProcessing/GenVertexNormalsProcess.cpp ../../../3rdparty/assimp/src/code/PostProcessing/GenVertexNormalsProcess.h
        ../../../3rdparty/assimp/src/code/PostProcessing/ImproveCacheLocality.cpp ../../../3rdparty/assimp/src/code/PostProcessing/ImproveCacheLocality.h
        ../../../3rdparty/assimp/src/code/PostProcessing/JoinVerticesProcess.cpp ../../../3rdparty/assimp/src/code/PostProcessing/JoinVerticesProcess.h
        ../../../3rdparty/assimp/src/code/PostProcessing/MakeVerboseFormat.cpp ../../../3rdparty/assimp/src/code/PostProcessing/MakeVerboseFormat.h
        ../../../3rdparty/assimp/src/code/PostProcessing/OptimizeGraph.cpp ../../../3rdparty/assimp/src/code/PostProcessing/OptimizeGraph.h
        ../../../3rdparty/assimp/src/code/PostProcessing/OptimizeMeshes.cpp ../../../3rdparty/assimp/src/code/PostProcessing/OptimizeMeshes.h
        ../../../3rdparty/assimp/src/code/PostProcessing/PretransformVertices.cpp ../../../3rdparty/assimp/src/code/PostProcessing/PretransformVertices.h
        ../../../3rdparty/assimp/src/code/PostProcessing/ProcessHelper.cpp ../../../3rdparty/assimp/src/code/PostProcessing/ProcessHelper.h
        ../../../3rdparty/assimp/src/code/PostProcessing/RemoveRedundantMaterials.cpp ../../../3rdparty/assimp/src/code/PostProcessing/RemoveRedundantMaterials.h
        ../../../3rdparty/assimp/src/code/PostProcessing/RemoveVCProcess.cpp ../../../3rdparty/assimp/src/code/PostProcessing/RemoveVCProcess.h
        ../../../3rdparty/assimp/src/code/PostProcessing/ScaleProcess.cpp ../../../3rdparty/assimp/src/code/PostProcessing/ScaleProcess.h
        ../../../3rdparty/assimp/src/code/PostProcessing/SortByPTypeProcess.cpp ../../../3rdparty/assimp/src/code/PostProcessing/SortByPTypeProcess.h
        ../../../3rdparty/assimp/src/code/PostProcessing/SplitLargeMeshes.cpp ../../../3rdparty/assimp/src/code/PostProcessing/SplitLargeMeshes.h
        ../../../3rdparty/assimp/src/code/PostProcessing/TextureTransform.cpp ../../../3rdparty/assimp/src/code/PostProcessing/TextureTransform.h
        ../../../3rdparty/assimp/src/code/PostProcessing/TriangulateProcess.cpp ../../../3rdparty/assimp/src/code/PostProcessing/TriangulateProcess.h
        ../../../3rdparty/assimp/src/code/PostProcessing/ValidateDataStructure.cpp ../../../3rdparty/assimp/src/code/PostProcessing/ValidateDataStructure.h
        ../../../3rdparty/assimp/src/code/STL/STLLoader.cpp ../../../3rdparty/assimp/src/code/STL/STLLoader.h
        ../../../3rdparty/assimp/src/code/glTF/glTFAsset.h ../../../3rdparty/assimp/src/code/glTF/glTFAsset.inl
        ../../../3rdparty/assimp/src/code/glTF/glTFAssetWriter.h ../../../3rdparty/assimp/src/code/glTF/glTFAssetWriter.inl
        ../../../3rdparty/assimp/src/code/glTF/glTFCommon.cpp ../../../3rdparty/assimp/src/code/glTF/glTFCommon.h
        ../../../3rdparty/assimp/src/code/glTF/glTFImporter.cpp ../../../3rdparty/assimp/src/code/glTF/glTFImporter.h
        ../../../3rdparty/assimp/src/code/glTF2/glTF2Asset.h ../../../3rdparty/assimp/src/code/glTF2/glTF2Asset.inl
        ../../../3rdparty/assimp/src/code/glTF2/glTF2AssetWriter.h ../../../3rdparty/assimp/src/code/glTF2/glTF2AssetWriter.inl
        ../../../3rdparty/assimp/src/code/glTF2/glTF2Importer.cpp ../../../3rdparty/assimp/src/code/glTF2/glTF2Importer.h
        ../../../3rdparty/assimp/src/contrib/irrXML/CXMLReaderImpl.h
        ../../../3rdparty/assimp/src/contrib/irrXML/heapsort.h
        ../../../3rdparty/assimp/src/contrib/irrXML/irrArray.h
        ../../../3rdparty/assimp/src/contrib/irrXML/irrString.h
        ../../../3rdparty/assimp/src/contrib/irrXML/irrTypes.h
        ../../../3rdparty/assimp/src/contrib/irrXML/irrXML.cpp ../../../3rdparty/assimp/src/contrib/irrXML/irrXML.h
        ../../../3rdparty/assimp/src/contrib/poly2tri/poly2tri/common/shapes.cc ../../../3rdparty/assimp/src/contrib/poly2tri/poly2tri/common/shapes.h
        ../../../3rdparty/assimp/src/contrib/poly2tri/poly2tri/common/utils.h
        ../../../3rdparty/assimp/src/contrib/poly2tri/poly2tri/sweep/advancing_front.cc ../../../3rdparty/assimp/src/contrib/poly2tri/poly2tri/sweep/advancing_front.h
        ../../../3rdparty/assimp/src/contrib/poly2tri/poly2tri/sweep/cdt.cc ../../../3rdparty/assimp/src/contrib/poly2tri/poly2tri/sweep/cdt.h
        ../../../3rdparty/assimp/src/contrib/poly2tri/poly2tri/sweep/sweep.cc ../../../3rdparty/assimp/src/contrib/poly2tri/poly2tri/sweep/sweep.h
        ../../../3rdparty/assimp/src/contrib/poly2tri/poly2tri/sweep/sweep_context.cc ../../../3rdparty/assimp/src/contrib/poly2tri/poly2tri/sweep/sweep_context.h
        ../../../3rdparty/assimp/src/contrib/unzip/crypt.h
        ../../../3rdparty/assimp/src/contrib/unzip/ioapi.c ../../../3rdparty/assimp/src/contrib/unzip/ioapi.h
        ../../../3rdparty/assimp/src/contrib/unzip/unzip.c
        ../../../3rdparty/assimp/src/include/assimp/BaseImporter.h
        ../../../3rdparty/assimp/src/include/assimp/Bitmap.h
        ../../../3rdparty/assimp/src/include/assimp/BlobIOSystem.h
        ../../../3rdparty/assimp/src/include/assimp/ByteSwapper.h
        ../../../3rdparty/assimp/src/include/assimp/Compiler/poppack1.h
        ../../../3rdparty/assimp/src/include/assimp/Compiler/pstdint.h
        ../../../3rdparty/assimp/src/include/assimp/Compiler/pushpack1.h
        ../../../3rdparty/assimp/src/include/assimp/CreateAnimMesh.h
        ../../../3rdparty/assimp/src/include/assimp/DefaultIOStream.h
        ../../../3rdparty/assimp/src/include/assimp/DefaultIOSystem.h
        ../../../3rdparty/assimp/src/include/assimp/DefaultLogger.hpp
        ../../../3rdparty/assimp/src/include/assimp/Defines.h
        ../../../3rdparty/assimp/src/include/assimp/Exceptional.h
        ../../../3rdparty/assimp/src/include/assimp/Exporter.hpp
        ../../../3rdparty/assimp/src/include/assimp/GenericProperty.h
        ../../../3rdparty/assimp/src/include/assimp/Hash.h
        ../../../3rdparty/assimp/src/include/assimp/IOStream.hpp
        ../../../3rdparty/assimp/src/include/assimp/IOStreamBuffer.h
        ../../../3rdparty/assimp/src/include/assimp/IOSystem.hpp
        ../../../3rdparty/assimp/src/include/assimp/Importer.hpp
        ../../../3rdparty/assimp/src/include/assimp/LineSplitter.h
        ../../../3rdparty/assimp/src/include/assimp/LogAux.h
        ../../../3rdparty/assimp/src/include/assimp/LogStream.hpp
        ../../../3rdparty/assimp/src/include/assimp/Logger.hpp
        ../../../3rdparty/assimp/src/include/assimp/Macros.h
        ../../../3rdparty/assimp/src/include/assimp/MathFunctions.h
        ../../../3rdparty/assimp/src/include/assimp/MemoryIOWrapper.h
        ../../../3rdparty/assimp/src/include/assimp/NullLogger.hpp
        ../../../3rdparty/assimp/src/include/assimp/ParsingUtils.h
        ../../../3rdparty/assimp/src/include/assimp/Profiler.h
        ../../../3rdparty/assimp/src/include/assimp/ProgressHandler.hpp
        ../../../3rdparty/assimp/src/include/assimp/RemoveComments.h
        ../../../3rdparty/assimp/src/include/assimp/SGSpatialSort.h
        ../../../3rdparty/assimp/src/include/assimp/SceneCombiner.h
        ../../../3rdparty/assimp/src/include/assimp/SkeletonMeshBuilder.h
        ../../../3rdparty/assimp/src/include/assimp/SmoothingGroups.h ../../../3rdparty/assimp/src/include/assimp/SmoothingGroups.inl
        ../../../3rdparty/assimp/src/include/assimp/SpatialSort.h
        ../../../3rdparty/assimp/src/include/assimp/StandardShapes.h
        ../../../3rdparty/assimp/src/include/assimp/StreamReader.h
        ../../../3rdparty/assimp/src/include/assimp/StreamWriter.h
        ../../../3rdparty/assimp/src/include/assimp/StringComparison.h
        ../../../3rdparty/assimp/src/include/assimp/StringUtils.h
        ../../../3rdparty/assimp/src/include/assimp/Subdivision.h
        ../../../3rdparty/assimp/src/include/assimp/TinyFormatter.h
        ../../../3rdparty/assimp/src/include/assimp/Vertex.h
        ../../../3rdparty/assimp/src/include/assimp/XMLTools.h
        ../../../3rdparty/assimp/src/include/assimp/ZipArchiveIOSystem.h
        ../../../3rdparty/assimp/src/include/assimp/ai_assert.h
        ../../../3rdparty/assimp/src/include/assimp/anim.h
        ../../../3rdparty/assimp/src/include/assimp/camera.h
        ../../../3rdparty/assimp/src/include/assimp/cexport.h
        ../../../3rdparty/assimp/src/include/assimp/cfileio.h
        ../../../3rdparty/assimp/src/include/assimp/cimport.h
        ../../../3rdparty/assimp/src/include/assimp/color4.h ../../../3rdparty/assimp/src/include/assimp/color4.inl
        ../../../3rdparty/assimp/src/include/assimp/defs.h
        ../../../3rdparty/assimp/src/include/assimp/fast_atof.h
        ../../../3rdparty/assimp/src/include/assimp/importerdesc.h
        ../../../3rdparty/assimp/src/include/assimp/irrXMLWrapper.h
        ../../../3rdparty/assimp/src/include/assimp/light.h
        ../../../3rdparty/assimp/src/include/assimp/material.h ../../../3rdparty/assimp/src/include/assimp/material.inl
        ../../../3rdparty/assimp/src/include/assimp/matrix3x3.h ../../../3rdparty/assimp/src/include/assimp/matrix3x3.inl
        ../../../3rdparty/assimp/src/include/assimp/matrix4x4.h ../../../3rdparty/assimp/src/include/assimp/matrix4x4.inl
        ../../../3rdparty/assimp/src/include/assimp/mesh.h
        ../../../3rdparty/assimp/src/include/assimp/metadata.h
        ../../../3rdparty/assimp/src/include/assimp/pbrmaterial.h
        ../../../3rdparty/assimp/src/include/assimp/postprocess.h
        ../../../3rdparty/assimp/src/include/assimp/qnan.h
        ../../../3rdparty/assimp/src/include/assimp/quaternion.h ../../../3rdparty/assimp/src/include/assimp/quaternion.inl
        ../../../3rdparty/assimp/src/include/assimp/scene.h
        ../../../3rdparty/assimp/src/include/assimp/texture.h
        ../../../3rdparty/assimp/src/include/assimp/types.h
        ../../../3rdparty/assimp/src/include/assimp/vector2.h ../../../3rdparty/assimp/src/include/assimp/vector2.inl
        ../../../3rdparty/assimp/src/include/assimp/vector3.h ../../../3rdparty/assimp/src/include/assimp/vector3.inl
        ../../../3rdparty/assimp/src/include/assimp/version.h
        ../../../3rdparty/assimp/unzip/unzip.h
    DEFINES
        ASSIMP_BUILD_BOOST_WORKAROUND
        ASSIMP_BUILD_NO_3DS_IMPORTER
        ASSIMP_BUILD_NO_3D_IMPORTER
        ASSIMP_BUILD_NO_3MF_IMPORTER
        ASSIMP_BUILD_NO_AC_IMPORTER
        ASSIMP_BUILD_NO_AMF_IMPORTER
        ASSIMP_BUILD_NO_ASE_IMPORTER
        ASSIMP_BUILD_NO_ASSBIN_IMPORTER
        ASSIMP_BUILD_NO_B3D_IMPORTER
        ASSIMP_BUILD_NO_BVH_IMPORTER
        ASSIMP_BUILD_NO_C4D_IMPORTER
        ASSIMP_BUILD_NO_COB_IMPORTER
        ASSIMP_BUILD_NO_COMPRESSED_IFC
        ASSIMP_BUILD_NO_CSM_IMPORTER
        ASSIMP_BUILD_NO_DXF_IMPORTER
        ASSIMP_BUILD_NO_EXPORT
        ASSIMP_BUILD_NO_HMP_IMPORTER
        ASSIMP_BUILD_NO_IFC_IMPORTER
        ASSIMP_BUILD_NO_IRRMESH_IMPORTER
        ASSIMP_BUILD_NO_IRR_IMPORTER
        ASSIMP_BUILD_NO_LIMITBONEWEIGHTS_PROCESS
        ASSIMP_BUILD_NO_LWO_IMPORTER
        ASSIMP_BUILD_NO_LWS_IMPORTER
        ASSIMP_BUILD_NO_MD2_IMPORTER
        ASSIMP_BUILD_NO_MD3_IMPORTER
        ASSIMP_BUILD_NO_MD5_IMPORTER
        ASSIMP_BUILD_NO_MDC_IMPORTER
        ASSIMP_BUILD_NO_MDL_IMPORTER
        ASSIMP_BUILD_NO_MMD_IMPORTER
        ASSIMP_BUILD_NO_MS3D_IMPORTER
        ASSIMP_BUILD_NO_NDO_IMPORTER
        ASSIMP_BUILD_NO_NFF_IMPORTER
        ASSIMP_BUILD_NO_OFF_IMPORTER
        ASSIMP_BUILD_NO_OGRE_IMPORTER
        ASSIMP_BUILD_NO_OPENGEX_IMPORTER
        ASSIMP_BUILD_NO_OWN_ZLIB
        ASSIMP_BUILD_NO_PLY_IMPORTER
        ASSIMP_BUILD_NO_Q3BSP_IMPORTER
        ASSIMP_BUILD_NO_Q3D_IMPORTER
        ASSIMP_BUILD_NO_RAW_IMPORTER
        ASSIMP_BUILD_NO_SIB_IMPORTER
        ASSIMP_BUILD_NO_SMD_IMPORTER
        ASSIMP_BUILD_NO_STEP_IMPORTER
        ASSIMP_BUILD_NO_TERRAGEN_IMPORTER
        ASSIMP_BUILD_NO_X3D_IMPORTER
        ASSIMP_BUILD_NO_XGL_IMPORTER
        ASSIMP_BUILD_NO_X_IMPORTER
        ASSIMP_BUILD_NO_BLEND_IMPORTER
        NOUNCRYPT
    INCLUDE_DIRECTORIES
        ../../../3rdparty/assimp
        ../../../3rdparty/assimp/..
        ../../../3rdparty/assimp/contrib/unzip
        ../../../3rdparty/assimp/src
        ../../../3rdparty/assimp/src/code
        ../../../3rdparty/assimp/src/code/CApi
        ../../../3rdparty/assimp/src/code/Collada
        ../../../3rdparty/assimp/src/code/Common
        ../../../3rdparty/assimp/src/code/FBX
        ../../../3rdparty/assimp/src/code/Material
        ../../../3rdparty/assimp/src/code/Obj
        ../../../3rdparty/assimp/src/code/PostProcessing
        ../../../3rdparty/assimp/src/code/STL
        ../../../3rdparty/assimp/src/code/glTF
        ../../../3rdparty/assimp/src/code/glTF2
        ../../../3rdparty/assimp/src/contrib/irrXML
        ../../../3rdparty/assimp/src/contrib/poly2tri
        ../../../3rdparty/assimp/src/contrib/rapidjson/include
        ../../../3rdparty/assimp/src/contrib/utf8cpp/source
        ../../../3rdparty/assimp/src/include
        ../../../3rdparty/assimp/src/include/assimp/Compiler
        ../../../3rdparty/assimp/unzip
)

qt_internal_extend_target(AssimpImporterPlugin CONDITION CMAKE_BUILD_TYPE STREQUAL Debug AND (CMAKE_CROSSCOMPILING OR NOT QT_FEATURE_system_assimp) AND (host_build OR NOT QT_FEATURE_system_assimp)
    DEFINES
        _DEBUG
)

qt_internal_extend_target(AssimpImporterPlugin CONDITION WIN32 AND (CMAKE_CROSSCOMPILING OR NOT QT_FEATURE_system_assimp) AND (host_build OR NOT QT_FEATURE_system_assimp)
    DEFINES
        WindowsStore
        _CRT_SECURE_NO_WARNINGS
)

qt_internal_extend_target(AssimpImporterPlugin CONDITION QT_FEATURE_system_zlib AND NOT QT_FEATURE_system_assimp AND (NOT CMAKE_CROSSCOMPILING OR NOT host_build)
    LIBRARIES
        ZLIB::ZLIB
)

qt_internal_extend_target(AssimpImporterPlugin CONDITION (CMAKE_CROSSCOMPILING OR NOT QT_FEATURE_system_assimp) AND (CMAKE_CROSSCOMPILING OR NOT QT_FEATURE_system_zlib) AND (host_build OR NOT QT_FEATURE_system_assimp) AND (host_build OR NOT QT_FEATURE_system_zlib)
    LIBRARIES
        Qt::ZlibPrivate
)

# special case begin
qt_internal_extend_target(AssimpImporterPlugin CONDITION ICC AND NOT QT_FEATURE_system_assimp AND (NOT CMAKE_CROSSCOMPILING OR NOT host_build)
    COMPILE_OPTIONS
        "-wd310" "-wd68" "-wd858"
)

qt_internal_extend_target(AssimpImporterPlugin CONDITION (GCC OR CLANG) AND NOT QT_FEATURE_system_assimp AND (NOT CMAKE_CROSSCOMPILING OR NOT host_build)
    COMPILE_OPTIONS
        "-Wno-ignored-qualifiers"
        "-Wno-unused-parameter"
        "-Wno-unused-variable"
        "-Wno-deprecated-declarations"
        "-Wno-unused-function"
        "-Wno-reorder"
)

qt_internal_extend_target(AssimpImporterPlugin CONDITION MSVC AND NOT QT_FEATURE_system_assimp AND (NOT CMAKE_CROSSCOMPILING OR NOT host_build)
    COMPILE_OPTIONS
        "-wd4100"
        "-wd4189"
        "-wd4267"
        "-wd4996"
        "-wd4828"
)

qt_internal_extend_target(AssimpImporterPlugin CONDITION CLANG AND NOT QT_FEATURE_system_assimp AND (NOT CMAKE_CROSSCOMPILING OR NOT host_build)
    COMPILE_OPTIONS
        "-Wno-unused-private-field"
)
# special case end

qt_internal_extend_target(AssimpImporterPlugin CONDITION MSVC AND (CMAKE_CROSSCOMPILING OR NOT QT_FEATURE_system_assimp) AND (host_build OR NOT QT_FEATURE_system_assimp)
    DEFINES
        _CRT_SECURE_NO_WARNINGS
        _SCL_SECURE_NO_WARNINGS
    COMPILE_OPTIONS
        /bigobj
)
