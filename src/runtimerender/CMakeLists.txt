# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from runtimerender.pro.

#####################################################################
## Quick3DRuntimeRender Module:
#####################################################################

# Handle the conditional finding of either system zlib or qt zlib.
if(NOT QT_FEATURE_system_zlib)
    find_package(Qt6 COMPONENTS ZlibPrivate)
elseif(NOT TARGET WrapZLIB::WrapZLIB)
    qt_find_package(WrapZLIB PROVIDED_TARGETS WrapZLIB::WrapZLIB)
endif()

qt_internal_add_module(Quick3DRuntimeRender
    SOURCES
        graphobjects/qssgrendercamera.cpp graphobjects/qssgrendercamera_p.h
        graphobjects/qssgrendercustommaterial_p.h
        graphobjects/qssgrenderdefaultmaterial.cpp graphobjects/qssgrenderdefaultmaterial_p.h
        graphobjects/qssgrendereffect.cpp graphobjects/qssgrendereffect_p.h
        graphobjects/qssgrendergeometry.cpp graphobjects/qssgrendergeometry_p.h
        graphobjects/qssgrendergraphobject.cpp graphobjects/qssgrendergraphobject_p.h
        graphobjects/qssgrenderimage.cpp graphobjects/qssgrenderimage_p.h
        graphobjects/qssgrenderinstancetable.cpp graphobjects/qssgrenderinstancetable_p.h
        graphobjects/qssgrenderitem2d.cpp graphobjects/qssgrenderitem2d_p.h
        graphobjects/qssgrenderjoint.cpp graphobjects/qssgrenderjoint_p.h
        graphobjects/qssgrenderlayer.cpp graphobjects/qssgrenderlayer_p.h
        graphobjects/qssgrenderlight.cpp graphobjects/qssgrenderlight_p.h
        graphobjects/qssgrendermodel.cpp graphobjects/qssgrendermodel_p.h
        graphobjects/qssgrendernode.cpp graphobjects/qssgrendernode_p.h
        graphobjects/qssgrenderskeleton.cpp graphobjects/qssgrenderskeleton_p.h
        graphobjects/qssgrenderskin.cpp graphobjects/qssgrenderskin_p.h
        graphobjects/qssgrendertexturedata.cpp graphobjects/qssgrendertexturedata_p.h
        graphobjects/qssgrendermorphtarget.cpp graphobjects/qssgrendermorphtarget_p.h
        graphobjects/qssgrenderresourceloader.cpp graphobjects/qssgrenderresourceloader_p.h
        graphobjects/qssgrenderreflectionprobe.cpp graphobjects/qssgrenderreflectionprobe_p.h
        qssgperframeallocator_p.h
        qssgrenderableimage_p.h
        qssgrenderclippingfrustum.cpp qssgrenderclippingfrustum_p.h
        graphobjects/qssgrenderparticles.cpp graphobjects/qssgrenderparticles_p.h
        qssgrendercommands.cpp qssgrendercommands_p.h
        qssgrendercontextcore.cpp qssgrendercontextcore_p.h
        qssgrenderdefaultmaterialshadergenerator.cpp qssgrenderdefaultmaterialshadergenerator_p.h
        qssgrendererutil_p.h
        qssgrenderimagetexture_p.h
        qssgrendermaterialshadergenerator_p.h
        qssgrendermesh_p.h
        qssgrenderray.cpp qssgrenderray_p.h
        qssgrendershadercache.cpp qssgrendershadercache_p.h
        qssgrendershadercodegenerator.cpp qssgrendershadercodegenerator_p.h
        qssgrendershaderkeys_p.h
        qssgrendershadermetadata.cpp qssgrendershadermetadata_p.h
        qssgrendershadowmap.cpp qssgrendershadowmap_p.h
        qssgrenderreflectionmap.cpp qssgrenderreflectionmap_p.h
        qssgrenderpickresult_p.h
        qssgrhiparticles.cpp qssgrhiparticles_p.h
        qssgrhicontext.cpp qssgrhicontext_p.h
        qssgrhicustommaterialsystem.cpp qssgrhicustommaterialsystem_p.h
        qssgrhieffectsystem.cpp qssgrhieffectsystem_p.h
        qssgrhiquadrenderer.cpp qssgrhiquadrenderer_p.h
        qssgruntimerenderlogging.cpp qssgruntimerenderlogging_p.h
        qssgshadermapkey_p.h
        qssgshadermaterialadapter.cpp qssgshadermaterialadapter_p.h
        qssgshaderresourcemergecontext_p.h
        qtquick3druntimerenderglobal_p.h
        rendererimpl/qssgrenderableobjects.cpp rendererimpl/qssgrenderableobjects_p.h
        rendererimpl/qssgrenderer.cpp rendererimpl/qssgrenderer_p.h
        rendererimpl/qssglayerrenderdata_p.h
        rendererimpl/qssglayerrenderdata.cpp
        rendererimpl/qssglightmapper.cpp rendererimpl/qssglightmapper_p.h
        rendererimpl/qssgrendererimplshaders_rhi.cpp
        rendererimpl/qssgvertexpipelineimpl.cpp rendererimpl/qssgvertexpipelineimpl_p.h
        rendererimpl/qssgrenderpass_p.h rendererimpl/qssgrenderpass.cpp
        resourcemanager/qssgrenderbuffermanager.cpp resourcemanager/qssgrenderbuffermanager_p.h
        resourcemanager/qssgrenderloadedtexture.cpp resourcemanager/qssgrenderloadedtexture_p.h
        resourcemanager/qssgrendershaderlibrarymanager.cpp resourcemanager/qssgrendershaderlibrarymanager_p.h
        rendererimpl/qssgcputonemapper_p.h
        extensionapi/qssgrenderextensions_p.h extensionapi/qssgrenderextensions.cpp
        extensionapi/qssgrenderhelpers_p.h extensionapi/qssgrenderhelpers.cpp
        qssgdebugdrawsystem_p.h qssgdebugdrawsystem.cpp
    NO_UNITY_BUILD_SOURCES
        rendererimpl/qssglightmapper.cpp              # avoiding possible clash with tinyexr' macros
        resourcemanager/qssgrenderbuffermanager.cpp # redefinition of 'cube' (from qssgrenderreflectionmap.cpp)
        resourcemanager/qssgrenderloadedtexture.cpp   # avoiding possible clash with tinyexr' macros
    DEFINES
        QT_BUILD_QUICK3DRUNTIMERENDER_LIB
    LIBRARIES
        Qt::Quick3DUtilsPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Quick
        Qt::Quick3DUtils
    PRIVATE_MODULE_INTERFACE
        Qt::Quick3DUtilsPrivate
        Qt::QuickPrivate
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)

