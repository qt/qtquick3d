# Generated from balsam.pro.

#####################################################################
## balsam Tool:
#####################################################################

qt_get_tool_target_name(target_name balsam)
qt_internal_add_tool(${target_name}
    TOOLS_TARGET Quick3D # special case
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Qml # special case
        Qt::Gui
        Qt::Quick3DAssetImportPrivate
        Qt::Quick3DIblBakerPrivate
)
qt_internal_return_unless_building_tools()
