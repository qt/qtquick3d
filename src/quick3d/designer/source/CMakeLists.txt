qt_path_join(destination ${QT_INSTALL_DIR} "${INSTALL_QMLDIR}/QtQuick3D/designer/source")
qt_copy_or_install(
    FILES
        cone_model_template.qml
        cube_model_template.qml
        custom_material_default_shader.frag
        cylinder_model_template.qml
        effect_default_shader.frag
        effect_template.qml
        plane_model_template.qml
        sphere_model_template.qml
        view3D_template.qml
    DESTINATION "${destination}"
)
