qt_get_tool_target_name(target_name materialeditor)
qt_internal_add_tool(${target_name}
    TOOLS_TARGET Quick3D
    SOURCES
        materialadapter.h materialadapter.cpp
        resourceserver.h resourceserver.cpp
        resourceclient.h resourceclient.cpp
        syntaxhighlighter.h syntaxhighlighter.cpp
        uniformmodel.h uniformmodel.cpp
        custommaterial.h custommaterial.cpp
        message.h message.cpp
        buildmessage.h buildmessage.cpp
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
        Qt::Quick3DUtils
    LIBRARIES
        Qt::Quick3DGlslParserPrivate
        Qt::Quick3DAssetUtilsPrivate
        Qt::Quick3DRuntimeRenderPrivate
)

qt_internal_return_unless_building_tools()

qt_internal_add_resource(${target_name} "qml"
    PREFIX "/"
    FILES
        main.qml
        ShaderEditor.qml
        EditorView.qml
        Preview.qml
        PreviewControls.qml
        qtquickcontrols2.conf
        FrostedGlass.qml
        AboutDialog.qml
        MaterialPropertiesPane.qml
        SaveChangesDialog.qml
        UniformManagerPane.qml
)

qt_internal_add_resource(${target_name} "assets"
    PREFIX "/"
    FILES
        assets/meshes/suzanne.mesh
        assets/skybox/OpenfootageNET_lowerAustria01-1024.hdr
        assets/icons/lightdirectional.png
        assets/icons/lightdirectional@2x.png
        assets/icons/lightpoint.png
        assets/icons/lightpoint@2x.png
        assets/icons/lightspot.png
        assets/icons/lightspot@2x.png
        assets/icons/texture.png
        assets/icons/texture@2x.png
        assets/images/noise.png
)

qt_add_qml_module(${target_name}
    VERSION 1.0
    URI MaterialEditorHelpers
    SOURCES
        materialadapter.h materialadapter.cpp
        syntaxhighlighter.h syntaxhighlighter.cpp
        uniformmodel.h uniformmodel.cpp
        buildmessage.h buildmessage.cpp
)

qt_add_shaders(${target_name} "shaders"
    PREFIX
        "/"
    FILES
        "assets/shaders/frostedGlass.frag"
    SILENT
)
