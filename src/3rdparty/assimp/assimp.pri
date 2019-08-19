CONFIG(debug, debug|release) : DEFINES+=_DEBUG
CONFIG += exceptions rtti

CONFIG -= precompile_header

win32:DEFINES+=_CRT_SECURE_NO_WARNINGS

qtConfig(system-zlib):!if(cross_compile:host_build): \
    QMAKE_USE_PRIVATE += zlib
else: \
    QT_PRIVATE += zlib-private

DEFINES += \
    ASSIMP_BUILD_NO_X_IMPORTER \
    ASSIMP_BUILD_NO_AMF_IMPORTER \
    ASSIMP_BUILD_NO_3DS_IMPORTER \
    ASSIMP_BUILD_NO_MD3_IMPORTER \
    ASSIMP_BUILD_NO_MDL_IMPORTER \
    ASSIMP_BUILD_NO_MD2_IMPORTER \
    ASSIMP_BUILD_NO_PLY_IMPORTER \
    ASSIMP_BUILD_NO_ASE_IMPORTER \
    ASSIMP_BUILD_NO_HMP_IMPORTER \
    ASSIMP_BUILD_NO_SMD_IMPORTER \
    ASSIMP_BUILD_NO_MDC_IMPORTER \
    ASSIMP_BUILD_NO_MD5_IMPORTER \
    ASSIMP_BUILD_NO_STL_IMPORTER \
    ASSIMP_BUILD_NO_LWO_IMPORTER \
    ASSIMP_BUILD_NO_DXF_IMPORTER \
    ASSIMP_BUILD_NO_NFF_IMPORTER \
    ASSIMP_BUILD_NO_RAW_IMPORTER \
    ASSIMP_BUILD_NO_SIB_IMPORTER \
    ASSIMP_BUILD_NO_OFF_IMPORTER \
    ASSIMP_BUILD_NO_AC_IMPORTER \
    ASSIMP_BUILD_NO_BVH_IMPORTER \
    ASSIMP_BUILD_NO_IRRMESH_IMPORTER \
    ASSIMP_BUILD_NO_IRR_IMPORTER \
    ASSIMP_BUILD_NO_Q3D_IMPORTER \
    ASSIMP_BUILD_NO_B3D_IMPORTER \
    ASSIMP_BUILD_NO_TERRAGEN_IMPORTER \
    ASSIMP_BUILD_NO_CSM_IMPORTER \
    ASSIMP_BUILD_NO_3D_IMPORTER \
    ASSIMP_BUILD_NO_LWS_IMPORTER \
    ASSIMP_BUILD_NO_OGRE_IMPORTER \
    ASSIMP_BUILD_NO_OPENGEX_IMPORTER \
    ASSIMP_BUILD_NO_MS3D_IMPORTER \
    ASSIMP_BUILD_NO_COB_IMPORTER \
    ASSIMP_BUILD_NO_Q3BSP_IMPORTER \
    ASSIMP_BUILD_NO_NDO_IMPORTER \
    ASSIMP_BUILD_NO_IFC_IMPORTER \
    ASSIMP_BUILD_NO_XGL_IMPORTER \
    ASSIMP_BUILD_NO_ASSBIN_IMPORTER \
    ASSIMP_BUILD_NO_C4D_IMPORTER \
    ASSIMP_BUILD_NO_3MF_IMPORTER \
    ASSIMP_BUILD_NO_X3D_IMPORTER \
    ASSIMP_BUILD_NO_MMD_IMPORTER \
    ASSIMP_BUILD_NO_STEP_IMPORTER \
    ASSIMP_BUILD_NO_OWN_ZLIB \
    ASSIMP_BUILD_NO_COMPRESSED_IFC \
    ASSIMP_BUILD_NO_EXPORT \
    ASSIMP_BUILD_BOOST_WORKAROUND \
    NOUNCRYPT

win32: DEFINES += WindowsStore

