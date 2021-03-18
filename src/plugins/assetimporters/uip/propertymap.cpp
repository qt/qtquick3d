/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "propertymap.h"

#include "datamodelparser.h"

QT_BEGIN_NAMESPACE

PropertyMap *PropertyMap::instance()
{
    static PropertyMap p;
    return &p;
}

PropertyMap::PropertiesMap* PropertyMap::propertiesForType(GraphObject::Type type)
{
    if (m_properties.contains(type))
        return m_properties[type];

    return nullptr;
}

QVariant PropertyMap::getDefaultValue(GraphObject::Type type, const QString &property)
{
    QVariant value;

    if (m_properties.contains(type)) {
        auto properties = m_properties[type];
        if (properties->contains(property))
            value = properties->value(property).defaultValue;
    }

    return value;
}

bool PropertyMap::isDefaultValue(GraphObject::Type type, const QString &property, const QVariant &value)
{
    bool isTheSame = value == getDefaultValue(type, property);
    return isTheSame;
}

namespace  {

void insertNodeProperties(PropertyMap::PropertiesMap *node)
{
    node->insert(QStringLiteral("position"), PropertyMap::Property(QStringLiteral("position"), Q3DS::Vector, QVector3D(0, 0, 0)));
    node->insert(QStringLiteral("position.x"), PropertyMap::Property(QStringLiteral("position.x"), Q3DS::Float, 0.0f));
    node->insert(QStringLiteral("position.y"), PropertyMap::Property(QStringLiteral("position.y"), Q3DS::Float, 0.0f));
    node->insert(QStringLiteral("position.z"), PropertyMap::Property(QStringLiteral("position.z"), Q3DS::Float, 0.0f));
    node->insert(QStringLiteral("rotation"), PropertyMap::Property(QStringLiteral("eulerRotation"), Q3DS::Rotation, QVector3D(0, 0, 0)));
    node->insert(QStringLiteral("rotation.x"), PropertyMap::Property(QStringLiteral("eulerRotation.x"), Q3DS::Float, 0.0f));
    node->insert(QStringLiteral("rotation.y"), PropertyMap::Property(QStringLiteral("eulerRotation.y"), Q3DS::Float, 0.0f));
    node->insert(QStringLiteral("rotation.z"), PropertyMap::Property(QStringLiteral("eulerRotation.z"), Q3DS::Float, 0.0f));
    node->insert(QStringLiteral("scale"), PropertyMap::Property(QStringLiteral("scale"), Q3DS::Vector, QVector3D(1.f, 1.f, 1.f)));
    node->insert(QStringLiteral("scale.x"), PropertyMap::Property(QStringLiteral("scale.x"), Q3DS::Float, 1.0f));
    node->insert(QStringLiteral("scale.y"), PropertyMap::Property(QStringLiteral("scale.y"), Q3DS::Float, 1.0f));
    node->insert(QStringLiteral("scale.z"), PropertyMap::Property(QStringLiteral("scale.z"), Q3DS::Float, 1.0f));
    node->insert(QStringLiteral("pivot"), PropertyMap::Property(QStringLiteral("pivot"), Q3DS::Vector, QVector3D(0, 0, 0)));
    node->insert(QStringLiteral("pivot.x"), PropertyMap::Property(QStringLiteral("pivot.x"), Q3DS::Float, 0.0f));
    node->insert(QStringLiteral("pivot.y"), PropertyMap::Property(QStringLiteral("pivot.y"), Q3DS::Float, 0.0f));
    node->insert(QStringLiteral("pivot.z"), PropertyMap::Property(QStringLiteral("pivot.z"), Q3DS::Float, 0.0f));
    node->insert(QStringLiteral("opacity"), PropertyMap::Property(QStringLiteral("opacity"), Q3DS::Float, 1.0f));
    node->insert(QStringLiteral("visible"), PropertyMap::Property(QStringLiteral("visible"), Q3DS::Boolean, true));
    node->insert(QStringLiteral("eyeball"), PropertyMap::Property(QStringLiteral("visible"), Q3DS::Boolean, true));
}

}

