TARGET = QtQuick3DRuntimeRender
MODULE = quick3druntimerender

QT += quick3drender-private quick3dassetimport-private

include(graphobjects/graphobjects.pri)
include(rendererimpl/rendererimpl.pri)
include(resourcemanager/resourcemanager.pri)

DEFINES += QT_BUILD_QUICK3DRUNTIMERENDER_LIB

HEADERS += \
    qtquick3druntimerenderglobal_p.h \
    qssgrendergpuprofiler_p.h \
    qssgoffscreenrenderkey_p.h \
    qssgoffscreenrendermanager_p.h \
    qssgrenderableimage_p.h \
    qssgrenderclippingfrustum_p.h \
    qssgrendercontextcore_p.h \
    qssgrendercustommaterialrendercontext_p.h \
    qssgrendercustommaterialsystem_p.h \
    qssgrenderdefaultmaterialshadergenerator_p.h \
    qssgrenderdynamicobjectsystem_p.h \
    qssgrenderdynamicobjectsystemcommands_p.h \
    qssgrenderdynamicobjectsystemutil_p.h \
    qssgrendereffectsystem_p.h \
    qssgrenderer_p.h \
    qssgrendererutil_p.h \
    qssgrendereulerangles_p.h \
    qssgrendergraphobjectpickquery_p.h \
    qssgrenderimagetexturedata_p.h \
    qssgrenderinputstreamfactory_p.h \
    qssgrenderlightconstantproperties_p.h \
    qssgrendermaterialshadergenerator_p.h \
    qssgrendermesh_p.h \
    qssgrenderpathmanager_p.h \
    qssgrenderpathmath_p.h \
    qssgrenderpathrendercontext_p.h \
    qssgrenderpixelgraphicsrenderer_p.h \
    qssgrenderpixelgraphicstypes_p.h \
    qssgrenderray_p.h \
    qssgrenderrenderlist_p.h \
    qssgrendershadercache_p.h \
    qssgrendershadercodegenerator_p.h \
    qssgrendershadercodegeneratorv2_p.h \
    qssgrendershaderkeys_p.h \
    qssgrendershadowmap_p.h \
    qssgrendertessmodevalues_p.h \
    qssgrenderthreadpool_p.h \
    qssgrenderwidgets_p.h \
    qssgruntimerenderlogging_p.h \
    qssgperframeallocator_p.h

SOURCES += \
    qssgoffscreenrendermanager.cpp \
    qssgrenderclippingfrustum.cpp \
    qssgrendercontextcore.cpp \
    qssgrendercustommaterialshadergenerator.cpp \
    qssgrendercustommaterialsystem.cpp \
    qssgrenderdefaultmaterialshadergenerator.cpp \
    qssgrenderdynamicobjectsystem.cpp \
    qssgrendereffectsystem.cpp \
    qssgrendererutil.cpp \
    qssgrendereulerangles.cpp \
    qssgrendergpuprofiler.cpp \
    qssgrenderinputstreamfactory.cpp \
    qssgrendermaterialshadergenerator.cpp \
    qssgrenderpathmanager.cpp \
    qssgrenderpixelgraphicsrenderer.cpp \
    qssgrenderray.cpp \
    qssgrenderrenderlist.cpp \
    qssgrendershadercache.cpp \
    qssgrendershadercodegenerator.cpp \
    qssgrendershadercodegeneratorv2.cpp \
    qssgrendershadowmap.cpp \
    qssgrenderthreadpool.cpp \
    qssgrenderwidgets.cpp \
    qssgruntimerenderlogging.cpp \
    qssgrenderer.cpp \
    qssgrendercustommaterialrendercontext.cpp

RESOURCES += res.qrc

load(qt_module)
