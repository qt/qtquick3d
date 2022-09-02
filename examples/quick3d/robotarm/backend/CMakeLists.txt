find_package(Qt6 REQUIRED COMPONENTS Gui)

qt_add_qml_module(backend
    URI Backend
    VERSION 1.0
    SOURCES
        animatedparam.cpp
        animatedparam.h
        backend.cpp
        backend.h
)

target_link_libraries(backend PRIVATE Qt6::Gui)
