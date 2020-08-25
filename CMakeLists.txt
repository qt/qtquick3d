# Generated from qtquick3d.pro.

cmake_minimum_required(VERSION 3.15.0)

project(QtQuick3D
    VERSION 6.0.0
    DESCRIPTION "Qt Quick3D Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

# special case begin
find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core Gui Qml Quick ShaderTools)
find_package(Qt6 ${PROJECT_VERSION} CONFIG OPTIONAL_COMPONENTS Network)

# TODO: Fix warnings and remove this
set(QT_REPO_NOT_WARNINGS_CLEAN TRUE)

include(src/quick3d/Qt6Quick3DMacros.cmake)
# special case end

if(INTEGRITY OR QNX OR VXWORKS OR WATCHOS)
    message(NOTICE "Skipping the build as the condition \"NOT INTEGRITY AND NOT QNX AND NOT VXWORKS AND NOT WATCHOS\" is not met.")
    return()
endif()
if(NOT TARGET Qt::Quick)
    message(NOTICE "Skipping the build as the condition \"TARGET Qt::Quick\" is not met.")
    return()
endif()
if(NOT QT_FEATURE_opengl)
    message(NOTICE "Skipping the build as the condition \"QT_FEATURE_opengl\" is not met.")
    return()
endif()
qt_build_repo()
