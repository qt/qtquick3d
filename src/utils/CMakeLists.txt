# Generated from utils.pro.

#####################################################################
## Quick3DUtils Module:
#####################################################################

qt_internal_add_module(Quick3DUtils
    SOURCES
        qqsbcollection.cpp qqsbcollection_p.h
        qssgbounds3.cpp qssgbounds3_p.h
        qssgdataref.cpp qssgdataref_p.h
        qssginvasivelinkedlist_p.h
        qssgmeshbvh.cpp qssgmeshbvh_p.h
        qssgoption_p.h
        qssgplane.cpp qssgplane_p.h
        qssgrenderbasetypes_p.h
        qssgutils.cpp qssgutils_p.h
        qssgmeshbvhbuilder.cpp qssgmeshbvhbuilder_p.h
        qssgmesh.cpp qssgmesh_p.h
        qtquick3dutilsglobal_p.h
    DEFINES
        QT_BUILD_QUICK3DUTILS_LIB
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:utils.pro:<TRUE>:
# MODULE = "quick3dutils"
