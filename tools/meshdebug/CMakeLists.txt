# Generated from meshdebug.pro.

#####################################################################
## meshdebug Tool:
#####################################################################

qt_add_tool(meshdebug
    TOOLS_TARGET Quick3D # special case
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick3DAssetImportPrivate
)
