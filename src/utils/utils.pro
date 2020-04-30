TARGET = QtQuick3DUtils
MODULE = quick3dutils

QT = core-private gui-private

DEFINES += QT_BUILD_QUICK3DUTILS_LIB

HEADERS += \
    qssgbounds3_p.h \
    qssgmeshbvh_p.h \
    qssgutils_p.h \
    qssgdataref_p.h \
    qssgoption_p.h \
    qssginvasivelinkedlist_p.h \
    qssgplane_p.h \
    qssgperftimer_p.h \
    qtquick3dutilsglobal_p.h

SOURCES += \
    qssgbounds3.cpp \
    qssgdataref.cpp \
    qssgmeshbvh.cpp \
    qssgperftimer.cpp \
    qssgplane.cpp \
    qssgutils.cpp

load(qt_module)
