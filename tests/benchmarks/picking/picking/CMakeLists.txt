# Generated from picking.pro.

#####################################################################
## picking Test:
#####################################################################

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(picking LANGUAGES C CXX ASM)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(picking
    SOURCES
        tst_picking.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3DRuntimeRenderPrivate
)

#### Keys ignored in scope 1:.:.:picking.pro:<TRUE>:
# TEMPLATE = "app"
