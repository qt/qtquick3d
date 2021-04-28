cmake_minimum_required(VERSION 3.14)
project(runtimeloader LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick3d/runtimeloader")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Quick3D)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(runtimeloader
    main.cpp
)
set_target_properties(runtimeloader PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(runtimeloader PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Quick3D
)

if(TARGET Qt::Widgets)
    target_compile_definitions(runtimeloader PUBLIC
        HAS_MODULE_QT_WIDGETS
    )

    target_link_libraries(runtimeloader PUBLIC
        Qt::Widgets
    )
endif()


# Resources:
set(qml_resource_files
    "main.qml"
    "maps/OpenfootageNET_lowerAustria01-1024.hdr"
)

qt6_add_resources(runtimeloader "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

install(TARGETS runtimeloader
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
