CONFIG(debug, debug|release) : DEFINES+=_DEBUG
CONFIG += exceptions rtti

CONFIG -= precompile_header

ASSIMP_SRC = $$PWD/src

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
    $$ASSIMP_SRC \
    $$ASSIMP_SRC/code \
    $$ASSIMP_SRC/code/res

INCLUDEPATH += \
        $$PWD \
        $$PWD/.. \
        $$ASSIMP_SRC \
        $$ASSIMP_SRC/code \
        $$ASSIMP_SRC/include \
        $$ASSIMP_SRC/include/assimp/Compiler

HEADERS += \
    $$PWD/config.h \
    $$PWD/revision.h

HEADERS += \
    $$ASSIMP_SRC/include/assimp/Compiler/pushpack1.h \
    $$ASSIMP_SRC/include/assimp/Compiler/poppack1.h \
    $$ASSIMP_SRC/include/assimp/Compiler/pstdint.h \
    $$ASSIMP_SRC/include/assimp/anim.h \
    $$ASSIMP_SRC/include/assimp/ai_assert.h \
    $$ASSIMP_SRC/include/assimp/camera.h \
    $$ASSIMP_SRC/include/assimp/color4.h \
    $$ASSIMP_SRC/include/assimp/color4.inl \
    $$ASSIMP_SRC/include/assimp/defs.h \
    $$ASSIMP_SRC/include/assimp/Defines.h \
    $$ASSIMP_SRC/include/assimp/cfileio.h \
    $$ASSIMP_SRC/include/assimp/light.h \
    $$ASSIMP_SRC/include/assimp/material.h \
    $$ASSIMP_SRC/include/assimp/material.inl \
    $$ASSIMP_SRC/include/assimp/matrix3x3.h \
    $$ASSIMP_SRC/include/assimp/matrix3x3.inl \
    $$ASSIMP_SRC/include/assimp/matrix4x4.h \
    $$ASSIMP_SRC/include/assimp/matrix4x4.inl \
    $$ASSIMP_SRC/include/assimp/mesh.h \
    $$ASSIMP_SRC/include/assimp/pbrmaterial.h \
    $$ASSIMP_SRC/include/assimp/postprocess.h \
    $$ASSIMP_SRC/include/assimp/quaternion.h \
    $$ASSIMP_SRC/include/assimp/quaternion.inl \
    $$ASSIMP_SRC/include/assimp/scene.h \
    $$ASSIMP_SRC/include/assimp/metadata.h \
    $$ASSIMP_SRC/include/assimp/texture.h \
    $$ASSIMP_SRC/include/assimp/types.h \
    $$ASSIMP_SRC/include/assimp/vector2.h \
    $$ASSIMP_SRC/include/assimp/vector2.inl \
    $$ASSIMP_SRC/include/assimp/vector3.h \
    $$ASSIMP_SRC/include/assimp/vector3.inl \
    $$ASSIMP_SRC/include/assimp/version.h \
    $$ASSIMP_SRC/include/assimp/cimport.h \
    $$ASSIMP_SRC/include/assimp/importerdesc.h \
    $$ASSIMP_SRC/include/assimp/Importer.hpp \
    $$ASSIMP_SRC/include/assimp/DefaultLogger.hpp \
    $$ASSIMP_SRC/include/assimp/ProgressHandler.hpp \
    $$ASSIMP_SRC/include/assimp/IOStream.hpp \
    $$ASSIMP_SRC/include/assimp/IOSystem.hpp \
    $$ASSIMP_SRC/include/assimp/Logger.hpp \
    $$ASSIMP_SRC/include/assimp/LogStream.hpp \
    $$ASSIMP_SRC/include/assimp/NullLogger.hpp \
    $$ASSIMP_SRC/include/assimp/cexport.h \
    $$ASSIMP_SRC/include/assimp/Exporter.hpp \
    $$ASSIMP_SRC/include/assimp/DefaultIOStream.h \
    $$ASSIMP_SRC/include/assimp/DefaultIOSystem.h \
    $$ASSIMP_SRC/include/assimp/SceneCombiner.h \
    $$ASSIMP_SRC/include/assimp/fast_atof.h \
    $$ASSIMP_SRC/include/assimp/qnan.h \
    $$ASSIMP_SRC/include/assimp/BaseImporter.h \
    $$ASSIMP_SRC/include/assimp/Hash.h \
    $$ASSIMP_SRC/include/assimp/MemoryIOWrapper.h \
    $$ASSIMP_SRC/include/assimp/ParsingUtils.h \
    $$ASSIMP_SRC/include/assimp/StreamReader.h \
    $$ASSIMP_SRC/include/assimp/StreamWriter.h \
    $$ASSIMP_SRC/include/assimp/StringComparison.h \
    $$ASSIMP_SRC/include/assimp/StringUtils.h \
    $$ASSIMP_SRC/include/assimp/SGSpatialSort.h \
    $$ASSIMP_SRC/include/assimp/GenericProperty.h \
    $$ASSIMP_SRC/include/assimp/SpatialSort.h \
    $$ASSIMP_SRC/include/assimp/SkeletonMeshBuilder.h \
    $$ASSIMP_SRC/include/assimp/SmoothingGroups.h \
    $$ASSIMP_SRC/include/assimp/SmoothingGroups.inl \
    $$ASSIMP_SRC/include/assimp/StandardShapes.h \
    $$ASSIMP_SRC/include/assimp/RemoveComments.h \
    $$ASSIMP_SRC/include/assimp/Subdivision.h \
    $$ASSIMP_SRC/include/assimp/Vertex.h \
    $$ASSIMP_SRC/include/assimp/LineSplitter.h \
    $$ASSIMP_SRC/include/assimp/TinyFormatter.h \
    $$ASSIMP_SRC/include/assimp/Profiler.h \
    $$ASSIMP_SRC/include/assimp/LogAux.h \
    $$ASSIMP_SRC/include/assimp/Bitmap.h \
    $$ASSIMP_SRC/include/assimp/XMLTools.h \
    $$ASSIMP_SRC/include/assimp/IOStreamBuffer.h \
    $$ASSIMP_SRC/include/assimp/CreateAnimMesh.h \
    $$ASSIMP_SRC/include/assimp/irrXMLWrapper.h \
    $$ASSIMP_SRC/include/assimp/BlobIOSystem.h \
    $$ASSIMP_SRC/include/assimp/MathFunctions.h \
    $$ASSIMP_SRC/include/assimp/Macros.h \
    $$ASSIMP_SRC/include/assimp/Exceptional.h \
    $$ASSIMP_SRC/include/assimp/ByteSwapper.h \
    $$ASSIMP_SRC/include/assimp/DefaultLogger.hpp \
    $$ASSIMP_SRC/include/assimp/LogStream.hpp \
    $$ASSIMP_SRC/include/assimp/Logger.hpp \
    $$ASSIMP_SRC/include/assimp/NullLogger.hpp \
    $$ASSIMP_SRC/code/FileLogStream.h \
    $$ASSIMP_SRC/code/StdOStreamLogStream.h \
    $$ASSIMP_SRC/code/BaseProcess.h \
    $$ASSIMP_SRC/code/Importer.h \
    $$ASSIMP_SRC/code/ScenePrivate.h \
    $$ASSIMP_SRC/code/DefaultProgressHandler.h \
    $$ASSIMP_SRC/code/CInterfaceIOWrapper.h \
    $$ASSIMP_SRC/code/IFF.h \
    $$ASSIMP_SRC/code/VertexTriangleAdjacency.h \
    $$ASSIMP_SRC/code/ScenePreprocessor.h \
    $$ASSIMP_SRC/code/SplitByBoneCountProcess.h \
    $$ASSIMP_SRC/code/TargetAnimation.h \
    $$ASSIMP_SRC/code/simd.h \
    $$ASSIMP_SRC/code/ColladaHelper.h \
    $$ASSIMP_SRC/code/ColladaLoader.h \
    $$ASSIMP_SRC/code/ColladaParser.h \
    $$ASSIMP_SRC/code/MaterialSystem.h \
    $$ASSIMP_SRC/code/ObjFileData.h \
    $$ASSIMP_SRC/code/ObjFileImporter.h \
    $$ASSIMP_SRC/code/ObjFileMtlImporter.h \
    $$ASSIMP_SRC/code/ObjFileParser.h \
    $$ASSIMP_SRC/code/ObjTools.h \
    $$ASSIMP_SRC/code/BlenderLoader.h \
    $$ASSIMP_SRC/code/BlenderDNA.h \
    $$ASSIMP_SRC/code/BlenderDNA.inl \
    $$ASSIMP_SRC/code/BlenderScene.h \
    $$ASSIMP_SRC/code/BlenderSceneGen.h \
    $$ASSIMP_SRC/code/BlenderIntermediate.h \
    $$ASSIMP_SRC/code/BlenderModifier.h \
    $$ASSIMP_SRC/code/BlenderBMesh.h \
    $$ASSIMP_SRC/code/BlenderTessellator.h \
    $$ASSIMP_SRC/code/BlenderCustomData.h \
    $$ASSIMP_SRC/code/FBXCompileConfig.h \
    $$ASSIMP_SRC/code/FBXImporter.h \
    $$ASSIMP_SRC/code/FBXParser.h \
    $$ASSIMP_SRC/code/FBXTokenizer.h \
    $$ASSIMP_SRC/code/FBXImportSettings.h \
    $$ASSIMP_SRC/code/FBXConverter.h \
    $$ASSIMP_SRC/code/FBXUtil.h \
    $$ASSIMP_SRC/code/FBXDocument.h \
    $$ASSIMP_SRC/code/FBXProperties.h \
    $$ASSIMP_SRC/code/FBXMeshGeometry.h \
    $$ASSIMP_SRC/code/FBXCommon.h \
    $$ASSIMP_SRC/code/CalcTangentsProcess.h \
    $$ASSIMP_SRC/code/ComputeUVMappingProcess.h \
    $$ASSIMP_SRC/code/ConvertToLHProcess.h \
    $$ASSIMP_SRC/code/EmbedTexturesProcess.h \
    $$ASSIMP_SRC/code/FindDegenerates.h \
    $$ASSIMP_SRC/code/FindInstancesProcess.h \
    $$ASSIMP_SRC/code/FindInvalidDataProcess.h \
    $$ASSIMP_SRC/code/FixNormalsStep.h \
    $$ASSIMP_SRC/code/DropFaceNormalsProcess.h \
    $$ASSIMP_SRC/code/GenFaceNormalsProcess.h \
    $$ASSIMP_SRC/code/GenVertexNormalsProcess.h \
    $$ASSIMP_SRC/code/PretransformVertices.h \
    $$ASSIMP_SRC/code/ImproveCacheLocality.h \
    $$ASSIMP_SRC/code/JoinVerticesProcess.h \
    $$ASSIMP_SRC/code/LimitBoneWeightsProcess.h \
    $$ASSIMP_SRC/code/RemoveRedundantMaterials.h \
    $$ASSIMP_SRC/code/RemoveVCProcess.h \
    $$ASSIMP_SRC/code/SortByPTypeProcess.h \
    $$ASSIMP_SRC/code/SplitLargeMeshes.h \
    $$ASSIMP_SRC/code/TextureTransform.h \
    $$ASSIMP_SRC/code/TriangulateProcess.h \
    $$ASSIMP_SRC/code/ValidateDataStructure.h \
    $$ASSIMP_SRC/code/OptimizeGraph.h \
    $$ASSIMP_SRC/code/OptimizeMeshes.h \
    $$ASSIMP_SRC/code/DeboneProcess.h \
    $$ASSIMP_SRC/code/ProcessHelper.h \
    $$ASSIMP_SRC/code/PolyTools.h \
    $$ASSIMP_SRC/code/MakeVerboseFormat.h \
    $$ASSIMP_SRC/code/ScaleProcess.h \
    $$ASSIMP_SRC/code/glTFAsset.h \
    $$ASSIMP_SRC/code/glTFAsset.inl \
    $$ASSIMP_SRC/code/glTFAssetWriter.inl \
    $$ASSIMP_SRC/code/glTFAssetWriter.h \
    $$ASSIMP_SRC/code/glTFImporter.h \
    $$ASSIMP_SRC/code/glTF2AssetWriter.h \
    $$ASSIMP_SRC/code/glTF2Asset.h \
    $$ASSIMP_SRC/code/glTF2Asset.inl \
    $$ASSIMP_SRC/code/glTF2AssetWriter.inl \
    $$ASSIMP_SRC/code/glTF2Importer.h

