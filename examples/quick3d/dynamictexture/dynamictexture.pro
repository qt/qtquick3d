QT += quick quick3d-private

target.path = $$[QT_INSTALL_EXAMPLES]/quick3d/dynamictexture
INSTALLS += target

SOURCES += \
    main.cpp

RESOURCES += \
    qml.qrc \
    content/Panel.qml \
    content/cork.jpg \
    content/note-yellow.png \
    content/tack.png \
    CorkBoards.qml \
    Doors.qml \
    meshes/door1.mesh \
    meshes/door2.mesh \
    meshes/wall.mesh \
    main.qml

OTHER_FILES += \
    doc/src/*.*


