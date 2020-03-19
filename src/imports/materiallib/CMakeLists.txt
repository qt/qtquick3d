# Generated from materiallib.pro.

#####################################################################
## qtquick3dmaterialplugin Plugin:
#####################################################################

qt_add_qml_module(qtquick3dmaterialplugin
    URI "QtQuick3D.Materials"
    VERSION "1.${CMAKE_PROJECT_VERSION_MINOR}"
    DESIGNER_SUPPORTED
    CLASSNAME QtQuick3DMaterialPlugin
    DEPENDENCIES
        QtQuick3D/1.0
        QtQuick.Window/2.1
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::Quick
        Qt::Quick3DPrivate
)

# Resources:
set(qtmateriallibrary_resource_files
    "shaders/aluminum.frag"
    "shaders/aluminumAnodized.frag"
    "shaders/aluminumAnodizedEmissive.frag"
    "shaders/aluminumBrushed.frag"
    "shaders/aluminumEmissive.frag"
    "shaders/copper.frag"
    "shaders/frostedThinGlass.frag"
    "shaders/frostedThinGlassBlurX.frag"
    "shaders/frostedThinGlassBlurY.frag"
    "shaders/frostedThinGlassNoop.frag"
    "shaders/frostedThinGlassPreBlur.frag"
    "shaders/frostedThinGlassSp.frag"
    "shaders/paperArtistic.frag"
    "shaders/paperOffice.frag"
    "shaders/plasticStructuredRed.frag"
    "shaders/plasticStructuredRedEmissive.frag"
    "shaders/porcelain.frag"
    "shaders/simpleGlass.frag"
    "shaders/simpleGlassRefractive.frag"
    "shaders/steelMilledConcentric.frag"
)

qt_add_resource(qtquick3dmaterialplugin "qtmateriallibrary"
    PREFIX
        "/"
    FILES
        ${qtmateriallibrary_resource_files}
)


#### Keys ignored in scope 2:.:.:plugin.pro:<TRUE>:
# CXX_MODULE = "qml"
# DISTFILES = "qmldir"
# IMPORT_VERSION = "1.$$QT_MINOR_VERSION"
# MATERIAL_IMAGE_FILES = "maps/randomGradient1D.png" "maps/randomGradient2D.png" "maps/randomGradient3D.png" "maps/randomGradient4D.png" "maps/art_paper_normal.png" "maps/art_paper_trans.png" "maps/asphalt.png" "maps/asphalt_bump.png" "maps/bamboo_natural.png" "maps/bamboo_natural_bump.png" "maps/bamboo_natural_spec.png" "maps/brushed_a.png" "maps/brushed_full_contrast.png" "maps/carbon_fiber.png" "maps/carbon_fiber_aniso.png" "maps/carbon_fiber_bump.png" "maps/carbon_fiber_spec.png" "maps/concentric_milled_steel.png" "maps/concentric_milled_steel_aniso.png" "maps/concrete_plain.png" "maps/concrete_plain_bump.png" "maps/cyclone_mesh_fencing.png" "maps/cyclone_mesh_fencing_normal.png" "maps/emissive.png" "maps/emissive_mask.png" "maps/grunge_b.png" "maps/grunge_d.png" "maps/metal_mesh.png" "maps/metal_mesh_bump.png" "maps/metal_mesh_spec.png" "maps/paper_diffuse.png" "maps/paper_trans.png" "maps/powdercoat_bump_01.png" "maps/shadow.png" "maps/smooth_black_leather.png" "maps/smooth_black_leather_bump.png" "maps/smooth_black_leather_spec.png" "maps/spherical_checker.png" "maps/studded_rubber_bump.png" "maps/walnut.png" "maps/walnut_bump.png" "maps/walnut_spec.png"
# OTHER_FILES = "$$QML_FILES"
# QML_FILES = "AluminumBrushedMaterial.qml" "AluminumEmissiveMaterial.qml" "AluminumMaterial.qml" "AluminumAnodizedEmissiveMaterial.qml" "AluminumAnodizedMaterial.qml" "CopperMaterial.qml" "PaperArtisticMaterial.qml" "PaperOfficeMaterial.qml" "PlasticStructuredRedMaterial.qml" "PlasticStructuredRedEmissiveMaterial.qml" "SteelMilledConcentricMaterial.qml" "GlassMaterial.qml" "GlassRefractiveMaterial.qml" "FrostedGlassMaterial.qml" "FrostedGlassSinglePassMaterial.qml" "$$MATERIAL_IMAGE_FILES"
# TARGETPATH = "QtQuick3D/Materials"

