# Generated from plugin.pro.

#####################################################################
## qtquick3dparticles3dplugin Plugin:
#####################################################################

qt_internal_add_qml_module(qtquick3dparticles3dplugin
    URI "QtQuick3D.Particles3D"
    VERSION "${CMAKE_PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    CLASSNAME QtQuick3DParticles3DPlugin
    DEPENDENCIES
        QtQuick3D/6.1
    GENERATE_QMLTYPES
    INSTALL_QMLTYPES
    SOURCES
        plugin.cpp
        qquick3dparticle.cpp qquick3dparticle_p.h
        qquick3dparticleaffector.cpp qquick3dparticleaffector_p.h
        qquick3dparticleattractor.cpp qquick3dparticleattractor_p.h
        qquick3dparticledata_p.h
        qquick3dparticledirection.cpp qquick3dparticledirection_p.h
        qquick3dparticleemitburst.cpp qquick3dparticleemitburst_p.h
        qquick3dparticleemitter.cpp qquick3dparticleemitter_p.h
        qquick3dparticlegravity.cpp qquick3dparticlegravity_p.h
        qquick3dparticlemodelparticle.cpp qquick3dparticlemodelparticle_p.h
        qquick3dparticlepointrotator.cpp qquick3dparticlepointrotator_p.h
        qquick3dparticlerandomizer_p.h
        qquick3dparticleshape.cpp qquick3dparticleshape_p.h
        qquick3dparticleshapecube.cpp qquick3dparticleshapecube_p.h
        qquick3dparticleshapecylinder.cpp qquick3dparticleshapecylinder_p.h
        qquick3dparticleshapenode.cpp qquick3dparticleshapenode_p.h
        qquick3dparticleshapesphere.cpp qquick3dparticleshapesphere_p.h
        qquick3dparticlespriteparticle.cpp qquick3dparticlespriteparticle_p.h
        qquick3dparticlesystem.cpp qquick3dparticlesystem_p.h
        qquick3dparticlesystemlogging.cpp qquick3dparticlesystemlogging_p.h
        qquick3dparticletargetdirection.cpp qquick3dparticletargetdirection_p.h
        qquick3dparticletrailemitter.cpp qquick3dparticletrailemitter_p.h
        qquick3dparticlevectordirection.cpp qquick3dparticlevectordirection_p.h
        qquick3dparticlewander.cpp qquick3dparticlewander_p.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::Quick3DPrivate
)

#### Keys ignored in scope 1:.:.:plugin.pro:<TRUE>:
# CXX_MODULE = "qml"
# DISTFILES = "qmldir"
# QMLTYPES_FILENAME = "plugins.qmltypes"
# QMLTYPES_INSTALL_DIR = "$$[QT_INSTALL_QML]/QtQuick3D/Particles3D"
# QML_IMPORT_MAJOR_VERSION = "1"
# QML_IMPORT_NAME = "QtQuick3D.Particles3D"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "QtQuick3D/Particles3D"
