# Generated from meshdebug.pro.

#####################################################################
## meshdebug Tool:
#####################################################################

qt_add_tool(meshdebug
    TOOLS_TARGET Quick3D # special case
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Qml # special case
        Qt::OpenGL # special case
        Qt::Gui
        Qt::Quick3DAssetImportPrivate
)
