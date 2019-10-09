TEMPLATE = app

QT = testlib quick3d-private

requires(qtConfig(private_tests))

CONFIG += testcase
SOURCES = tst_qquick3darealight.cpp
