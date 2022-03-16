qt_internal_add_manual_test(manual_test_reflectionprobe
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
)

set(qml_resource_files
    "main.qml"
    "qt_logo.png"
    "OpenfootageNET_garage-1024.hdr"
    "material_simple.frag"
    "snowflake.png"
)

qt_internal_add_resource(manual_test_reflectionprobe "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
