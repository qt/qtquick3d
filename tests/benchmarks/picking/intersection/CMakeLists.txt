# Generated from intersection.pro.

#####################################################################
## intersection Test:
#####################################################################

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(intersection LANGUAGES C CXX ASM)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(intersection
    SOURCES
        tst_intersection.cpp
    PUBLIC_LIBRARIES
        Qt::Quick3DRuntimeRenderPrivate
)

#### Keys ignored in scope 1:.:.:intersection.pro:<TRUE>:
# TEMPLATE = "app"
