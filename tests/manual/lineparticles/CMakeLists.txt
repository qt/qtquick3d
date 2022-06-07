cmake_minimum_required(VERSION 3.16)

qt_internal_add_manual_test(manual_test_lineParticles
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::Quick3D
)

set(qml_resource_files
    "qml/main.qml"
    "qml/LineEmitter.qml"
    "qml/LineEmitterSystem.qml"
)

qt_internal_add_resource(manual_test_lineParticles "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

set(image_resource_files
    "qml/images/qt_logo.png"
)

qt_internal_add_resource(manual_test_lineParticles "image"
    PREFIX
        "/"
    FILES
        ${image_resource_files}
)
