# Generated from view3DDeleteTest.pro.

#####################################################################
## view3DDeleteTest Binary:
#####################################################################

qt_internal_add_manual_test(view3DDeleteTest
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
    "view3DComponent.qml"
)

qt_internal_add_resource(view3DDeleteTest "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
