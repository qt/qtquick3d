# Generated from simplescene.pro.

#####################################################################
## qquick3dsimplescene Test:
#####################################################################

qt_internal_add_test(tst_qquick3dsimplescene
    SOURCES
        ../shared/util.cpp ../shared/util.h
        tst_simplescene.cpp
    INCLUDE_DIRECTORIES
        ../shared
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick3DPrivate
        Qt::Quick3DRuntimeRenderPrivate
)

#### Keys ignored in scope 1:.:.:simplescene.pro:<TRUE>:
# OTHER_FILES = "data/cube.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquick3dsimplescene CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qquick3dsimplescene CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
