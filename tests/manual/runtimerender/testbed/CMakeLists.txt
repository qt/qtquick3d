# Generated from testbed.pro.

#####################################################################
## testbed Binary:
#####################################################################

qt_add_manual_test(testbed
    GUI
    SOURCES
        main.cpp
        renderwindow.cpp renderwindow.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick3D
        Qt::Quick3DRuntimeRenderPrivate
)
