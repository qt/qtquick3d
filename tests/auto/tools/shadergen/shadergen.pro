QT += testlib quick3d-private qml-private gui-private quick3druntimerender-private

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

TOOLSPWD = ../../../../tools

INCLUDEPATH += $$TOOLSPWD/shadergen

HEADERS += $$TOOLSPWD/shadergen/parser.h

SOURCES += $$TOOLSPWD/shadergen/parser.cpp

SOURCES +=  \
    tst_shadergen.cpp

RESOURCES += \
    qml.qrc