intel_icc: {
    # warning #310: old-style parameter list (anachronism)
    QMAKE_CFLAGS_WARN_ON += -wd310

    # warning #68: integer conversion resulted in a change of sign
    QMAKE_CFLAGS_WARN_ON += -wd68

    # warning #858: type qualifier on return type is meaningless
    QMAKE_CFLAGS_WARN_ON += -wd858

    QMAKE_CXXFLAGS_WARN_ON += $$QMAKE_CFLAGS_WARN_ON
} else:gcc|clang: {
    # Stop compiler complaining about ignored qualifiers on return types
    QMAKE_CFLAGS_WARN_ON += -Wno-ignored-qualifiers -Wno-unused-parameter -Wno-unused-variable -Wno-deprecated-declarations -Wno-unused-function
    QMAKE_CXXFLAGS_WARN_ON = $$QMAKE_CFLAGS_WARN_ON -Wno-reorder
} else:msvc {
    # Disabled Warnings:
    #   4100: 'identifier' : unreferenced formal parameter
    #   4189: 'identifier' : local variable is initialized but not referenced
    #   4267: coversion from 'size_t' to 'int', possible loss of data
    #   4996: Function call with parameters that may be unsafe
    #   4828: The file contains a character starting at offset 0x167b that
    #         is illegal in the current source character set (codepage 65001)
    QMAKE_CFLAGS_WARN_ON += -wd"4100" -wd"4189" -wd"4267" -wd"4996" -wd"4828"
    QMAKE_CXXFLAGS_WARN_ON = $$QMAKE_CFLAGS_WARN_ON
}

clang: {
    QMAKE_CFLAGS_WARN_ON += -Wno-unused-private-field
    QMAKE_CXXFLAGS_WARN_ON = $$QMAKE_CFLAGS_WARN_ON
}

# Prevents "catastrophic error: Too many segments for object format" for builds using Windows ICC
msvc: QMAKE_CXXFLAGS += /bigobj

CONFIG += warn_on

VPATH += \
    $$PWD \
    $$PWD/code \
    $$PWD/code/res

INCLUDEPATH += \
        $$PWD \
        $$PWD/code \
        $$PWD/include \
        $$PWD/include/assimp/Compiler

