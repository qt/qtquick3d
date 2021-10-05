cmake_minimum_required(VERSION 3.14)

project(materialeditor VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Quick REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Quick Quick3D Quick3DGlslParserPrivate Quick3DAssetUtils REQUIRED)

set(PROJECT_SOURCES
        materialadapter.h materialadapter.cpp
        resourceserver.h resourceserver.cpp
        resourceclient.h resourceclient.cpp
        syntaxhighlighter.h syntaxhighlighter.cpp
        uniformmodel.h uniformmodel.cpp
        custommaterial.h custommaterial.cpp
        message.h message.cpp
        buildmessage.h buildmessage.cpp
        main.cpp
        qml.qrc
        assets.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(materialeditor
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${GEN_SHADER_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET materialeditor APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(materialeditor SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(materialeditor
          ${PROJECT_SOURCES}
        )
    endif()
endif()

target_compile_definitions(materialeditor
  PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(materialeditor
  PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Quick Qt${QT_VERSION_MAJOR}::Quick3DPrivate PRIVATE Qt${QT_VERSION_MAJOR}::Quick3DGlslParserPrivate Qt::Quick3DAssetUtilsPrivate)

set_target_properties(materialeditor PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_import_qml_plugins(materialeditor)
    qt_finalize_executable(materialeditor)
endif()

qt_add_qml_module(materialeditor
    VERSION 1.0
    URI MaterialEditorHelpers
    SOURCES
        materialadapter.h materialadapter.cpp
        syntaxhighlighter.h syntaxhighlighter.cpp
        uniformmodel.h uniformmodel.cpp
        buildmessage.h buildmessage.cpp
        )

qt_add_shaders(materialeditor "shaders"
    PREFIX
        "/"
    FILES
        "assets/shaders/frostedGlass.frag"
)
