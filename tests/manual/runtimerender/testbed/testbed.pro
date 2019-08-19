QT += quick3druntimerender-private

SOURCES += \
    main.cpp \
    renderwindow.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/runtimerender/testbed
INSTALLS += target

HEADERS += \
    renderwindow.h
