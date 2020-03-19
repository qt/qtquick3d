# Generated from render.pro.

#####################################################################
## Quick3DRender Module:
#####################################################################

qt_add_module(Quick3DRender
    SOURCES
        backends/gl/qssgopenglextensions.cpp backends/gl/qssgopenglextensions_p.h
        backends/gl/qssgopengltokens_p.h
        backends/gl/qssgopenglutil_p.h
        backends/gl/qssgrenderbackendgl3.cpp backends/gl/qssgrenderbackendgl3_p.h
        backends/gl/qssgrenderbackendgl4.cpp backends/gl/qssgrenderbackendgl4_p.h
        backends/gl/qssgrenderbackendglbase.cpp backends/gl/qssgrenderbackendglbase_p.h
        backends/gl/qssgrenderbackendgles2.cpp backends/gl/qssgrenderbackendgles2_p.h
        backends/gl/qssgrenderbackendinputassemblergl_p.h
        backends/gl/qssgrenderbackendrenderstatesgl_p.h
        backends/gl/qssgrenderbackendshaderprogramgl_p.h
        backends/gl/qssgrendercontextgl.cpp
        backends/qssgrenderbackend_p.h
        backends/software/qssgrenderbackendnull.cpp backends/software/qssgrenderbackendnull_p.h
        glg/qssgglimplobjects_p.h
        qssgrenderattriblayout.cpp qssgrenderattriblayout_p.h
        qssgrenderbasetypes_p.h
        qssgrenderconstantbuffer.cpp qssgrenderconstantbuffer_p.h
        qssgrendercontext.cpp qssgrendercontext_p.h
        qssgrenderdatabuffer.cpp qssgrenderdatabuffer_p.h
        qssgrenderdepthstencilstate.cpp qssgrenderdepthstencilstate_p.h
        qssgrenderframebuffer.cpp qssgrenderframebuffer_p.h
        qssgrenderimagetexture.cpp qssgrenderimagetexture_p.h
        qssgrenderindexbuffer.cpp qssgrenderindexbuffer_p.h
        qssgrenderinputassembler.cpp qssgrenderinputassembler_p.h
        qssgrenderlogging.cpp qssgrenderlogging_p.h
        qssgrenderprogrampipeline.cpp qssgrenderprogrampipeline_p.h
        qssgrenderquerybase.cpp qssgrenderquerybase_p.h
        qssgrenderrasterizerstate.cpp qssgrenderrasterizerstate_p.h
        qssgrenderrenderbuffer.cpp qssgrenderrenderbuffer_p.h
        qssgrendershaderconstant.cpp qssgrendershaderconstant_p.h
        qssgrendershaderprogram.cpp qssgrendershaderprogram_p.h
        qssgrenderstoragebuffer.cpp qssgrenderstoragebuffer_p.h
        qssgrendersync.cpp qssgrendersync_p.h
        qssgrendertexture2d.cpp qssgrendertexture2d_p.h
        qssgrendertexturebase.cpp qssgrendertexturebase_p.h
        qssgrendertexturecube.cpp qssgrendertexturecube_p.h
        qssgrendertimerquery.cpp qssgrendertimerquery_p.h
        qssgrendervertexbuffer.cpp qssgrendervertexbuffer_p.h
        qssgrhicontext.cpp qssgrhicontext_p.h
        qtquick3drenderglobal_p.h
    DEFINES
        QT_BUILD_QUICK3DRENDER_LIB
    LIBRARIES
        Qt::GuiPrivate
        Qt::Quick3DUtilsPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::OpenGLExtensions
        Qt::Quick
        Qt::Quick3DUtils
    PRIVATE_MODULE_INTERFACE
        Qt::GuiPrivate
        Qt::Quick3DUtilsPrivate
)

#### Keys ignored in scope 1:.:.:render.pro:<TRUE>:
# MODULE = "quick3drender"
