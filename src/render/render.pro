TARGET = QtQuick3DRender
MODULE = quick3drender

QT += quick3dutils-private quick

QT_PRIVATE += openglextensions

DEFINES += QT_BUILD_QUICK3DRENDER_LIB

HEADERS += \
    qssgrendershaderprogram_p.h \
    qssgrenderbasetypes_p.h \
    qssgrenderattriblayout_p.h \
    qssgrenderconstantbuffer_p.h \
    qssgrendercontext_p.h \
    qssgrenderdatabuffer_p.h \
    qssgrenderdepthstencilstate_p.h \
    qssgrenderframebuffer_p.h \
    backends/gl/qssgrenderbackendgles2_p.h \
    backends/gl/qssgopenglextensions_p.h \
    backends/gl/qssgopengltokens_p.h \
    backends/gl/qssgopenglutil_p.h \
    backends/gl/qssgrenderbackendgl3_p.h \
    backends/gl/qssgrenderbackendgl4_p.h \
    backends/gl/qssgrenderbackendglbase_p.h \
    backends/gl/qssgrenderbackendinputassemblergl_p.h \
    backends/gl/qssgrenderbackendrenderstatesgl_p.h \
    backends/gl/qssgrenderbackendshaderprogramgl_p.h \
    backends/software/qssgrenderbackendnull_p.h \
    backends/qssgrenderbackend_p.h \
    glg/qssgglimplobjects_p.h \
    qssgrenderimagetexture_p.h \
    qssgrenderindexbuffer_p.h \
    qssgrenderinputassembler_p.h \
    qssgrenderprogrampipeline_p.h \
    qssgrenderquerybase_p.h \
    qssgrenderrasterizerstate_p.h \
    qssgrenderrenderbuffer_p.h \
    qssgrendershaderconstant_p.h \
    qssgrenderstoragebuffer_p.h \
    qssgrendersync_p.h \
    qssgrendertexture2d_p.h \
    qssgrendertexturebase_p.h \
    qssgrendertexturecube_p.h \
    qssgrendertimerquery_p.h \
    qssgrendervertexbuffer_p.h \
    qssgrenderlogging_p.h \
    qtquick3drenderglobal_p.h

SOURCES += \
    backends/gl/qssgopenglextensions.cpp \
    backends/gl/qssgrenderbackendgles2.cpp \
    backends/gl/qssgrenderbackendgl3.cpp \
    backends/gl/qssgrenderbackendgl4.cpp \
    backends/gl/qssgrenderbackendglbase.cpp \
    backends/gl/qssgrendercontextgl.cpp \
    backends/software/qssgrenderbackendnull.cpp \
    qssgrenderattriblayout.cpp \
    qssgrenderconstantbuffer.cpp \
    qssgrendercontext.cpp \
    qssgrenderdatabuffer.cpp \
    qssgrenderdepthstencilstate.cpp \
    qssgrenderframebuffer.cpp \
    qssgrenderimagetexture.cpp \
    qssgrenderindexbuffer.cpp \
    qssgrenderinputassembler.cpp \
    qssgrenderprogrampipeline.cpp \
    qssgrenderquerybase.cpp \
    qssgrenderrasterizerstate.cpp \
    qssgrenderrenderbuffer.cpp \
    qssgrendershaderprogram.cpp \
    qssgrenderstoragebuffer.cpp \
    qssgrendersync.cpp \
    qssgrendertexture2d.cpp \
    qssgrendertexturebase.cpp \
    qssgrendertexturecube.cpp \
    qssgrendertimerquery.cpp \
    qssgrendervertexbuffer.cpp \
    qssgrenderlogging.cpp \
    qssgrendershaderconstant.cpp

load(qt_module)