set(qml_files
    "AluminumBrushedMaterial.qml"
    "AluminumEmissiveMaterial.qml"
    "AluminumMaterial.qml"
    "AluminumAnodizedEmissiveMaterial.qml"
    "AluminumAnodizedMaterial.qml"
    "CopperMaterial.qml"
    "PaperArtisticMaterial.qml"
    "PaperOfficeMaterial.qml"
    "PlasticStructuredRedMaterial.qml"
    "PlasticStructuredRedEmissiveMaterial.qml"
    "SteelMilledConcentricMaterial.qml"
    "GlassMaterial.qml"
    "GlassRefractiveMaterial.qml"
    "FrostedGlassMaterial.qml"
    "FrostedGlassSinglePassMaterial.qml"
    "maps/randomGradient1D.png"
    "maps/randomGradient2D.png"
    "maps/randomGradient3D.png"
    "maps/randomGradient4D.png"
    "maps/art_paper_normal.png"
    "maps/art_paper_trans.png"
    "maps/asphalt.png"
    "maps/asphalt_bump.png"
    "maps/bamboo_natural.png"
    "maps/bamboo_natural_bump.png"
    "maps/bamboo_natural_spec.png"
    "maps/brushed_a.png"
    "maps/brushed_full_contrast.png"
    "maps/carbon_fiber.png"
    "maps/carbon_fiber_aniso.png"
    "maps/carbon_fiber_bump.png"
    "maps/carbon_fiber_spec.png"
    "maps/concentric_milled_steel.png"
    "maps/concentric_milled_steel_aniso.png"
    "maps/concrete_plain.png"
    "maps/concrete_plain_bump.png"
    "maps/cyclone_mesh_fencing.png"
    "maps/cyclone_mesh_fencing_normal.png"
    "maps/emissive.png"
    "maps/emissive_mask.png"
    "maps/grunge_b.png"
    "maps/grunge_d.png"
    "maps/metal_mesh.png"
    "maps/metal_mesh_bump.png"
    "maps/metal_mesh_spec.png"
    "maps/paper_diffuse.png"
    "maps/paper_trans.png"
    "maps/powdercoat_bump_01.png"
    "maps/shadow.png"
    "maps/smooth_black_leather.png"
    "maps/smooth_black_leather_bump.png"
    "maps/smooth_black_leather_spec.png"
    "maps/spherical_checker.png"
    "maps/studded_rubber_bump.png"
    "maps/walnut.png"
    "maps/walnut_bump.png"
    "maps/walnut_spec.png"
)
set_source_files_properties(AluminumBrushedMaterial.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)
set_source_files_properties(AluminumEmissiveMaterial.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)
set_source_files_properties(AluminumMaterial.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)
set_source_files_properties(AluminumAnodizedEmissiveMaterial.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)
set_source_files_properties(AluminumAnodizedMaterial.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)
set_source_files_properties(CopperMaterial.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)
set_source_files_properties(PaperArtisticMaterial.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)
set_source_files_properties(PaperOfficeMaterial.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)
set_source_files_properties(PlasticStructuredRedMaterial.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)
set_source_files_properties(PlasticStructuredRedEmissiveMaterial.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)
set_source_files_properties(SteelMilledConcentricMaterial.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)
set_source_files_properties(GlassMaterial.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)
set_source_files_properties(GlassRefractiveMaterial.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)
set_source_files_properties(FrostedGlassMaterial.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)
set_source_files_properties(FrostedGlassSinglePassMaterial.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)
set_source_files_properties(maps/randomGradient1D.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/randomGradient2D.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/randomGradient3D.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/randomGradient4D.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/art_paper_normal.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/art_paper_trans.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/asphalt.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/asphalt_bump.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/bamboo_natural.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/bamboo_natural_bump.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/bamboo_natural_spec.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/brushed_a.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/brushed_full_contrast.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/carbon_fiber.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/carbon_fiber_aniso.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/carbon_fiber_bump.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/carbon_fiber_spec.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/concentric_milled_steel.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/concentric_milled_steel_aniso.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/concrete_plain.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/concrete_plain_bump.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/cyclone_mesh_fencing.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/cyclone_mesh_fencing_normal.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/emissive.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/emissive_mask.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/grunge_b.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/grunge_d.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/metal_mesh.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/metal_mesh_bump.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/metal_mesh_spec.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/paper_diffuse.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/paper_trans.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/powdercoat_bump_01.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/shadow.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/smooth_black_leather.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/smooth_black_leather_bump.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/smooth_black_leather_spec.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/spherical_checker.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/studded_rubber_bump.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/walnut.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/walnut_bump.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)
set_source_files_properties(maps/walnut_spec.png PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)

qt6_target_qml_files(qtquick3dmaterialplugin
    FILES
        ${qml_files}
)
