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
    SKIP_TYPE_REGISTRATION
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::Quick3DPrivate
        Qt::Quick3DParticlesPrivate
)

