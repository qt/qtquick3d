TARGET = QtQuick3DRuntimeRender
MODULE = quick3druntimerender

QT += quick3dassetimport-private quick3dutils-private shadertools-private quick-private opengl

include(graphobjects/graphobjects.pri)
include(rendererimpl/rendererimpl.pri)
include(resourcemanager/resourcemanager.pri)

DEFINES += QT_BUILD_QUICK3DRUNTIMERENDER_LIB

HEADERS += \
    qssgrendercommands_p.h \
    qssgrendercustommaterialshadergenerator_p.h \
    qtquick3druntimerenderglobal_p.h \
    qssgrenderableimage_p.h \
    qssgrenderclippingfrustum_p.h \
    qssgrendercontextcore_p.h \
    qssgrendercustommaterialrendercontext_p.h \
    qssgrendercustommaterialsystem_p.h \
    qssgrenderdefaultmaterialshadergenerator_p.h \
    qssgrendererutil_p.h \
    qssgrenderimagetexturedata_p.h \
    qssgrenderinputstreamfactory_p.h \
    qssgrendermaterialshadergenerator_p.h \
    qssgrendermesh_p.h \
    qssgrenderray_p.h \
    qssgrendershadercache_p.h \
    qssgrendershadercodegenerator_p.h \
    qssgrendershaderkeys_p.h \
    qssgrendershadowmap_p.h \
    qssgrenderthreadpool_p.h \
    qssgruntimerenderlogging_p.h \
    qssgperframeallocator_p.h \
    qssgshaderresourcemergecontext_p.h \
    qssgrendershadermetadata_p.h \
    qssgrhiquadrenderer_p.h \
    qssgrhieffectsystem_p.h \
    qssgrhicontext_p.h

SOURCES += \
    qssgrenderclippingfrustum.cpp \
    qssgrendercommands.cpp \
    qssgrendercontextcore.cpp \
    qssgrendercustommaterialshadergenerator.cpp \
    qssgrendercustommaterialsystem.cpp \
    qssgrenderdefaultmaterialshadergenerator.cpp \
    qssgrenderinputstreamfactory.cpp \
    qssgrenderray.cpp \
    qssgrendershadercache.cpp \
    qssgrendershadercodegenerator.cpp \
    qssgrendershadermetadata.cpp \
    qssgrendershadowmap.cpp \
    qssgrenderthreadpool.cpp \
    qssgruntimerenderlogging.cpp \
    qssgrendercustommaterialrendercontext.cpp \
    qssgrhiquadrenderer.cpp \
    qssgrhieffectsystem.cpp \
    qssgrhicontext.cpp

RESOURCES += res.qrc

load(qt_module)
