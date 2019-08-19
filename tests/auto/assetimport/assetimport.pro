QT += testlib
QT += gui quick3dassetimport-private core

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

SOURCES +=  tst_assetimport.cpp
