#####################################################################
## Quick3DParticleEffects Module:
#####################################################################

qt_internal_add_qml_module(Quick3DParticleEffects
    URI "QtQuick3D.ParticleEffects"
    VERSION "${PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    CLASSNAME QtQuick3DParticleEffectsPlugin
    PLUGIN_TARGET qtquick3dparticleeffectsplugin
    DEPENDENCIES
        QtQuick3D/auto
        QtQuick3DParticles3D/auto
    NO_SYNC_QT
    DEFINES
        QT_BUILD_QUICK3DPARTICLEEFFECTS_LIB
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::Quick3DRuntimeRender
        Qt::Quick3DAssetImport
        Qt::Quick3D
        Qt::Quick3DParticles
)

set_target_properties(Quick3DParticleEffects PROPERTIES DISABLE_PRECOMPILE_HEADERS ON)

if(QT_FEATURE_quick_designer AND QT_BUILD_SHARED_LIBS) # special case handle unconverted static
    add_subdirectory(designer)
endif()