HEADERS += \
    revision.h \
    include/assimp/Compiler/pushpack1.h \
    include/assimp/Compiler/poppack1.h \
    include/assimp/Compiler/pstdint.h \
    include/assimp/anim.h \
    include/assimp/ai_assert.h \
    include/assimp/camera.h \
    include/assimp/color4.h \
    include/assimp/color4.inl \
    include/assimp/config.h \
    include/assimp/defs.h \
    include/assimp/Defines.h \
    include/assimp/cfileio.h \
    include/assimp/light.h \
    include/assimp/material.h \
    include/assimp/material.inl \
    include/assimp/matrix3x3.h \
    include/assimp/matrix3x3.inl \
    include/assimp/matrix4x4.h \
    include/assimp/matrix4x4.inl \
    include/assimp/mesh.h \
    include/assimp/pbrmaterial.h \
    include/assimp/postprocess.h \
    include/assimp/quaternion.h \
    include/assimp/quaternion.inl \
    include/assimp/scene.h \
    include/assimp/metadata.h \
    include/assimp/texture.h \
    include/assimp/types.h \
    include/assimp/vector2.h \
    include/assimp/vector2.inl \
    include/assimp/vector3.h \
    include/assimp/vector3.inl \
    include/assimp/version.h \
    include/assimp/cimport.h \
    include/assimp/importerdesc.h \
    include/assimp/Importer.hpp \
    include/assimp/DefaultLogger.hpp \
    include/assimp/ProgressHandler.hpp \
    include/assimp/IOStream.hpp \
    include/assimp/IOSystem.hpp \
    include/assimp/Logger.hpp \
    include/assimp/LogStream.hpp \
    include/assimp/NullLogger.hpp \
    include/assimp/cexport.h \
    include/assimp/Exporter.hpp \
    include/assimp/DefaultIOStream.h \
    include/assimp/DefaultIOSystem.h \
    include/assimp/SceneCombiner.h \
    include/assimp/fast_atof.h \
    include/assimp/qnan.h \
    include/assimp/BaseImporter.h \
    include/assimp/Hash.h \
    include/assimp/MemoryIOWrapper.h \
    include/assimp/ParsingUtils.h \
    include/assimp/StreamReader.h \
    include/assimp/StreamWriter.h \
    include/assimp/StringComparison.h \
    include/assimp/StringUtils.h \
    include/assimp/SGSpatialSort.h \
    include/assimp/GenericProperty.h \
    include/assimp/SpatialSort.h \
    include/assimp/SkeletonMeshBuilder.h \
    include/assimp/SmoothingGroups.h \
    include/assimp/SmoothingGroups.inl \
    include/assimp/StandardShapes.h \
    include/assimp/RemoveComments.h \
    include/assimp/Subdivision.h \
    include/assimp/Vertex.h \
    include/assimp/LineSplitter.h \
    include/assimp/TinyFormatter.h \
    include/assimp/Profiler.h \
    include/assimp/LogAux.h \
    include/assimp/Bitmap.h \
    include/assimp/XMLTools.h \
    include/assimp/IOStreamBuffer.h \
    include/assimp/CreateAnimMesh.h \
    include/assimp/irrXMLWrapper.h \
    include/assimp/BlobIOSystem.h \
    include/assimp/MathFunctions.h \
    include/assimp/Macros.h \
    include/assimp/Exceptional.h \
    include/assimp/ByteSwapper.h \
    include/assimp/DefaultLogger.hpp \
    include/assimp/LogStream.hpp \
    include/assimp/Logger.hpp \
    include/assimp/NullLogger.hpp \
    code/FileLogStream.h \
    code/StdOStreamLogStream.h \
    code/BaseProcess.h \
    code/Importer.h \
    code/ScenePrivate.h \
    code/DefaultProgressHandler.h \
    code/CInterfaceIOWrapper.h \
    code/IFF.h \
    code/VertexTriangleAdjacency.h \
    code/ScenePreprocessor.h \
    code/SplitByBoneCountProcess.h \
    code/TargetAnimation.h \
    code/simd.h \
    code/ColladaHelper.h \
    code/ColladaLoader.h \
    code/ColladaParser.h \
    code/MaterialSystem.h \
    code/ObjFileData.h \
    code/ObjFileImporter.h \
    code/ObjFileMtlImporter.h \
    code/ObjFileParser.h \
    code/ObjTools.h \
    code/BlenderLoader.h \
    code/BlenderDNA.h \
    code/BlenderDNA.inl \
    code/BlenderScene.h \
    code/BlenderSceneGen.h \
    code/BlenderIntermediate.h \
    code/BlenderModifier.h \
    code/BlenderBMesh.h \
    code/BlenderTessellator.h \
    code/BlenderCustomData.h \
    code/FBXCompileConfig.h \
    code/FBXImporter.h \
    code/FBXParser.h \
    code/FBXTokenizer.h \
    code/FBXImportSettings.h \
    code/FBXConverter.h \
    code/FBXUtil.h \
    code/FBXDocument.h \
    code/FBXProperties.h \
    code/FBXMeshGeometry.h \
    code/FBXCommon.h \
    code/CalcTangentsProcess.h \
    code/ComputeUVMappingProcess.h \
    code/ConvertToLHProcess.h \
    code/EmbedTexturesProcess.h \
    code/FindDegenerates.h \
    code/FindInstancesProcess.h \
    code/FindInvalidDataProcess.h \
    code/FixNormalsStep.h \
    code/DropFaceNormalsProcess.h \
    code/GenFaceNormalsProcess.h \
    code/GenVertexNormalsProcess.h \
    code/PretransformVertices.h \
    code/ImproveCacheLocality.h \
    code/JoinVerticesProcess.h \
    code/LimitBoneWeightsProcess.h \
    code/RemoveRedundantMaterials.h \
    code/RemoveVCProcess.h \
    code/SortByPTypeProcess.h \
    code/SplitLargeMeshes.h \
    code/TextureTransform.h \
    code/TriangulateProcess.h \
    code/ValidateDataStructure.h \
    code/OptimizeGraph.h \
    code/OptimizeMeshes.h \
    code/DeboneProcess.h \
    code/ProcessHelper.h \
    code/PolyTools.h \
    code/MakeVerboseFormat.h \
    code/ScaleProcess.h \
    code/glTFAsset.h \
    code/glTFAsset.inl \
    code/glTFAssetWriter.inl \
    code/glTFAssetWriter.h \
    code/glTFImporter.h \
    code/glTF2AssetWriter.h \
    code/glTF2Asset.h \
    code/glTF2Asset.inl \
    code/glTF2AssetWriter.inl \
    code/glTF2Importer.h 

