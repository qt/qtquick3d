# Generated from unlit_lit_test.pro.

#####################################################################
## unlit_lit_test Binary:
#####################################################################

qt_internal_add_manual_test(unlit_lit_test
    GUI
    SOURCES
        rhitest.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3DPrivate
)

# Resources:
set_source_files_properties("../shared/teapot.mesh"
    PROPERTIES QT_RESOURCE_ALIAS "teapot.mesh"
)
set(rhitest_resource_files
    "../shared/teapot.mesh"
    "SomePointLight.qml"
    "qt_logo.png"
    "rhitest.qml"
)

qt_internal_add_resource(unlit_lit_test "rhitest"
    PREFIX
        "/"
    FILES
        ${rhitest_resource_files}
)


#### Keys ignored in scope 1:.:.:unlit_lit_test.pro:<TRUE>:
# OTHER_FILES = "rhitest.qml" "SomePointLight.qml"
