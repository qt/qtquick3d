qt_path_join(destination ${QT_INSTALL_DIR} "${INSTALL_QMLDIR}/QtQuick3D/Helpers/designer")
qt_copy_or_install(
    FILES
        AxisHelperSection.qml
        AxisHelperSpecifics.qml
        DebugViewSection.qml
        DebugViewSpecifics.qml
        GridGeometrySection.qml
        GridGeometrySpecifics.qml
        HeightFieldGeometrySection.qml
        HeightFieldGeometrySpecifics.qml
        InstanceModelSection.qml
        InstanceModelSpecifics.qml
        InstanceRepeaterSection.qml
        InstanceRepeaterSpecifics.qml
        LookAtNodeSection.qml
        LookAtNodeSpecifics.qml
        NodeSection.qml
        Repeater3DSection.qml
        WasdControllerSection.qml
        WasdControllerSpecifics.qml
        IdComboBox.qml
        helpers.metainfo
    DESTINATION "${destination}"
)

add_subdirectory(images)
