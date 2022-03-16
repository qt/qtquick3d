qt_internal_add_manual_test(manual_test_3d_input
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
)

# Resources:
set(qml_resource_files
    "main.qml"
    "InputDebugger.qml"
    "custom.frag"
)

qt_internal_add_resource(manual_test_3d_input "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