PropertyMap::PropertyMap()
{
    // Image
    PropertiesMap *image = new PropertiesMap;
    image->insert(QStringLiteral("scaleu"), Property(QStringLiteral("scaleU"), Q3DS::Float, 1.0f));
    image->insert(QStringLiteral("scalev"), Property(QStringLiteral("scaleV"), Q3DS::Float, 1.0f));
    image->insert(QStringLiteral("mappingmode"), Property(QStringLiteral("mappingMode"), Q3DS::Enum, QStringLiteral("Texture.UV")));
    image->insert(QStringLiteral("tilingmodehorz"), Property(QStringLiteral("tilingModeHorizontal"), Q3DS::Enum, QStringLiteral("Texture.ClampToEdge")));
    image->insert(QStringLiteral("tilingmodevert"), Property(QStringLiteral("tilingModeVertical"), Q3DS::Enum, QStringLiteral("Texture.ClampToEdge")));
    image->insert(QStringLiteral("rotationuv"), Property(QStringLiteral("rotationUV"), Q3DS::Float, 0.0f));
    image->insert(QStringLiteral("positionu"), Property(QStringLiteral("positionU"), Q3DS::Float, 0.0f));
    image->insert(QStringLiteral("positionv"), Property(QStringLiteral("positionV"), Q3DS::Float, 0.0f));
    image->insert(QStringLiteral("pivotu"), Property(QStringLiteral("pivotU"), Q3DS::Float, 0.0f));
    image->insert(QStringLiteral("pivotv"), Property(QStringLiteral("pivotV"), Q3DS::Float, 0.0f));
    m_properties.insert(GraphObject::Image, image);
    // Group
    PropertiesMap *node = new PropertiesMap;;
    insertNodeProperties(node);
    m_properties.insert(GraphObject::Group, node);

    // Layer
    PropertiesMap *layer = new PropertiesMap;
//    layer->insert(QStringLiteral("horzfields"), Property(QStringLiteral("horizontalFieldValue"), Q3DS::Enum, QStringLiteral("Layer.LeftWidth")));
//    layer->insert(QStringLiteral("vertfields"), Property(QStringLiteral("verticalFieldValue"), Q3DS::Enum, QStringLiteral("Layer.TopHeight")));
//    layer->insert(QStringLiteral("leftunits"), Property(QStringLiteral("leftUnits"), Q3DS::Enum, QStringLiteral("Layer.Percent")));
//    layer->insert(QStringLiteral("rightunits"), Property(QStringLiteral("rightUnits"), Q3DS::Enum, QStringLiteral("Layer.Percent")));
//    layer->insert(QStringLiteral("bottomunits"), Property(QStringLiteral("bottomUnits"), Q3DS::Enum, QStringLiteral("Layer.Percent")));
//    layer->insert(QStringLiteral("topunits"), Property(QStringLiteral("topUnits"), Q3DS::Enum, QStringLiteral("Layer.Percent")));
//    layer->insert(QStringLiteral("widthunits"), Property(QStringLiteral("widthUnits"), Q3DS::Enum, QStringLiteral("Layer.Percent")));
//    layer->insert(QStringLiteral("heightunits"), Property(QStringLiteral("heightUnits"), Q3DS::Enum, QStringLiteral("Layer.Percent")));
    layer->insert(QStringLiteral("eyeball"), PropertyMap::Property(QStringLiteral("visible"), Q3DS::Boolean, true));
    layer->insert(QStringLiteral("visible"), PropertyMap::Property(QStringLiteral("visible"), Q3DS::Boolean, true));
    layer->insert(QStringLiteral("left"), Property(QStringLiteral("left"), Q3DS::Float, 0.0f));
    layer->insert(QStringLiteral("right"), Property(QStringLiteral("right"), Q3DS::Float, 0.0f));
    layer->insert(QStringLiteral("top"), Property(QStringLiteral("top"), Q3DS::Float, 0.0f));
    layer->insert(QStringLiteral("bottom"), Property(QStringLiteral("bottom"), Q3DS::Float, 0.0f));
    layer->insert(QStringLiteral("width"), Property(QStringLiteral("width"), Q3DS::Float, 100.0f));
    layer->insert(QStringLiteral("height"), Property(QStringLiteral("height"), Q3DS::Float, 100.0f));

    layer->insert(QStringLiteral("progressiveaa"), Property(QStringLiteral("antialiasingQuality"), Q3DS::Enum, QStringLiteral("SceneEnvironment.High")));
    layer->insert(QStringLiteral("environment.progressiveaa"), Property(QStringLiteral("environment.antialiasingQuality"), Q3DS::Enum, QStringLiteral("SceneEnvironment.High")));
    layer->insert(QStringLiteral("multisampleaa"), Property(QStringLiteral("antialiasingQuality"), Q3DS::Enum, QStringLiteral("SceneEnvironment.High")));
    layer->insert(QStringLiteral("environment.multisampleaa"), Property(QStringLiteral("environment.antialiasingQuality"), Q3DS::Enum, QStringLiteral("SceneEnvironment.High")));
    layer->insert(QStringLiteral("background"), Property(QStringLiteral("backgroundMode"), Q3DS::Enum, QStringLiteral("SceneEnvironment.Transparent")));
    layer->insert(QStringLiteral("environment.background"), Property(QStringLiteral("environment.backgroundMode"), Q3DS::Enum, QStringLiteral("SceneEnvironment.Transparent")));
    layer->insert(QStringLiteral("backgroundcolor"), Property(QStringLiteral("clearColor"), Q3DS::Color, QColor(Qt::black)));
    layer->insert(QStringLiteral("environment.backgroundcolor"), Property(QStringLiteral("environment.clearColor"), Q3DS::Color, QColor(Qt::black)));
    // Disable because it shouldn't be used this way.
//    layer->insert(QStringLiteral("blendtype"), Property(QStringLiteral("blendType"), Q3DS::Enum, QStringLiteral("SceneEnvironment.Normal")));
//    layer->insert(QStringLiteral("environment.blendtype"), Property(QStringLiteral("environment.blendType"), Q3DS::Enum, QStringLiteral("SceneEnvironment.Normal")));

    layer->insert(QStringLiteral("aostrength"), Property(QStringLiteral("aoStrength"), Q3DS::Float, 0.0f));
    layer->insert(QStringLiteral("aodistance"), Property(QStringLiteral("aoDistance"), Q3DS::Float, 5.0f));
    layer->insert(QStringLiteral("aosoftness"), Property(QStringLiteral("aoSoftness"), Q3DS::Float, 50.0f));
    layer->insert(QStringLiteral("aodither"), Property(QStringLiteral("aoDither"), Q3DS::Boolean, false));
    layer->insert(QStringLiteral("aosamplerate"), Property(QStringLiteral("aoSampleRate"), Q3DS::Long, 2));
    layer->insert(QStringLiteral("aobias"), Property(QStringLiteral("aoBias"), Q3DS::Float, 0.0f));

    layer->insert(QStringLiteral("environment.aostrength"), Property(QStringLiteral("environment.aoStrength"), Q3DS::Float, 0.0f));
    layer->insert(QStringLiteral("environment.aodistance"), Property(QStringLiteral("environment.aoDistance"), Q3DS::Float, 5.0f));
    layer->insert(QStringLiteral("environment.aosoftness"), Property(QStringLiteral("environment.aoSoftness"), Q3DS::Float, 50.0f));
    layer->insert(QStringLiteral("environment.aodither"), Property(QStringLiteral("environment.aoDither"), Q3DS::Boolean, false));
    layer->insert(QStringLiteral("environment.aosamplerate"), Property(QStringLiteral("environment.aoSampleRate"), Q3DS::Long, 2));
    layer->insert(QStringLiteral("environment.aobias"), Property(QStringLiteral("environment.aoBias"), Q3DS::Float, 0.0f));

    layer->insert(QStringLiteral("probebright"), Property(QStringLiteral("probeBrightness"), Q3DS::Float, 0.0f));
    layer->insert(QStringLiteral("fastibl"), Property(QStringLiteral("fastImageBasedLightingEnabled"), Q3DS::Boolean, false));
    layer->insert(QStringLiteral("probehorizon"), Property(QStringLiteral("probeHorizon"), Q3DS::Float, -1.0f));
    layer->insert(QStringLiteral("probefov"), Property(QStringLiteral("probeFieldOfView"), Q3DS::Float, 180.0f));

    layer->insert(QStringLiteral("environment.probebright"), Property(QStringLiteral("environment.probeBrightness"), Q3DS::Float, 0.0f));
    layer->insert(QStringLiteral("environment.fastibl"), Property(QStringLiteral("environment.fastImageBasedLightingEnabled"), Q3DS::Boolean, false));
    layer->insert(QStringLiteral("environment.probehorizon"), Property(QStringLiteral("environment.probeHorizon"), Q3DS::Float, -1.0f));
    layer->insert(QStringLiteral("environment.probefov"), Property(QStringLiteral("environment.probeFieldOfView"), Q3DS::Float, 180.0f));

    layer->insert(QStringLiteral("probe2fade"), Property(QStringLiteral("probe2Fade"), Q3DS::Float, 1.0f));
    layer->insert(QStringLiteral("probe2window"), Property(QStringLiteral("probe2Window"), Q3DS::Float, 1.0f));
    layer->insert(QStringLiteral("probe2pos"), Property(QStringLiteral("probe2Postion"), Q3DS::Float, 0.5f));

    layer->insert(QStringLiteral("environment.probe2fade"), Property(QStringLiteral("environment.probe2Fade"), Q3DS::Float, 1.0f));
    layer->insert(QStringLiteral("environment.probe2window"), Property(QStringLiteral("environment.probe2Window"), Q3DS::Float, 1.0f));
    layer->insert(QStringLiteral("environment.probe2pos"), Property(QStringLiteral("environment.probe2Postion"), Q3DS::Float, 0.5f));

    layer->insert(QStringLiteral("temporalaa"), Property(QStringLiteral("temporalAAEnabled"), Q3DS::Boolean, false));
    layer->insert(QStringLiteral("disabledepthtest"), Property(QStringLiteral("depthTestEnabled"), Q3DS::Boolean, true));
    layer->insert(QStringLiteral("disabledepthprepass"), Property(QStringLiteral("depthPrePassEnabled"), Q3DS::Boolean, false));

    layer->insert(QStringLiteral("environment.temporalaa"), Property(QStringLiteral("environment.temporalAAEnabled"), Q3DS::Boolean, false));
    layer->insert(QStringLiteral("environment.disabledepthtest"), Property(QStringLiteral("environment.depthTestEnabled"), Q3DS::Boolean, true));
    layer->insert(QStringLiteral("environment.disabledepthprepass"), Property(QStringLiteral("environment.depthPrePassEnabled"), Q3DS::Boolean, false));


    m_properties.insert(GraphObject::Layer, layer);

    // Camera
    // Node (properties)
    PropertiesMap *camera = new PropertiesMap;
    insertNodeProperties(camera);

    camera->insert(QStringLiteral("clipnear"), Property(QStringLiteral("clipNear"), Q3DS::Float, 10.0f));
    camera->insert(QStringLiteral("clipfar"), Property(QStringLiteral("clipFar"), Q3DS::Float, 10000.0f));
    camera->insert(QStringLiteral("fov"), Property(QStringLiteral("fieldOfView"), Q3DS::Float, 60.0f));
    camera->insert(QStringLiteral("fovhorizontal"), Property(QStringLiteral("fieldOfViewOrientation"),  Q3DS::String, QStringLiteral("Camera.Vertical")));
    camera->insert(QStringLiteral("enablefrustumculling"), Property(QStringLiteral("frustumCullingEnabled"), Q3DS::Boolean, false));
    m_properties.insert(GraphObject::Camera, camera);

    // Light
    PropertiesMap *light = new PropertiesMap;
    insertNodeProperties(light);

    light->insert(QStringLiteral("lighttype"), Property(QStringLiteral("lightType"), Q3DS::Enum, QStringLiteral("Light.Directional")));
    light->insert(QStringLiteral("lightdiffuse"), Property(QStringLiteral("color"), Q3DS::Color, QColor(Qt::white)));
    light->insert(QStringLiteral("lightspecular"), Property(QStringLiteral("color"), Q3DS::Color, QColor(Qt::white)));
    light->insert(QStringLiteral("lightambient"), Property(QStringLiteral("ambientColor"), Q3DS::Color, QColor(Qt::black)));
    light->insert(QStringLiteral("brightness"), Property(QStringLiteral("brightness"), Q3DS::Float, 100.0f));
    light->insert(QStringLiteral("constantfade"), Property(QStringLiteral("constantFade"), Q3DS::Float, 1.0f));
    light->insert(QStringLiteral("linearfade"), Property(QStringLiteral("linearFade"), Q3DS::Float, 0.0f));
    light->insert(QStringLiteral("expfade"), Property(QStringLiteral("quadraticFade"), Q3DS::Float, 1.0f));
    light->insert(QStringLiteral("areawidth"), Property(QStringLiteral("width"), Q3DS::Float, 100.0f));
    light->insert(QStringLiteral("areaheight"), Property(QStringLiteral("height"), Q3DS::Float, 100.0f));
    light->insert(QStringLiteral("castshadow"), Property(QStringLiteral("castsShadow"), Q3DS::Boolean, false));
    light->insert(QStringLiteral("shdwbias"), Property(QStringLiteral("shadowBias"), Q3DS::Float, 0.0f));
    light->insert(QStringLiteral("shdwfactor"), Property(QStringLiteral("shadowFactor"), Q3DS::Float, 5.0f));
    light->insert(QStringLiteral("shdwmapres"), Property(QStringLiteral("shadowMapQuality"),
                  Q3DS::Enum, QStringLiteral("Light.ShadowMapQualityMedium")));
    light->insert(QStringLiteral("shdwmapfar"), Property(QStringLiteral("shadowMapFar"), Q3DS::Float, 5000.0f));
    light->insert(QStringLiteral("shdwmapfov"), Property(QStringLiteral("shadowMapFieldOfView"), Q3DS::Float, 90.0f));
    light->insert(QStringLiteral("shdwfilter"), Property(QStringLiteral("shadowFilter"), Q3DS::Float, 35.0f));
    m_properties.insert(GraphObject::Light, light);

    // Model
    PropertiesMap *model = new PropertiesMap;
    insertNodeProperties(model);
    model->insert(QStringLiteral("tessellation"), Property(QStringLiteral("tessellationMode"), Q3DS::Enum, QStringLiteral("Model.NoTessellation")));
    model->insert(QStringLiteral("edgetess"), Property(QStringLiteral("edgeTessellation"), Q3DS::Float, 1.0f));
    model->insert(QStringLiteral("innertess"), Property(QStringLiteral("innerTessellation"), Q3DS::Float, 1.0f));
    m_properties.insert(GraphObject::Model, model);

    // Component
    PropertiesMap *component = new PropertiesMap;
    insertNodeProperties(component);
    m_properties.insert(GraphObject::Component, component);

    // DefaultMaterial
    PropertiesMap *defaultMaterial = new PropertiesMap;
    defaultMaterial->insert(QStringLiteral("shaderlighting"), Property(QStringLiteral("lighting"), Q3DS::Enum, QStringLiteral("DefaultMaterial.FragmentLighting")));
    defaultMaterial->insert(QStringLiteral("blendmode"), Property(QStringLiteral("blendMode"), Q3DS::Enum, QStringLiteral("DefaultMaterial.SourceOver")));
    defaultMaterial->insert(QStringLiteral("diffuse"), Property(QStringLiteral("diffuseColor"), Q3DS::Color, QColor(Qt::white)));
    defaultMaterial->insert(QStringLiteral("emissivepower"), Property(QStringLiteral("emissiveFactor"), Q3DS::Float, 0.0f));
    defaultMaterial->insert(QStringLiteral("emissivecolor"), Property(QStringLiteral("emissiveColor"), Q3DS::Color, QColor(Qt::white)));
    defaultMaterial->insert(QStringLiteral("specularmodel"), Property(QStringLiteral("specularModel"), Q3DS::Enum, QStringLiteral("DefaultMaterial.Default")));
    defaultMaterial->insert(QStringLiteral("speculartint"), Property(QStringLiteral("specularTint"), Q3DS::Color, QColor(Qt::white)));
    defaultMaterial->insert(QStringLiteral("ior"), Property(QStringLiteral("indexOfRefraction"), Q3DS::Float, 0.2f));
    defaultMaterial->insert(QStringLiteral("fresnelPower"), Property(QStringLiteral("fresnelFactor"), Q3DS::Float, 0.0f));
    defaultMaterial->insert(QStringLiteral("specularamount"), Property(QStringLiteral("specularAmount"), Q3DS::Float, 0.0f));
    defaultMaterial->insert(QStringLiteral("specularroughness"), Property(QStringLiteral("specularRoughness"), Q3DS::Float, 50.0f));
    defaultMaterial->insert(QStringLiteral("opacity"), Property(QStringLiteral("opacity"), Q3DS::Float, 1.0f));
    defaultMaterial->insert(QStringLiteral("bumpamount"), Property(QStringLiteral("bumpAmount"), Q3DS::Float, 0.0f));
    defaultMaterial->insert(QStringLiteral("translucentfalloff"), Property(QStringLiteral("translucentFalloff"), Q3DS::Float, 0.0f));
    defaultMaterial->insert(QStringLiteral("diffuselightwrap"), Property(QStringLiteral("diffuseLightWrap"), Q3DS::Float, 0.0f));
    defaultMaterial->insert(QStringLiteral("vertexcolors"), Property(QStringLiteral("vertexColorsEnabled"), Q3DS::Boolean, false));
    defaultMaterial->insert(QStringLiteral("displacementamount"), Property(QStringLiteral("displacementAmount"), Q3DS::Float, 0.0f));

    m_properties.insert(GraphObject::DefaultMaterial, defaultMaterial);

    // CustomMaterial
    PropertiesMap *customMaterial = new PropertiesMap;
    customMaterial->insert(QStringLiteral("displacementamount"), Property(QStringLiteral("displacementAmount"), Q3DS::Float, 0.0f));

    m_properties.insert(GraphObject::CustomMaterial, customMaterial);

    // ReferenceMaterial
    PropertiesMap *referenceMaterial = new PropertiesMap;
    m_properties.insert(GraphObject::ReferencedMaterial, referenceMaterial);

    // Effect
    PropertiesMap *effect = new PropertiesMap;
    m_properties.insert(GraphObject::Effect, effect);

    // Alias
    PropertiesMap *alias = new PropertiesMap;
    insertNodeProperties(alias);
    m_properties.insert(GraphObject::Alias, alias);

    PropertiesMap *text = new PropertiesMap;
    insertNodeProperties(text);
    text->insert(QStringLiteral("textstring"), Property(QStringLiteral("text"), Q3DS::String, QStringLiteral("Text")));
    text->insert(QStringLiteral("textcolor"), Property(QStringLiteral("color"), Q3DS::Color, QColor("0xFF000000")));
    text->insert(QStringLiteral("size"), Property(QStringLiteral("font.pixelSize"), Q3DS::FontSize, 0));
    text->insert(QStringLiteral("font"), Property(QStringLiteral("font.family"), Q3DS::Font, QString("")));
    text->insert(QStringLiteral("horzalign"), Property(QStringLiteral("horizontalAlignment"), Q3DS::Enum, QString("Text.AlignLeft")));
    text->insert(QStringLiteral("vertalign"), Property(QStringLiteral("verticalAlignment"), Q3DS::Enum, QString("Text.AlignTop")));
    text->insert(QStringLiteral("wordWrap"), Property(QStringLiteral("wrapMode"), Q3DS::Enum, QString("Text.NoWrap")));
    text->insert(QStringLiteral("elide"), Property(QStringLiteral("elide"), Q3DS::Enum, QString("Text.ElideNone")));
    m_properties.insert(GraphObject::Text, text);
}

PropertyMap::~PropertyMap()
{
    for (auto proprtyMap : m_properties.values())
        delete proprtyMap;
}

QT_END_NAMESPACE

