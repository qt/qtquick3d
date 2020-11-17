# Generated from quick3d.pro.

#####################################################################
## qquick3dplugin Plugin:
#####################################################################

qt_internal_add_qml_module(qquick3dplugin
    URI "QtQuick3D"
    VERSION "${CMAKE_PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    CLASSNAME QQuick3DPlugin
    SKIP_TYPE_REGISTRATION
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::QmlPrivate
        Qt::Quick3DPrivate
        Qt::QuickPrivate
)

#### Keys ignored in scope 1:.:.:quick3d.pro:<TRUE>:
# CXX_MODULE = "qml"
# QML_IMPORT_VERSION = "$$QT_VERSION"
# TARGETPATH = "QtQuick3D"

## Scopes:
#####################################################################

if(QT_FEATURE_quick_designer AND QT_BUILD_SHARED_LIBS) # special case handle unconverted static
    get_target_property(__aux_qml_files_install_base qquick3dplugin QT_QML_MODULE_INSTALL_DIR)

    qt_path_join(__aux_qml_files_install_dir "${__aux_qml_files_install_base}" "designer")

    qt_copy_or_install(
        FILES
            designer/BufferInputSection.qml
            designer/BufferInputSpecifics.qml
            designer/BufferSection.qml
            designer/BufferSpecifics.qml
            designer/CustomCameraSection.qml
            designer/CustomCameraSpecifics.qml
            designer/CustomMaterialSection.qml
            designer/CustomMaterialSpecifics.qml
            designer/DefaultMaterialSection.qml
            designer/DefaultMaterialSpecifics.qml
            designer/DirectionalLightSection.qml
            designer/DirectionalLightSpecifics.qml
            designer/EffectSection.qml
            designer/EffectSpecifics.qml
            designer/FrustumCameraSection.qml
            designer/FrustumCameraSpecifics.qml
            designer/IdComboBox.qml
            designer/MaterialSection.qml
            designer/ModelSection.qml
            designer/ModelSpecifics.qml
            designer/NodeSection.qml
            designer/NodeSpecifics.qml
            designer/Object3DSection.qml
            designer/OrthographicCameraSection.qml
            designer/OrthographicCameraSpecifics.qml
            designer/PassSection.qml
            designer/PassSpecifics.qml
            designer/PerspectiveCameraSection.qml
            designer/PerspectiveCameraSpecifics.qml
            designer/PointLightSection.qml
            designer/PointLightSpecifics.qml
            designer/PrincipledMaterialSection.qml
            designer/PrincipledMaterialSpecifics.qml
            designer/SceneEnvironmentSection.qml
            designer/SceneEnvironmentSpecifics.qml
            designer/SetUniformValueSection.qml
            designer/SetUniformValueSpecifics.qml
            designer/ShaderSection.qml
            designer/ShaderSpecifics.qml
            designer/ShadowSection.qml
            designer/SpotLightSection.qml
            designer/SpotLightSpecifics.qml
            designer/TextureInputSection.qml
            designer/TextureInputSpecifics.qml
            designer/TextureSection.qml
            designer/TextureSpecifics.qml
            designer/View3DSection.qml
            designer/View3DSpecifics.qml
            designer/quick3d.metainfo
        DESTINATION "${__aux_qml_files_install_dir}")

    qt_path_join(__aux_qml_files_install_dir "${__aux_qml_files_install_base}" "designer/source")

    qt_copy_or_install(
        FILES
            designer/source/cone_model_template.qml
            designer/source/cube_model_template.qml
            designer/source/custom_material_default_shader.frag
            designer/source/cylinder_model_template.qml
            designer/source/effect_default_shader.frag
            designer/source/effect_template.qml
            designer/source/plane_model_template.qml
            designer/source/sphere_model_template.qml
            designer/source/view3D_template.qml
        DESTINATION "${__aux_qml_files_install_dir}")

    qt_path_join(__aux_qml_files_install_dir "${__aux_qml_files_install_base}" "designer/images")

    qt_copy_or_install(
        FILES
            designer/images/camera.png
            designer/images/camera16.png
            designer/images/camera@2x.png
            designer/images/cone.png
            designer/images/cone16.png
            designer/images/cone@2x.png
            designer/images/cube.png
            designer/images/cube16.png
            designer/images/cube@2x.png
            designer/images/custommaterial.png
            designer/images/custommaterial16.png
            designer/images/custommaterial@2x.png
            designer/images/cylinder.png
            designer/images/cylinder16.png
            designer/images/cylinder@2x.png
            designer/images/dummy.png
            designer/images/dummy16.png
            designer/images/dummy@2x.png
            designer/images/effect.png
            designer/images/effect16.png
            designer/images/effect@2x.png
            designer/images/group.png
            designer/images/group16.png
            designer/images/group@2x.png
            designer/images/light.png
            designer/images/light16.png
            designer/images/light@2x.png
            designer/images/material.png
            designer/images/material16.png
            designer/images/material@2x.png
            designer/images/model16.png
            designer/images/plane.png
            designer/images/plane16.png
            designer/images/plane@2x.png
            designer/images/scene.png
            designer/images/scene16.png
            designer/images/scene@2x.png
            designer/images/shadercommand.png
            designer/images/shadercommand16.png
            designer/images/shadercommand@2x.png
            designer/images/shaderutil.png
            designer/images/shaderutil16.png
            designer/images/shaderutil@2x.png
            designer/images/sphere.png
            designer/images/sphere16.png
            designer/images/sphere@2x.png
            designer/images/texture.png
            designer/images/texture16.png
            designer/images/texture@2x.png
            designer/images/view3D.png
            designer/images/view3D16.png
            designer/images/view3D@2x.png
        DESTINATION "${__aux_qml_files_install_dir}")
endif()
qt_internal_add_docs(qquick3dplugin
    doc/qtquick3d.qdocconf
)

