QT += quick quick3d-private

HEADERS += \
    mousearea3d.h

SOURCES += \
    main.cpp \
    mousearea3d.cpp

RESOURCES += \
    qml.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick3d/studio
INSTALLS += target

