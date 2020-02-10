CXX_MODULE = qml
TARGET = qtquick3deffectplugin
TARGETPATH = QtQuick3D/Effects
QT += quick qml quick3d-private
IMPORT_VERSION = 1.$$QT_MINOR_VERSION
QML_FILES = \
    Fxaa.qml \
    SCurveTonemap.qml \
    Vignette.qml \
    Scatter.qml \
    Flip.qml \
    Emboss.qml \
    EdgeDetect.qml \
    DistortionSpiral.qml \
    DistortionSphere.qml \
    DistortionRipple.qml \
    Desaturate.qml \
    ColorMaster.qml \
    BrushStrokes.qml \
    Blur.qml \
    AdditiveColorGradient.qml

EFFECT_IMAGE_FILES += \
    maps/brushnoise.png

QML_FILES += $$EFFECT_IMAGE_FILES

OTHER_FILES += $$QML_FILES

SOURCES += \
    plugin.cpp

DISTFILES += \
    qmldir

RESOURCES += \
    qteffectlibrary.qrc

load(qml_plugin)