qt_internal_extend_target(Quick3DRuntimeRender CONDITION TARGET Qt::ShaderTools
    PUBLIC_LIBRARIES
        Qt::ShaderTools
    LIBRARIES
        Qt::ShaderToolsPrivate
    DEFINES
        QT_QUICK3D_HAS_RUNTIME_SHADERS
)

qt_internal_extend_target(Quick3DRuntimeRender CONDITION QT_QUICK3D_HAS_EMBREE
    PUBLIC_LIBRARIES
        Qt::Concurrent
    LIBRARIES
        Qt::BundledEmbree
    DEFINES
        QT_QUICK3D_HAS_LIGHTMAPPER
)

# Resources:
set(res_resource_files
    "res/effectlib/bsdf.glsllib"
    "res/effectlib/defaultMaterialBumpNoLod.glsllib"
    "res/effectlib/defaultMaterialFresnel.glsllib"
    "res/effectlib/depthpass.glsllib"
    "res/effectlib/funccalculatePointLightAttenuation.glsllib"
    "res/effectlib/funcdiffuseBurleyBSDF.glsllib"
    "res/effectlib/funcdiffuseReflectionBSDF.glsllib"
    "res/effectlib/funcdiffuseReflectionWrapBSDF.glsllib"
    "res/effectlib/funcgetTransformedUVCoords.glsllib"
    "res/effectlib/funclightmap.glsllib"
    "res/effectlib/funcsampleLightVars.glsllib"
    "res/effectlib/funcsampleNormalTexture.glsllib"
    "res/effectlib/funcspecularBSDF.glsllib"
    "res/effectlib/funcspecularGGXBSDF.glsllib"
    "res/effectlib/physGlossyBSDF.glsllib"
    "res/effectlib/principledMaterialFresnel.glsllib"
    "res/effectlib/sampleProbe.glsllib"
    "res/effectlib/sampleReflectionProbe.glsllib"
    "res/effectlib/shadowMapping.glsllib"
    "res/effectlib/skinanim.glsllib"
    "res/effectlib/ssao.glsllib"
    "res/effectlib/tonemapping.glsllib"
    "res/effectlib/transmission.glsllib"
    "res/effectlib/viewProperties.glsllib"
    "res/effectlib/morphanim.glsllib"
    "res/effectlib/morphanim_custom.glsllib"
    "res/effectlib/parallaxMapping.glsllib"
    "res/effectlib/particles.glsllib"
    "res/effectlib/fog.glsllib"
    "res/primitives/Cone.mesh"
    "res/primitives/Cube.mesh"
    "res/primitives/Cylinder.mesh"
    "res/primitives/Rectangle.mesh"
    "res/primitives/Sphere.mesh"
)

