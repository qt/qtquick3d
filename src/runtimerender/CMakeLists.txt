# Generated from runtimerender.pro.

#####################################################################
## Quick3DRuntimeRender Module:
#####################################################################

qt_internal_add_module(Quick3DRuntimeRender
    SOURCES
        graphobjects/qssgrendercamera.cpp graphobjects/qssgrendercamera_p.h
        graphobjects/qssgrendercustommaterial_p.h
        graphobjects/qssgrenderdefaultmaterial.cpp graphobjects/qssgrenderdefaultmaterial_p.h
        graphobjects/qssgrendereffect.cpp graphobjects/qssgrendereffect_p.h
        graphobjects/qssgrendergeometry.cpp graphobjects/qssgrendergeometry_p.h
        graphobjects/qssgrendergraphobject.cpp graphobjects/qssgrendergraphobject_p.h
        graphobjects/qssgrenderimage.cpp graphobjects/qssgrenderimage_p.h
        graphobjects/qssgrenderitem2d.cpp graphobjects/qssgrenderitem2d_p.h
        graphobjects/qssgrenderjoint.cpp graphobjects/qssgrenderjoint_p.h
        graphobjects/qssgrenderlayer.cpp graphobjects/qssgrenderlayer_p.h
        graphobjects/qssgrenderlight.cpp graphobjects/qssgrenderlight_p.h
        graphobjects/qssgrenderlightmaps.cpp graphobjects/qssgrenderlightmaps_p.h
        graphobjects/qssgrendermaterialdirty_p.h
        graphobjects/qssgrendermodel.cpp graphobjects/qssgrendermodel_p.h
        graphobjects/qssgrendernode.cpp graphobjects/qssgrendernode_p.h
        graphobjects/qssgrenderskeleton.cpp graphobjects/qssgrenderskeleton_p.h
        graphobjects/qssgrendertexturedata.cpp graphobjects/qssgrendertexturedata_p.h
        qssgperframeallocator_p.h
        qssgrenderableimage_p.h
        qssgrenderclippingfrustum.cpp qssgrenderclippingfrustum_p.h
        qssgrendercommands.cpp qssgrendercommands_p.h
        qssgrendercontextcore.cpp qssgrendercontextcore_p.h
        qssgrenderdefaultmaterialshadergenerator.cpp qssgrenderdefaultmaterialshadergenerator_p.h
        qssgrendererutil_p.h
        qssgrenderimagetexturedata_p.h
        qssgrenderinputstreamfactory.cpp qssgrenderinputstreamfactory_p.h
        qssgrendermaterialshadergenerator_p.h
        qssgrendermesh_p.h
        qssgrenderray.cpp qssgrenderray_p.h
        qssgrendershadercache.cpp qssgrendershadercache_p.h
        qssgrendershadercodegenerator.cpp qssgrendershadercodegenerator_p.h
        qssgrendershaderkeys_p.h
        qssgrendershadermetadata.cpp qssgrendershadermetadata_p.h
        qssgrendershadowmap.cpp qssgrendershadowmap_p.h
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
        rendererimpl/qssgrendererimpllayerrenderdata_p.h
        rendererimpl/qssgrendererimpllayerrenderdata_rhi.cpp
        rendererimpl/qssgrendererimpllayerrenderhelper.cpp rendererimpl/qssgrendererimpllayerrenderhelper_p.h
        rendererimpl/qssgrendererimpllayerrenderpreparationdata.cpp rendererimpl/qssgrendererimpllayerrenderpreparationdata_p.h
        rendererimpl/qssgrendererimplshaders_rhi.cpp
        rendererimpl/qssgvertexpipelineimpl.cpp rendererimpl/qssgvertexpipelineimpl_p.h
        resourcemanager/qssgrenderbuffermanager.cpp resourcemanager/qssgrenderbuffermanager_p.h
        resourcemanager/qssgrenderloadedtexture.cpp resourcemanager/qssgrenderloadedtexture_p.h
        resourcemanager/qssgrenderresourcemanager.cpp resourcemanager/qssgrenderresourcemanager_p.h
        resourcemanager/qssgrendershaderlibrarymanager.cpp resourcemanager/qssgrendershaderlibrarymanager_p.h
    DEFINES
        QT_BUILD_QUICK3DRUNTIMERENDER_LIB
    LIBRARIES
        Qt::Quick3DAssetImportPrivate
        Qt::Quick3DUtilsPrivate
        Qt::QuickPrivate
        Qt::ShaderToolsPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Quick
        Qt::Quick3DAssetImport
        Qt::Quick3DUtils
        Qt::ShaderTools
    PRIVATE_MODULE_INTERFACE
        Qt::Quick3DAssetImportPrivate
        Qt::Quick3DUtilsPrivate
        Qt::QuickPrivate
        Qt::ShaderToolsPrivate
)

