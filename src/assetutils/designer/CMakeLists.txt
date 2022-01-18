qt_path_join(destination ${QT_INSTALL_DIR} "${INSTALL_QMLDIR}/QtQuick3D/AssetUtils/designer")
qt_copy_or_install(
    FILES
        RuntimeLoaderSection.qml
        RuntimeLoaderSpecifics.qml
        NodeSection.qml
        IdComboBox.qml
        assetutils.metainfo
    DESTINATION "${destination}"
)

add_subdirectory(images)
