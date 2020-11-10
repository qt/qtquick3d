# Generated from scenegrabber.pro.

#####################################################################
## qmlscenegrabber Binary:
#####################################################################

qt_internal_add_executable(qmlscenegrabber
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

#### Keys ignored in scope 1:.:.:scenegrabber.pro:<TRUE>:
# QML_IMPORT_NAME = "QtQuick3D.Lancelot"
# QML_IMPORT_VERSION = "$$QT_VERSION"

## Scopes:
#####################################################################

set_target_properties(qmlscenegrabber PROPERTIES
    QT_QML_MODULE_VERSION ${CMAKE_PROJECT_VERSION}
    QT_QML_MODULE_URI QtQuick3D.Lancelot
)

qt6_qml_type_registration(qmlscenegrabber)
