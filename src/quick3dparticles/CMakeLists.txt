
#####################################################################
## Quick3DParticles Module:
#####################################################################

qt_internal_add_module(Quick3DParticles
    GENERATE_METATYPES
    SOURCES
        qtquick3dparticlesglobal.h qtquick3dparticlesglobal_p.h
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
        qquick3dparticlespriteparticle.cpp qquick3dparticlespriteparticle_p.h
        qquick3dparticlesystem.cpp qquick3dparticlesystem_p.h
        qquick3dparticlesystemlogging.cpp qquick3dparticlesystemlogging_p.h
        qquick3dparticletargetdirection.cpp qquick3dparticletargetdirection_p.h
        qquick3dparticletrailemitter.cpp qquick3dparticletrailemitter_p.h
        qquick3dparticlevectordirection.cpp qquick3dparticlevectordirection_p.h
        qquick3dparticlewander.cpp qquick3dparticlewander_p.h
        qquick3dparticleutils.cpp qquick3dparticleutils_p.h
        qquick3dparticleforeign_p.h
    DEFINES
        QT_BUILD_QUICK3DPARTICLES_LIB
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::Quick3DRuntimeRenderPrivate
        Qt::QuickPrivate
        Qt::Quick3DPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::Quick3DRuntimeRender
        Qt::Quick3D
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::Quick3DRuntimeRenderPrivate
        Qt::QuickPrivate
        Qt::Quick3DPrivate
)

#### Keys ignored in scope 1:.:.:quick3d.pro:<TRUE>:
# MODULE = "quick3dparticles"
# QMLTYPES_FILENAME = "plugins.qmltypes"
# QMLTYPES_INSTALL_DIR = "$$[QT_INSTALL_QML]/QtQuick3D/Particles3D"
# QML_IMPORT_NAME = "QtQuick3D.Particles3D"
# QML_IMPORT_VERSION = "$$QT_VERSION"

set_target_properties(Quick3DParticles PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${PROJECT_VERSION}
    QT_QML_MODULE_URI QtQuick3D.Particles3D
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/QtQuick3D/Particles3D"
)

qt6_qml_type_registration(Quick3DParticles)
