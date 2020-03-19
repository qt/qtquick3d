# Generated from rendertotexture.pro.

#####################################################################
## rendertotexture Binary:
#####################################################################

qt_add_manual_test(rendertotexture
    GUI
    SOURCES
        ../shared/renderexample.cpp ../shared/renderexample.h
        ../shared/renderexampletools.cpp ../shared/renderexampletools.h
        rendertotextureexample.cpp
    INCLUDE_DIRECTORIES
        ../shared/ # special case
    LIBRARIES
        Qt::Quick3DRenderPrivate
        Qt::Quick3DUtilsPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick3D
        Qt::Quick3DRenderPrivate
        Qt::Quick3DUtilsPrivate
)
