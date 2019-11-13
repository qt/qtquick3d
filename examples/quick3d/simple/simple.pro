QT += quick quick3d-private

target.path = $$[QT_INSTALL_EXAMPLES]/quick3d/simple
INSTALLS += target

SOURCES += \
    main.cpp

RESOURCES += \
    qml.qrc

OTHER_FILES += \
    doc/src/*.*