# Resources:
set(res_resource_files
    "res/effectlib/Fxaa3_11.glsllib"
    "res/effectlib/defaultMaterialBumpNoLod.glsllib"
    "res/effectlib/defaultMaterialFresnel.glsllib"
    "res/effectlib/depthpass.glsllib"
    "res/effectlib/doubleSided.glsllib"
    "res/effectlib/evalLightmaps.glsllib"
    "res/effectlib/funccalculatePointLightAttenuation.glsllib"
    "res/effectlib/funcdiffuseBurleyBSDF.glsllib"
    "res/effectlib/funcdiffuseReflectionBSDF.glsllib"
    "res/effectlib/funcdiffuseReflectionWrapBSDF.glsllib"
    "res/effectlib/funcgetTransformedUVCoords.glsllib"
    "res/effectlib/funcsampleLightVars.glsllib"
    "res/effectlib/funcsampleNormalTexture.glsllib"
    "res/effectlib/funcspecularBSDF.glsllib"
    "res/effectlib/funcspecularGGXBSDF.glsllib"
    "res/effectlib/physGlossyBSDF.glsllib"
    "res/effectlib/principledMaterialFresnel.glsllib"
    "res/effectlib/sampleProbe.glsllib"
    "res/effectlib/shadowMapping.glsllib"
    "res/effectlib/skinanim.glsllib"
    "res/effectlib/ssao.glsllib"
    "res/effectlib/tonemapping.glsllib"
    "res/effectlib/viewProperties.glsllib"
    "res/primitives/Cone.mesh"
    "res/primitives/Cube.mesh"
    "res/primitives/Cylinder.mesh"
    "res/primitives/Rectangle.mesh"
    "res/primitives/Sphere.mesh"
    "res/rhishaders/cubeshadowblurx.frag.qsb"
    "res/rhishaders/cubeshadowblurx.vert.qsb"
    "res/rhishaders/cubeshadowblury.frag.qsb"
    "res/rhishaders/cubeshadowblury.vert.qsb"
    "res/rhishaders/environmentmap.frag.qsb"
    "res/rhishaders/environmentmap.vert.qsb"
    "res/rhishaders/environmentmapirradiance.frag.qsb"
    "res/rhishaders/environmentmapirradiance.vert.qsb"
    "res/rhishaders/environmentmapirradiance_rgbe.frag.qsb"
    "res/rhishaders/environmentmapirradiance_rgbe.vert.qsb"
    "res/rhishaders/environmentmapprefilter.frag.qsb"
    "res/rhishaders/environmentmapprefilter.vert.qsb"
    "res/rhishaders/environmentmapprefilter_rgbe.frag.qsb"
    "res/rhishaders/environmentmapprefilter_rgbe.vert.qsb"
    "res/rhishaders/orthoshadowblurx.frag.qsb"
    "res/rhishaders/orthoshadowblurx.vert.qsb"
    "res/rhishaders/orthoshadowblury.frag.qsb"
    "res/rhishaders/orthoshadowblury.vert.qsb"
    "res/rhishaders/progressiveaa.frag.qsb"
    "res/rhishaders/progressiveaa.vert.qsb"
    "res/rhishaders/skybox_hdr_aces.frag.qsb"
    "res/rhishaders/skybox_hdr_aces.vert.qsb"
    "res/rhishaders/skybox_hdr_filmic.frag.qsb"
    "res/rhishaders/skybox_hdr_filmic.vert.qsb"
    "res/rhishaders/skybox_hdr_hejldawson.frag.qsb"
    "res/rhishaders/skybox_hdr_hejldawson.vert.qsb"
    "res/rhishaders/skybox_hdr_linear.frag.qsb"
    "res/rhishaders/skybox_hdr_linear.vert.qsb"
    "res/rhishaders/skybox_hdr_none.frag.qsb"
    "res/rhishaders/skybox_hdr_none.vert.qsb"
    "res/rhishaders/skybox_rgbe_aces.frag.qsb"
    "res/rhishaders/skybox_rgbe_aces.vert.qsb"
    "res/rhishaders/skybox_rgbe_filmic.frag.qsb"
    "res/rhishaders/skybox_rgbe_filmic.vert.qsb"
    "res/rhishaders/skybox_rgbe_hejldawson.frag.qsb"
    "res/rhishaders/skybox_rgbe_hejldawson.vert.qsb"
    "res/rhishaders/skybox_rgbe_linear.frag.qsb"
    "res/rhishaders/skybox_rgbe_linear.vert.qsb"
    "res/rhishaders/skybox_rgbe_none.frag.qsb"
    "res/rhishaders/skybox_rgbe_none.vert.qsb"
    "res/rhishaders/ssaaresolve.frag.qsb"
    "res/rhishaders/ssaaresolve.vert.qsb"
    "res/rhishaders/ssao.frag.qsb"
    "res/rhishaders/ssao.vert.qsb"
    "res/rhishaders/texturedquad.frag.qsb"
    "res/rhishaders/texturedquad.vert.qsb"
)

qt_internal_add_resource(Quick3DRuntimeRender "res"
    PREFIX
        "/"
    FILES
        ${res_resource_files}
)


#### Keys ignored in scope 1:.:.:runtimerender.pro:<TRUE>:
# MODULE = "quick3druntimerender"
