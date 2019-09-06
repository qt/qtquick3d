CONFIG += testcase
TARGET = tst_quick3d
DESTDIR=..
macos:CONFIG -= app_bundle
CONFIG += console

SOURCES += tst_quick3d.cpp

# Include Lancelot protocol code to communicate with baseline server.
# Assuming that we are in a normal Qt5 source code tree
include(../shared/qbaselinetest.pri)

TEST_HELPER_INSTALLS += .././qmlscenegrabber

TESTDATA += ../data
