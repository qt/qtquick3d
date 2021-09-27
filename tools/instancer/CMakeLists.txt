#####################################################################
## instancer Tool:
#####################################################################

qt_get_tool_target_name(target_name instancer)
qt_internal_add_tool(${target_name}
    TOOLS_TARGET Quick3D
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Qml # special case
        Qt::Gui
        Qt::Quick3DPrivate
)
qt_internal_return_unless_building_tools()
