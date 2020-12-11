CONFIG += testcase
TARGET = tst_quick3dmultiwindow

macos:CONFIG -= app_bundle

QT += testlib quick3d-private quick3druntimerender-private

SOURCES += \
    tst_multiwindow.cpp

OTHER_FILES += \
    data/cube.qml

include (../shared/util.pri)
