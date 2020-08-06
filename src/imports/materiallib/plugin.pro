CXX_MODULE = qml
TARGET = qtquick3dmaterialplugin
TARGETPATH = QtQuick3D/Materials
QT += quick qml quick3d-private
IMPORT_VERSION = 1.$$QT_MINOR_VERSION
QML_FILES = \
            AluminumBrushedMaterial.qml \
            AluminumEmissiveMaterial.qml \
            AluminumMaterial.qml \
            AluminumAnodizedEmissiveMaterial.qml \
            AluminumAnodizedMaterial.qml \
            CopperMaterial.qml \
            PaperArtisticMaterial.qml \
            PaperOfficeMaterial.qml \
            PlasticStructuredRedMaterial.qml \
            PlasticStructuredRedEmissiveMaterial.qml \
            SteelMilledConcentricMaterial.qml \
            GlassMaterial.qml \
            GlassRefractiveMaterial.qml \
            FrostedGlassMaterial.qml \
            FrostedGlassSinglePassMaterial.qml

MATERIAL_IMAGE_FILES += \
    maps/art_paper_normal.png \
    maps/art_paper_trans.png \
    maps/brushed_a.png \
    maps/brushed_full_contrast.png \
    maps/concentric_milled_steel.png \
    maps/concentric_milled_steel_aniso.png \
    maps/emissive.png \
    maps/emissive_mask.png \
    maps/grunge_b.png \
    maps/grunge_d.png \
    maps/paper_diffuse.png \
    maps/paper_trans.png \
    maps/randomGradient1D.png \
    maps/randomGradient2D.png \
    maps/randomGradient3D.png \
    maps/randomGradient4D.png \
    maps/shadow.png \
    maps/spherical_checker.png

QML_FILES += $$MATERIAL_IMAGE_FILES

OTHER_FILES += $$QML_FILES

RESOURCES += \
    qtmateriallibrary.qrc

SOURCES += \
    plugin.cpp

DISTFILES += \
    qmldir

!static: qtConfig(quick-designer): include(designer/designer.pri)

load(qml_plugin)
