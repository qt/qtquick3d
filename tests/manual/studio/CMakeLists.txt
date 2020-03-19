# Generated from studio.pro.

#####################################################################
## studio Binary:
#####################################################################

qt_add_manual_test(studio
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
    "gizmos/AbstractGizmo.qml"
    "gizmos/CameraGizmo.qml"
    "gizmos/GizmoScaleHelper.qml"
    "gizmos/RotateGizmo.qml"
    "gizmos/RotateGizmoTorus360.qml"
    "gizmos/RotateGizmoTorus90.qml"
    "gizmos/TranslateGizmo.qml"
    "gizmos/TranslateGizmoArrow.qml"
    "main.qml"
    "meshes/gizmoarrow.mesh"
    "meshes/gizmotorus90.mesh"
    "meshes/teapot.mesh"
)

qt_add_resource(studio "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)


#### Keys ignored in scope 1:.:.:studio.pro:<TRUE>:
# OTHER_FILES = "main.qml" "OverlayLabel.qml"
