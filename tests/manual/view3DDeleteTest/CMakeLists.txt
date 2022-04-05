qt_internal_add_manual_test(manual_test_view3DDeleteTest
    GUI
    SOURCES
        main.cpp
    DEFINES
        QT_DEPRECATED_WARNINGS
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
)

# Resources:
set(qml_resource_files
    "main.qml"
    "view3DComponent.qml"
)

qt_internal_add_resource(manual_test_view3DDeleteTest "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
