qt_internal_add_manual_test(manual_test_loader2
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
    "MyComponent.qml"
)

qt_internal_add_resource(manual_test_loader2 "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