SOURCES += \
    code/Assimp.cpp \
    code/DefaultLogger.cpp \
    code/BaseImporter.cpp \
    code/BaseProcess.cpp \
    code/PostStepRegistry.cpp \
    code/ImporterRegistry.cpp \
    code/DefaultIOStream.cpp \
    code/DefaultIOSystem.cpp \
    code/CInterfaceIOWrapper.cpp \
    code/Importer.cpp \
    code/SGSpatialSort.cpp \
    code/VertexTriangleAdjacency.cpp \
    code/SpatialSort.cpp \
    code/SceneCombiner.cpp \
    code/ScenePreprocessor.cpp \
    code/SkeletonMeshBuilder.cpp \
    code/SplitByBoneCountProcess.cpp \
    code/StandardShapes.cpp \
    code/TargetAnimation.cpp \
    code/RemoveComments.cpp \
    code/Subdivision.cpp \
    code/scene.cpp \
    code/Bitmap.cpp \
    code/Version.cpp \
    code/CreateAnimMesh.cpp \
    code/simd.cpp \
    code/ColladaLoader.cpp \
    code/ColladaParser.cpp \
    code/MaterialSystem.cpp \
    code/ObjFileImporter.cpp \
    code/ObjFileMtlImporter.cpp \
    code/ObjFileParser.cpp \
    code/BlenderLoader.cpp \
    code/BlenderDNA.cpp \
    code/BlenderScene.cpp \
    code/BlenderModifier.cpp \
    code/BlenderBMesh.cpp \
    code/BlenderTessellator.cpp \
    code/BlenderCustomData.cpp \
    code/FBXImporter.cpp \
    code/FBXParser.cpp \
    code/FBXTokenizer.cpp \
    code/FBXConverter.cpp \
    code/FBXUtil.cpp \
    code/FBXDocument.cpp \
    code/FBXProperties.cpp \
    code/FBXMeshGeometry.cpp \
    code/FBXMaterial.cpp \
    code/FBXModel.cpp \
    code/FBXAnimation.cpp \
    code/FBXNodeAttribute.cpp \
    code/FBXDeformer.cpp \
    code/FBXBinaryTokenizer.cpp \
    code/FBXDocumentUtil.cpp \
    code/CalcTangentsProcess.cpp \
    code/ComputeUVMappingProcess.cpp \
    code/ConvertToLHProcess.cpp \
    code/EmbedTexturesProcess.cpp \
    code/FindDegenerates.cpp \
    code/FindInstancesProcess.cpp \
    code/FindInvalidDataProcess.cpp \
    code/FixNormalsStep.cpp \
    code/DropFaceNormalsProcess.cpp \
    code/GenFaceNormalsProcess.cpp \
    code/GenVertexNormalsProcess.cpp \
    code/PretransformVertices.cpp \
    code/ImproveCacheLocality.cpp \
    code/JoinVerticesProcess.cpp \
    code/LimitBoneWeightsProcess.cpp \
    code/RemoveRedundantMaterials.cpp \
    code/RemoveVCProcess.cpp \
    code/SortByPTypeProcess.cpp \
    code/SplitLargeMeshes.cpp \
    code/TextureTransform.cpp \
    code/TriangulateProcess.cpp \
    code/ValidateDataStructure.cpp \
    code/OptimizeGraph.cpp \
    code/OptimizeMeshes.cpp \
    code/DeboneProcess.cpp \
    code/ProcessHelper.cpp \
    code/MakeVerboseFormat.cpp \
    code/ScaleProcess.cpp \
    code/glTFImporter.cpp \
    code/glTF2Importer.cpp

# IrrXML (needed for DAE/Collada support)
HEADERS += \
    contrib/irrXML/CXMLReaderImpl.h \
    contrib/irrXML/heapsort.h \
    contrib/irrXML/irrArray.h \
    contrib/irrXML/irrString.h \
    contrib/irrXML/irrTypes.h \
    contrib/irrXML/irrXML.h

SOURCES += contrib/irrXML/irrXML.cpp

VPATH += $$PWD/contrib/irrXML
INCLUDEPATH += $$PWD/contrib/irrXML

msvc: DEFINES += _SCL_SECURE_NO_WARNINGS _CRT_SECURE_NO_WARNINGS

# rapidjson (needed for GLTF/GLTF2)
VPATH += $$PWD/contrib/rapidjson/include
INCLUDEPATH += $$PWD/contrib/rapidjson/include

# utf8cpp
VPATH += $$PWD/contrib/utf8cpp/source
INCLUDEPATH += $$PWD/contrib/utf8cpp/source

# poly2tri (blender tessellator)
VPATH += $$PWD/contrib/poly2tri
INCLUDEPATH += $$PWD/contrib/poly2tri

HEADERS += \
    contrib/poly2tri/poly2tri/common/shapes.h \
    contrib/poly2tri/poly2tri/common/utils.h \
    contrib/poly2tri/poly2tri/sweep/advancing_front.h \
    contrib/poly2tri/poly2tri/sweep/cdt.h \
    contrib/poly2tri/poly2tri/sweep/sweep.h \
    contrib/poly2tri/poly2tri/sweep/sweep_context.h

SOURCES += \
    contrib/poly2tri/poly2tri/common/shapes.cc \
    contrib/poly2tri/poly2tri/sweep/advancing_front.cc \
    contrib/poly2tri/poly2tri/sweep/cdt.cc \
    contrib/poly2tri/poly2tri/sweep/sweep.cc \
    contrib/poly2tri/poly2tri/sweep/sweep_context.cc
