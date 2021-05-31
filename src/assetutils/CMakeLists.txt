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

qt_internal_extend_target(Quick3DAssetUtils CONDITION TARGET Qt::QuickTimeline
    LIBRARIES
        Qt::QuickTimelinePrivate
    PRIVATE_MODULE_INTERFACE
        Qt::QuickTimelinePrivate
    DEFINES
        QT_QUICK3D_ENABLE_RT_ANIMATIONS
)