qt_internal_add_resource(Quick3DRuntimeRender "res"
    PREFIX
        "/"
    FILES
        ${res_resource_files}
)

# special case begin

qt_internal_extend_target(Quick3DRuntimeRender
    INCLUDE_DIRECTORIES
        ../3rdparty/tinyexr
        ../3rdparty/embree/include
    NO_UNITY_BUILD_SOURCES
        ../3rdparty/tinyexr/tinyexr.h
        ../3rdparty/embree/include/embree3/rtcore.h
        ../3rdparty/embree/include/embree3/rtcore_buffer.h
        ../3rdparty/embree/include/embree3/rtcore_builder.h
        ../3rdparty/embree/include/embree3/rtcore_common.h
        ../3rdparty/embree/include/embree3/rtcore_config.h
        ../3rdparty/embree/include/embree3/rtcore_device.h
        ../3rdparty/embree/include/embree3/rtcore_geometry.h
        ../3rdparty/embree/include/embree3/rtcore_quaternion.h
        ../3rdparty/embree/include/embree3/rtcore_ray.h
        ../3rdparty/embree/include/embree3/rtcore_scene.h
)

qt_internal_extend_target(Quick3DRuntimeRender CONDITION QT_FEATURE_system_zlib
    LIBRARIES
        WrapZLIB::WrapZLIB
)

qt_internal_extend_target(Quick3DRuntimeRender CONDITION NOT QT_FEATURE_system_zlib
    LIBRARIES
        Qt6::ZlibPrivate
)

qt_internal_generate_tracepoints(Quick3DRuntimeRender quick3d
    SOURCES
        qssgrhicontext.cpp
        resourcemanager/qssgrenderbuffermanager.cpp
        qssgrendershadercache.cpp
        qssgrhicustommaterialsystem.cpp
)

