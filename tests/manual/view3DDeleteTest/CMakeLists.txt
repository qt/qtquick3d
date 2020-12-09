# Generated from view3DDeleteTest.pro.

#####################################################################
## view3DDeleteTest Binary:
#####################################################################

qt_internal_add_manual_test(view3DDeleteTest
    GUI
    SOURCES
        main.cpp
    DEFINES
        QT_DEPRECATED_WARNINGS
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
    COMPILE_OPTIONS
        /std:c++17
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


#### Keys ignored in scope 1:.:.:view3DDeleteTest.pro:<TRUE>:
# QML_DESIGNER_IMPORT_PATH = <EMPTY>
# QML_IMPORT_PATH = <EMPTY>

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:view3DDeleteTest.pro:QNX:
# target.path = "/tmp/$${TARGET}/bin"

#### Keys ignored in scope 4:.:.:view3DDeleteTest.pro:UNIX AND NOT ANDROID:
# target.path = "/opt/$${TARGET}/bin"

#### Keys ignored in scope 5:.:.:view3DDeleteTest.pro:NOT target.path_ISEMPTY:
# INSTALLS = "target"
