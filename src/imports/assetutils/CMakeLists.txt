#####################################################################
## qtquick3d assetutils Plugin:
#####################################################################

qt_internal_add_qml_module(qtquick3dassetutilsplugin
    URI "QtQuick3D.AssetUtils"
    VERSION "${PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    CLASSNAME QtQuick3DAssetUtilsPlugin
    DEPENDENCIES
        QtQuick3D/auto
    GENERATE_QMLTYPES
    INSTALL_QMLTYPES
    SOURCES
        plugin.cpp
        qquick3druntimeloader.cpp qquick3druntimeloader_p.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::Quick3DPrivate
        Qt::Quick3DAssetUtilsPrivate
        Qt::Quick3DAssetImportPrivate
     PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::Quick3DRuntimeRenderPrivate
        Qt::Quick3DPrivate
        Qt::Quick3DAssetUtilsPrivate
        Qt::Quick3DAssetImportPrivate
)