# default settings are GLSL "100es,120,150" HLSL "50" MSL "12"
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders"
    SILENT
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/"
    FILES
        res/rhishaders/orthoshadowblurx.vert
        res/rhishaders/orthoshadowblurx.frag
        res/rhishaders/orthoshadowblury.vert
        res/rhishaders/orthoshadowblury.frag
        res/rhishaders/texturedquad.vert
        res/rhishaders/texturedquad.frag
        res/rhishaders/environmentmap.vert
        res/rhishaders/environmentmap.frag
        res/rhishaders/debugobject.vert
        res/rhishaders/debugobject.frag
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_pertarget"
    SILENT
    PRECOMPILE
    OPTIMIZED
    PERTARGETCOMPILE
    PREFIX
        "/"
    FILES
        res/rhishaders/ssaaresolve.vert
        res/rhishaders/ssaaresolve.frag
        res/rhishaders/progressiveaa.vert
        res/rhishaders/progressiveaa.frag
        res/rhishaders/simplequad.vert
        res/rhishaders/simplequad.frag
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_es3"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,120,330"
    PREFIX
        "/"
    FILES
        res/rhishaders/cubeshadowblurx.vert
        res/rhishaders/cubeshadowblurx.frag
        res/rhishaders/cubeshadowblury.vert
        res/rhishaders/cubeshadowblury.frag
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_es3_pertarget"
    SILENT
    PRECOMPILE
    OPTIMIZED
    PERTARGETCOMPILE
    GLSL "300es,120,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/lightmapdilate.vert
        res/rhishaders/lightmapdilate.frag
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_es3_gl3"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/ssao.vert
        res/rhishaders/ssao.frag
        res/rhishaders/skybox.vert
        res/rhishaders/skybox.frag
        res/rhishaders/environmentmapprefilter.vert
        res/rhishaders/environmentmapprefilter.frag
        res/rhishaders/reflectionprobeprefilter.vert
        res/rhishaders/reflectionprobeprefilter.frag
        res/rhishaders/skyboxcube.vert
        res/rhishaders/skyboxcube.frag
        res/rhishaders/grid.frag
        res/rhishaders/grid.vert
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_lightprobe_rgbe"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/environmentmapprefilter.vert
        res/rhishaders/environmentmapprefilter.frag
    OUTPUTS
        res/rhishaders/environmentmapprefilter_rgbe.vert.qsb
        res/rhishaders/environmentmapprefilter_rgbe.frag.qsb
    DEFINES
        QSSG_ENABLE_RGBE_LIGHT_PROBE=1
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_skybox_hdr_none"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/skybox.vert
        res/rhishaders/skybox.frag
    OUTPUTS
        res/rhishaders/skybox_hdr_none.vert.qsb
        res/rhishaders/skybox_hdr_none.frag.qsb
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_skybox_rgbe_none"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/skybox.vert
        res/rhishaders/skybox.frag
    OUTPUTS
        res/rhishaders/skybox_rgbe_none.vert.qsb
        res/rhishaders/skybox_rgbe_none.frag.qsb
    DEFINES
        QSSG_ENABLE_RGBE_LIGHT_PROBE=1
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_skybox_hdr_linear"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/skybox.vert
        res/rhishaders/skybox.frag
    OUTPUTS
        res/rhishaders/skybox_hdr_linear.vert.qsb
        res/rhishaders/skybox_hdr_linear.frag.qsb
    DEFINES
        QSSG_ENABLE_LINEAR_TONEMAPPING=1
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_skybox_rgbe_linear"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/skybox.vert
        res/rhishaders/skybox.frag
    OUTPUTS
        res/rhishaders/skybox_rgbe_linear.vert.qsb
        res/rhishaders/skybox_rgbe_linear.frag.qsb
    DEFINES
        QSSG_ENABLE_LINEAR_TONEMAPPING=1
        QSSG_ENABLE_RGBE_LIGHT_PROBE=1
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_skybox_hdr_aces"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/skybox.vert
        res/rhishaders/skybox.frag
    OUTPUTS
        res/rhishaders/skybox_hdr_aces.vert.qsb
        res/rhishaders/skybox_hdr_aces.frag.qsb
    DEFINES
        QSSG_ENABLE_ACES_TONEMAPPING=1
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_skybox_rgbe_aces"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/skybox.vert
        res/rhishaders/skybox.frag
    OUTPUTS
        res/rhishaders/skybox_rgbe_aces.vert.qsb
        res/rhishaders/skybox_rgbe_aces.frag.qsb
    DEFINES
        QSSG_ENABLE_ACES_TONEMAPPING=1
        QSSG_ENABLE_RGBE_LIGHT_PROBE=1
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_skybox_hdr_hejldawson"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/skybox.vert
        res/rhishaders/skybox.frag
    OUTPUTS
        res/rhishaders/skybox_hdr_hejldawson.vert.qsb
        res/rhishaders/skybox_hdr_hejldawson.frag.qsb
    DEFINES
        QSSG_ENABLE_HEJLDAWSON_TONEMAPPING=1
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_skybox_rgbe_hejldawson"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/skybox.vert
        res/rhishaders/skybox.frag
    OUTPUTS
        res/rhishaders/skybox_rgbe_hejldawson.vert.qsb
        res/rhishaders/skybox_rgbe_hejldawson.frag.qsb
    DEFINES
        QSSG_ENABLE_HEJLDAWSON_TONEMAPPING=1
        QSSG_ENABLE_RGBE_LIGHT_PROBE=1
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_skybox_hdr_filmic"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/skybox.vert
        res/rhishaders/skybox.frag
    OUTPUTS
        res/rhishaders/skybox_hdr_filmic.vert.qsb
        res/rhishaders/skybox_hdr_filmic.frag.qsb
    DEFINES
        QSSG_ENABLE_FILMIC_TONEMAPPING=1
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_skybox_rgbe_filmic"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/skybox.vert
        res/rhishaders/skybox.frag
    OUTPUTS
        res/rhishaders/skybox_rgbe_filmic.vert.qsb
        res/rhishaders/skybox_rgbe_filmic.frag.qsb
    DEFINES
        QSSG_ENABLE_FILMIC_TONEMAPPING=1
        QSSG_ENABLE_RGBE_LIGHT_PROBE=1
)
# Shaders without lights
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_particles_nolight"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/particles.vert
        res/rhishaders/particles.frag
    OUTPUTS
        res/rhishaders/particlesnolightsimple.vert.qsb
        res/rhishaders/particlesnolightsimple.frag.qsb
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_particles_nolight_mapped"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/particles.vert
        res/rhishaders/particles.frag
    OUTPUTS
        res/rhishaders/particlesnolightmapped.vert.qsb
        res/rhishaders/particlesnolightmapped.frag.qsb
    DEFINES
        QSSG_PARTICLES_ENABLE_MAPPED
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_particles_nolight_animated"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/particles.vert
        res/rhishaders/particles.frag
    OUTPUTS
        res/rhishaders/particlesnolightanimated.vert.qsb
        res/rhishaders/particlesnolightanimated.frag.qsb
    DEFINES
        QSSG_PARTICLES_ENABLE_MAPPED
        QSSG_PARTICLES_ENABLE_ANIMATED
)
# Shaders with lights
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_particles_vlight"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/particles.vert
        res/rhishaders/particles.frag
    OUTPUTS
        res/rhishaders/particlesvlightsimple.vert.qsb
        res/rhishaders/particlesvlightsimple.frag.qsb
    DEFINES
        QSSG_PARTICLES_ENABLE_VERTEX_LIGHTING
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_particles_vlight_mapped"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/particles.vert
        res/rhishaders/particles.frag
    OUTPUTS
        res/rhishaders/particlesvlightmapped.vert.qsb
        res/rhishaders/particlesvlightmapped.frag.qsb
    DEFINES
        QSSG_PARTICLES_ENABLE_MAPPED
        QSSG_PARTICLES_ENABLE_VERTEX_LIGHTING
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_particles_vlight_animated"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/particles.vert
        res/rhishaders/particles.frag
    OUTPUTS
        res/rhishaders/particlesvlightanimated.vert.qsb
        res/rhishaders/particlesvlightanimated.frag.qsb
    DEFINES
        QSSG_PARTICLES_ENABLE_MAPPED
        QSSG_PARTICLES_ENABLE_ANIMATED
        QSSG_PARTICLES_ENABLE_VERTEX_LIGHTING
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_lightmapuvraster_default"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,120,330"
    PREFIX
        "/"
    FILES
        res/rhishaders/lightmapuvraster.vert
        res/rhishaders/lightmapuvraster.frag
    OUTPUTS
        res/rhishaders/lightmapuvraster.vert.qsb
        res/rhishaders/lightmapuvraster.frag.qsb
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_lightmapuvraster_uv"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,120,330"
    PREFIX
        "/"
    FILES
        res/rhishaders/lightmapuvraster.vert
        res/rhishaders/lightmapuvraster.frag
    OUTPUTS
        res/rhishaders/lightmapuvraster_uv.vert.qsb
        res/rhishaders/lightmapuvraster_uv.frag.qsb
    DEFINES
        QSSG_LIGHTMAPUVRASTER_UV
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_lightmapuvraster_uv_tangent"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,120,330"
    PREFIX
        "/"
    FILES
        res/rhishaders/lightmapuvraster.vert
        res/rhishaders/lightmapuvraster.frag
    OUTPUTS
        res/rhishaders/lightmapuvraster_uv_tangent.vert.qsb
        res/rhishaders/lightmapuvraster_uv_tangent.frag.qsb
    DEFINES
        QSSG_LIGHTMAPUVRASTER_UV_TANGENT
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_line_particles"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/lineparticles.vert
        res/rhishaders/lineparticles.frag
    OUTPUTS
        res/rhishaders/lineparticles.vert.qsb
        res/rhishaders/lineparticles.frag.qsb
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_line_particles_mapped"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/lineparticles.vert
        res/rhishaders/lineparticles.frag
    OUTPUTS
        res/rhishaders/lineparticlesmapped.vert.qsb
        res/rhishaders/lineparticlesmapped.frag.qsb
    DEFINES
        QSSG_PARTICLES_ENABLE_MAPPED
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_line_particles_animated"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/lineparticles.vert
        res/rhishaders/lineparticles.frag
    OUTPUTS
        res/rhishaders/lineparticlesanimated.vert.qsb
        res/rhishaders/lineparticlesanimated.frag.qsb
    DEFINES
        QSSG_PARTICLES_ENABLE_MAPPED
        QSSG_PARTICLES_ENABLE_ANIMATED
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_line_particles_vlight"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/lineparticles.vert
        res/rhishaders/lineparticles.frag
    OUTPUTS
        res/rhishaders/lineparticlesvlightsimple.vert.qsb
        res/rhishaders/lineparticlesvlightsimple.frag.qsb
    DEFINES
        QSSG_PARTICLES_ENABLE_VERTEX_LIGHTING
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_line_particles_vlight_mapped"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/lineparticles.vert
        res/rhishaders/lineparticles.frag
    OUTPUTS
        res/rhishaders/lineparticlesvlightmapped.vert.qsb
        res/rhishaders/lineparticlesvlightmapped.frag.qsb
    DEFINES
        QSSG_PARTICLES_ENABLE_MAPPED
        QSSG_PARTICLES_ENABLE_VERTEX_LIGHTING
)
qt_internal_add_shaders(Quick3DRuntimeRender "res_shaders_line_particles_vlight_animated"
    SILENT
    PRECOMPILE
    OPTIMIZED
    GLSL "300es,150"
    PREFIX
        "/"
    FILES
        res/rhishaders/lineparticles.vert
        res/rhishaders/lineparticles.frag
    OUTPUTS
        res/rhishaders/lineparticlesvlightanimated.vert.qsb
        res/rhishaders/lineparticlesvlightanimated.frag.qsb
    DEFINES
        QSSG_PARTICLES_ENABLE_MAPPED
        QSSG_PARTICLES_ENABLE_ANIMATED
        QSSG_PARTICLES_ENABLE_VERTEX_LIGHTING
)
# special case end

#### Keys ignored in scope 1:.:.:runtimerender.pro:<TRUE>:
# MODULE = "quick3druntimerender"