SOURCES += \
    $$ASSIMP_SRC/code/Assimp.cpp \
    $$ASSIMP_SRC/code/DefaultLogger.cpp \
    $$ASSIMP_SRC/code/BaseImporter.cpp \
    $$ASSIMP_SRC/code/BaseProcess.cpp \
    $$ASSIMP_SRC/code/PostStepRegistry.cpp \
    $$ASSIMP_SRC/code/ImporterRegistry.cpp \
    $$ASSIMP_SRC/code/DefaultIOStream.cpp \
    $$ASSIMP_SRC/code/DefaultIOSystem.cpp \
    $$ASSIMP_SRC/code/CInterfaceIOWrapper.cpp \
    $$ASSIMP_SRC/code/Importer.cpp \
    $$ASSIMP_SRC/code/SGSpatialSort.cpp \
    $$ASSIMP_SRC/code/VertexTriangleAdjacency.cpp \
    $$ASSIMP_SRC/code/SpatialSort.cpp \
    $$ASSIMP_SRC/code/SceneCombiner.cpp \
    $$ASSIMP_SRC/code/ScenePreprocessor.cpp \
    $$ASSIMP_SRC/code/SkeletonMeshBuilder.cpp \
    $$ASSIMP_SRC/code/SplitByBoneCountProcess.cpp \
    $$ASSIMP_SRC/code/StandardShapes.cpp \
    $$ASSIMP_SRC/code/TargetAnimation.cpp \
    $$ASSIMP_SRC/code/RemoveComments.cpp \
    $$ASSIMP_SRC/code/Subdivision.cpp \
    $$ASSIMP_SRC/code/scene.cpp \
    $$ASSIMP_SRC/code/Bitmap.cpp \
    $$ASSIMP_SRC/code/Version.cpp \
    $$ASSIMP_SRC/code/CreateAnimMesh.cpp \
    $$ASSIMP_SRC/code/simd.cpp \
    $$ASSIMP_SRC/code/ColladaLoader.cpp \
    $$ASSIMP_SRC/code/ColladaParser.cpp \
    $$ASSIMP_SRC/code/MaterialSystem.cpp \
    $$ASSIMP_SRC/code/ObjFileImporter.cpp \
    $$ASSIMP_SRC/code/ObjFileMtlImporter.cpp \
    $$ASSIMP_SRC/code/ObjFileParser.cpp \
    $$ASSIMP_SRC/code/BlenderLoader.cpp \
    $$ASSIMP_SRC/code/BlenderDNA.cpp \
    $$ASSIMP_SRC/code/BlenderScene.cpp \
    $$ASSIMP_SRC/code/BlenderModifier.cpp \
    $$ASSIMP_SRC/code/BlenderBMesh.cpp \
    $$ASSIMP_SRC/code/BlenderTessellator.cpp \
    $$ASSIMP_SRC/code/BlenderCustomData.cpp \
    $$ASSIMP_SRC/code/FBXImporter.cpp \
    $$ASSIMP_SRC/code/FBXParser.cpp \
    $$ASSIMP_SRC/code/FBXTokenizer.cpp \
    $$ASSIMP_SRC/code/FBXConverter.cpp \
    $$ASSIMP_SRC/code/FBXUtil.cpp \
    $$ASSIMP_SRC/code/FBXDocument.cpp \
    $$ASSIMP_SRC/code/FBXProperties.cpp \
    $$ASSIMP_SRC/code/FBXMeshGeometry.cpp \
    $$ASSIMP_SRC/code/FBXMaterial.cpp \
    $$ASSIMP_SRC/code/FBXModel.cpp \
    $$ASSIMP_SRC/code/FBXAnimation.cpp \
    $$ASSIMP_SRC/code/FBXNodeAttribute.cpp \
    $$ASSIMP_SRC/code/FBXDeformer.cpp \
    $$ASSIMP_SRC/code/FBXBinaryTokenizer.cpp \
    $$ASSIMP_SRC/code/FBXDocumentUtil.cpp \
    $$ASSIMP_SRC/code/CalcTangentsProcess.cpp \
    $$ASSIMP_SRC/code/ComputeUVMappingProcess.cpp \
    $$ASSIMP_SRC/code/ConvertToLHProcess.cpp \
    $$ASSIMP_SRC/code/EmbedTexturesProcess.cpp \
    $$ASSIMP_SRC/code/FindDegenerates.cpp \
    $$ASSIMP_SRC/code/FindInstancesProcess.cpp \
    $$ASSIMP_SRC/code/FindInvalidDataProcess.cpp \
    $$ASSIMP_SRC/code/FixNormalsStep.cpp \
    $$ASSIMP_SRC/code/DropFaceNormalsProcess.cpp \
    $$ASSIMP_SRC/code/GenFaceNormalsProcess.cpp \
    $$ASSIMP_SRC/code/GenVertexNormalsProcess.cpp \
    $$ASSIMP_SRC/code/PretransformVertices.cpp \
    $$ASSIMP_SRC/code/ImproveCacheLocality.cpp \
    $$ASSIMP_SRC/code/JoinVerticesProcess.cpp \
    $$ASSIMP_SRC/code/LimitBoneWeightsProcess.cpp \
    $$ASSIMP_SRC/code/RemoveRedundantMaterials.cpp \
    $$ASSIMP_SRC/code/RemoveVCProcess.cpp \
    $$ASSIMP_SRC/code/SortByPTypeProcess.cpp \
    $$ASSIMP_SRC/code/SplitLargeMeshes.cpp \
    $$ASSIMP_SRC/code/TextureTransform.cpp \
    $$ASSIMP_SRC/code/TriangulateProcess.cpp \
    $$ASSIMP_SRC/code/ValidateDataStructure.cpp \
    $$ASSIMP_SRC/code/OptimizeGraph.cpp \
    $$ASSIMP_SRC/code/OptimizeMeshes.cpp \
    $$ASSIMP_SRC/code/DeboneProcess.cpp \
    $$ASSIMP_SRC/code/ProcessHelper.cpp \
    $$ASSIMP_SRC/code/MakeVerboseFormat.cpp \
    $$ASSIMP_SRC/code/ScaleProcess.cpp \
    $$ASSIMP_SRC/code/glTFImporter.cpp \
    $$ASSIMP_SRC/code/glTF2Importer.cpp

