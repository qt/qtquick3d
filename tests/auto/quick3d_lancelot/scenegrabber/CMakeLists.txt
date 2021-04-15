# Generated from scenegrabber.pro.

#####################################################################
## qmlscenegrabber Binary:
#####################################################################

qt_internal_add_executable(tst_qquick3dqmlscenegrabber
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.." # special case
    SOURCES
        dynamicgeometry.cpp dynamicgeometry.h
        dynamictexturedata.cpp dynamictexturedata.h
        indextrianglestopology.cpp indextrianglestopology.h
        main.cpp
        nonindextrianglestopology.cpp nonindextrianglestopology.h
        pointstopology.cpp pointstopology.h
        skingeometry.cpp skingeometry.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3DPrivate
)

# Resources:
set(scenegrabber_resource_files
    "trim.ttf"
)

qt_internal_add_resource(tst_qquick3dqmlscenegrabber "scenegrabber"
    PREFIX
        "/"
    FILES
        ${scenegrabber_resource_files}
)

qt_quick3d_internal_bake_lightprobe_hdri(tst_qquick3dqmlscenegrabber "ibl_assets"
    PREFIX
        "/"
    FILES
        "../data/shared/maps/OpenfootageNET_lowerAustria01-1024.hdr"
)

#### Keys ignored in scope 1:.:.:scenegrabber.pro:<TRUE>:
# QML_IMPORT_NAME = "QtQuick3D.Lancelot"
# QML_IMPORT_VERSION = "$$QT_VERSION"

## Scopes:
#####################################################################

set_target_properties(tst_qquick3dqmlscenegrabber PROPERTIES
    QT_QML_MODULE_VERSION ${PROJECT_VERSION}
    QT_QML_MODULE_URI QtQuick3D.Lancelot
)

qt6_qml_type_registration(tst_qquick3dqmlscenegrabber)
