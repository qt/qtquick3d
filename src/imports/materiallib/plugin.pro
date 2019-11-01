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
    maps/randomGradient1D.png \
    maps/randomGradient2D.png \
    maps/randomGradient3D.png \
    maps/randomGradient4D.png \
    maps/art_paper_normal.png \
    maps/art_paper_trans.png \
    maps/asphalt.png \
    maps/asphalt_bump.png \
    maps/bamboo_natural.png \
    maps/bamboo_natural_bump.png \
    maps/bamboo_natural_spec.png \
    maps/brushed_a.png \
    maps/brushed_full_contrast.png \
    maps/carbon_fiber.png \
    maps/carbon_fiber_aniso.png \
    maps/carbon_fiber_bump.png \
    maps/carbon_fiber_spec.png \
    maps/concentric_milled_steel.png \
    maps/concentric_milled_steel_aniso.png \
    maps/concrete_plain.png \
    maps/concrete_plain_bump.png \
    maps/cyclone_mesh_fencing.png \
    maps/cyclone_mesh_fencing_normal.png \
    maps/emissive.png \
    maps/emissive_mask.png \
    maps/grunge_b.png \
    maps/grunge_d.png \
    maps/metal_mesh.png \
    maps/metal_mesh_bump.png \
    maps/metal_mesh_spec.png \
    maps/paper_diffuse.png \
    maps/paper_trans.png \
    maps/powdercoat_bump_01.png \
    maps/shadow.png \
    maps/smooth_black_leather.png \
    maps/smooth_black_leather_bump.png \
    maps/smooth_black_leather_spec.png \
    maps/spherical_checker.png \
    maps/studded_rubber_bump.png \
    maps/walnut.png \
    maps/walnut_bump.png \
    maps/walnut_spec.png

QML_FILES += $$MATERIAL_IMAGE_FILES

OTHER_FILES += $$QML_FILES

RESOURCES += \
    qtmateriallibrary.qrc

SOURCES += \
    plugin.cpp

DISTFILES += \
    qmldir

load(qml_plugin)