# IrrXML (needed for DAE/Collada support)
HEADERS += \
    $$ASSIMP_SRC/contrib/irrXML/CXMLReaderImpl.h \
    $$ASSIMP_SRC/contrib/irrXML/heapsort.h \
    $$ASSIMP_SRC/contrib/irrXML/irrArray.h \
    $$ASSIMP_SRC/contrib/irrXML/irrString.h \
    $$ASSIMP_SRC/contrib/irrXML/irrTypes.h \
    $$ASSIMP_SRC/contrib/irrXML/irrXML.h

SOURCES += $$ASSIMP_SRC/contrib/irrXML/irrXML.cpp

VPATH += $$ASSIMP_SRC/contrib/irrXML
INCLUDEPATH += $$ASSIMP_SRC/contrib/irrXML

msvc: DEFINES += _SCL_SECURE_NO_WARNINGS _CRT_SECURE_NO_WARNINGS

# rapidjson (needed for GLTF/GLTF2)
VPATH += $$ASSIMP_SRC/contrib/rapidjson/include
INCLUDEPATH += $$ASSIMP_SRC/contrib/rapidjson/include

# utf8cpp
VPATH += $$ASSIMP_SRC/contrib/utf8cpp/source
INCLUDEPATH += $$ASSIMP_SRC/contrib/utf8cpp/source

