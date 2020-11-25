CONFIG += testcase
TARGET = tst_quick3dsmoketest

macos:CONFIG -= app_bundle

QT += testlib quick3d-private quick3druntimerender-private

SOURCES += \
    tst_smoketest.cpp

OTHER_FILES += \
    data/view.qml

include (../shared/util.pri)
