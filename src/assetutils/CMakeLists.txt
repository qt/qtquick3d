qt_internal_add_module(Quick3DAssetUtils
    SOURCES
        qtquick3dassetutilsglobal_p.h
        qssgscenedesc.cpp qssgscenedesc_p.h
        qssgqmlutilities.cpp qssgqmlutilities_p.h
    DEFINES
        QT_BUILD_QUICK3DASSETUTILS_LIB
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::Quick3DRuntimeRenderPrivate
        Qt::Quick3DPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::Quick3DRuntimeRenderPrivate
        Qt::Quick3DPrivate
)