# poly2tri (blender tessellator)
VPATH += $$ASSIMP_SRC/contrib/poly2tri
INCLUDEPATH += $$ASSIMP_SRC/contrib/poly2tri

HEADERS += \
    $$ASSIMP_SRC/contrib/poly2tri/poly2tri/common/shapes.h \
    $$ASSIMP_SRC/contrib/poly2tri/poly2tri/common/utils.h \
    $$ASSIMP_SRC/contrib/poly2tri/poly2tri/sweep/advancing_front.h \
    $$ASSIMP_SRC/contrib/poly2tri/poly2tri/sweep/cdt.h \
    $$ASSIMP_SRC/contrib/poly2tri/poly2tri/sweep/sweep.h \
    $$ASSIMP_SRC/contrib/poly2tri/poly2tri/sweep/sweep_context.h

SOURCES += \
    $$ASSIMP_SRC/contrib/poly2tri/poly2tri/common/shapes.cc \
    $$ASSIMP_SRC/contrib/poly2tri/poly2tri/sweep/advancing_front.cc \
    $$ASSIMP_SRC/contrib/poly2tri/poly2tri/sweep/cdt.cc \
    $$ASSIMP_SRC/contrib/poly2tri/poly2tri/sweep/sweep.cc \
    $$ASSIMP_SRC/contrib/poly2tri/poly2tri/sweep/sweep_context.cc
