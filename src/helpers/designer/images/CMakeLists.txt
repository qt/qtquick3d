qt_path_join(destination ${QT_INSTALL_DIR} "${INSTALL_QMLDIR}/QtQuick3D/Helpers/designer/images")
qt_copy_or_install(
    FILES
        dummy.png
        dummy16.png
        dummy@2x.png
    DESTINATION "${destination}"
)
