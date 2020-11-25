CONFIG += testcase
TARGET = tst_quick3drendercontrol

macos:CONFIG -= app_bundle

QT += testlib quick3d-private quick3druntimerender-private

SOURCES += \
    tst_rendercontrol.cpp

OTHER_FILES += \
    data/cube.qml

include (../shared/util.pri)
