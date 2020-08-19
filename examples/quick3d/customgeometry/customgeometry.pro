QT += quick quick3d

CONFIG += qmltypes
QML_IMPORT_NAME = CustomGeometryExample
QML_IMPORT_MAJOR_VERSION = 1

target.path = $$[QT_INSTALL_EXAMPLES]/quick3d/customgeometry
INSTALLS += target

SOURCES += \
    main.cpp \
    examplegeometry.cpp

HEADERS += \
    examplegeometry.h

RESOURCES += \
    resources.qrc

OTHER_FILES += \
    doc/src/*.*
