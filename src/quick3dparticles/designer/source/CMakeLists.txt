qt_path_join(destination ${QT_INSTALL_DIR} "${INSTALL_QMLDIR}/QtQuick3D/Particles3D/designer/source")
qt_copy_or_install(
    FILES
        particlesystem_template.qml
    DESTINATION "${destination}"
)
