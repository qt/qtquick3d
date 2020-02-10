TARGET = QtQuick3D
MODULE = quick3d

QT += core-private gui-private quick3druntimerender-private quick-private qml-private

DEFINES += QT_BUILD_QUICK3D_LIB

SOURCES += \
    qquick3d.cpp \
    qquick3dabstractlight.cpp \
    qquick3dcamera.cpp \
    qquick3deffect.cpp \
    qquick3dgeometry.cpp \
    qquick3dorthographiccamera.cpp \
    qquick3dperspectivecamera.cpp \
    qquick3dfrustumcamera.cpp \
    qquick3dcustomcamera.cpp \
    qquick3dcustommaterial.cpp \
    qquick3ddefaultmaterial.cpp \
    qquick3ddirectionallight.cpp \
    qquick3dpointlight.cpp \
    qquick3darealight.cpp \
    qquick3dquaternionanimation.cpp \
    qquick3dspotlight.cpp \
    qquick3dloader.cpp \
    qquick3dmaterial.cpp \
    qquick3dmodel.cpp \
    qquick3dnode.cpp \
    qquick3dobject.cpp \
    qquick3drenderstats.cpp \
    qquick3drepeater.cpp \
    qquick3dsceneenvironment.cpp \
    qquick3dscenemanager.cpp \
    qquick3dscenerenderer.cpp \
    qquick3dscenerootnode.cpp \
    qquick3dshaderutils.cpp \
    qquick3dtexture.cpp \
    qquick3dviewport.cpp \
    qquick3dpickresult.cpp \
    qquick3dprincipledmaterial.cpp \
    qquick3dquaternionutils.cpp \
    qquick3ditem2d.cpp

HEADERS += \
    qquick3d.h \
    qquick3dabstractlight_p.h \
    qquick3deffect_p.h \
    qquick3dgeometry.h \
    qquick3dgeometry_p.h \
    qquick3dloader_p.h \
    qquick3dnode_p_p.h \
    qquick3dobject.h \
    qquick3dobject_p.h \
    qquick3dobjectchangelistener_p.h \
    qquick3dquaternionanimation_p.h \
    qquick3drenderstats_p.h \
    qquick3drepeater_p.h \
    qquick3dscenemanager_p.h \
    qquick3dshaderutils_p.h \
    qquick3dutils_p.h \
    qtquick3dglobal.h \
    qtquick3dglobal_p.h \
    qquick3dcamera_p.h \
    qquick3dorthographiccamera_p.h \
    qquick3dperspectivecamera_p.h \
    qquick3dfrustumcamera_p.h \
    qquick3dcustomcamera_p.h \
    qquick3dcustommaterial_p.h \
    qquick3ddefaultmaterial_p.h \
    qquick3ddirectionallight_p.h \
    qquick3dpointlight_p.h \
    qquick3darealight_p.h \
    qquick3dspotlight_p.h \
    qquick3dmaterial_p.h \
    qquick3dmodel_p.h \
    qquick3dnode_p.h \
    qquick3dsceneenvironment_p.h \
    qquick3dscenerenderer_p.h \
    qquick3dscenerootnode_p.h \
    qquick3dtexture_p.h \
    qquick3dviewport_p.h \
    qquick3dpickresult_p.h \
    qquick3dprincipledmaterial_p.h \
    qquick3dquaternionutils_p.h \
    qquick3ditem2d_p.h

load(qt_module)
