TARGET = qmlscenegrabber
DESTDIR=..
macos:CONFIG -= app_bundle
CONFIG += console

QML_IMPORT_NAME = QtQuick3D.Lancelot
QML_IMPORT_VERSION = $$QT_VERSION
CONFIG += qmltypes

QT += quick quick3d-private

SOURCES += main.cpp \
    dynamicgeometry.cpp \
    indextrianglestopology.cpp \
    nonindextrianglestopology.cpp \
    pointstopology.cpp

HEADERS += \
    dynamicgeometry.h \
    indextrianglestopology.h \
    nonindextrianglestopology.h \
    pointstopology.h
