CXX_MODULE = qml
TARGET = qtquick3dparticles3dplugin
TARGETPATH = QtQuick3D/Particles3D
QT += quick qml quick3d-private
QML_IMPORT_VERSION = $$QT_VERSION

SOURCES += \
    plugin.cpp \
    qquick3dparticle.cpp \
    qquick3dparticleaffector.cpp \
    qquick3dparticledirection.cpp \
    qquick3dparticleemitburst.cpp \
    qquick3dparticleemitter.cpp \
    qquick3dparticlemodelparticle.cpp \
    qquick3dparticleshape.cpp \
    qquick3dparticleshapenode.cpp \
    qquick3dparticleattractor.cpp \
    qquick3dparticlegravity.cpp \
    qquick3dparticlepointrotator.cpp \
    qquick3dparticlewander.cpp \
    qquick3dparticlespriteparticle.cpp \
    qquick3dparticlesystem.cpp \
    qquick3dparticlesystemlogging.cpp \
    qquick3dparticletargetdirection.cpp \
    qquick3dparticletrailemitter.cpp \
    qquick3dparticlevectordirection.cpp

HEADERS += \
    qquick3dparticleemitburst_p.h \
    qquick3dparticlerandomizer_p.h \
    qquick3dparticledata_p.h \
    qquick3dparticledirection_p.h \
    qquick3dparticleemitter_p.h \
    qquick3dparticlemodelparticle_p.h \
    qquick3dparticleshape_p.h \
    qquick3dparticleshapenode_p.h \
    qquick3dparticleaffector_p.h \
    qquick3dparticleattractor_p.h \
    qquick3dparticlegravity_p.h \
    qquick3dparticlepointrotator_p.h \
    qquick3dparticlewander_p.h \
    qquick3dparticle_p.h \
    qquick3dparticlespriteparticle_p.h \
    qquick3dparticlesystem_p.h \
    qquick3dparticlesystemlogging_p.h \
    qquick3dparticletargetdirection_p.h \
    qquick3dparticletrailemitter_p.h \
    qquick3dparticlevectordirection_p.h

DISTFILES += \
    qmldir

#!static: qtConfig(quick-designer): include(designer/designer.pri)

load(qml_plugin)

QMLTYPES_FILENAME = plugins.qmltypes
QMLTYPES_INSTALL_DIR = $$[QT_INSTALL_QML]/QtQuick3D/Particles3D
QML_IMPORT_NAME = QtQuick3D.Particles3D
QML_IMPORT_VERSION = $$QT_VERSION
CONFIG += qmltypes install_qmltypes install_metatypes
