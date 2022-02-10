qt_path_join(destination ${QT_INSTALL_DIR} "${INSTALL_QMLDIR}/QtQuick3D/Helpers/designer/images")
qt_copy_or_install(
    FILES
        dummy.png
        dummy16.png
        dummy@2x.png
        lookatnode.png
        lookatnode16.png
        lookatnode@2x.png
        axishelper.png
        axishelper16.png
        axishelper@2x.png
        debugview.png
        debugview16.png
        debugview@2x.png
        gridgeometry.png
        gridgeometry16.png
        gridgeometry@2x.png
        wasdcontroller.png
        wasdcontroller16.png
        wasdcontroller@2x.png
    DESTINATION "${destination}"
)
