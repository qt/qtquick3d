QT += quick quick3d

target.path = $$[QT_INSTALL_EXAMPLES]/quick3d/proceduraltexture
INSTALLS += target

SOURCES += \
    gradienttexture.cpp \
    main.cpp

RESOURCES += \
    qml.qrc

HEADERS += \
    gradienttexture.h

CONFIG += qmltypes
QML_IMPORT_NAME = ProceduralTextureExample
QML_IMPORT_MAJOR_VERSION = 1
