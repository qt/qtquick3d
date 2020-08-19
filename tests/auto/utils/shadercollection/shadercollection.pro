QT += testlib shadertools-private gui-private quick3dutils-private

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

SOURCES +=  \
    tst_shadercollection.cpp

requires(qtHaveModule(shadertools))
