// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGPRINCIPLEDMATERIAL_H
#define QSSGPRINCIPLEDMATERIAL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick3D/private/qquick3dmaterial_p.h>
#include <QtQuick3D/private/qquick3dtexture_p.h>

#include <QColor>
#include <QHash>

QT_BEGIN_NAMESPACE

class Q_QUICK3D_EXPORT QQuick3DPrincipledMaterial : public QQuick3DMaterial
{
    Q_OBJECT
    Q_PROPERTY(Lighting lighting READ lighting WRITE setLighting NOTIFY lightingChanged)
    Q_PROPERTY(BlendMode blendMode READ blendMode WRITE setBlendMode NOTIFY blendModeChanged)

    Q_PROPERTY(QColor baseColor READ baseColor WRITE setBaseColor NOTIFY baseColorChanged)
    Q_PROPERTY(QQuick3DTexture *baseColorMap READ baseColorMap WRITE setBaseColorMap NOTIFY baseColorMapChanged)

    Q_PROPERTY(float metalness READ metalness WRITE setMetalness NOTIFY metalnessChanged)
    Q_PROPERTY(QQuick3DTexture *metalnessMap READ metalnessMap WRITE setMetalnessMap NOTIFY metalnessMapChanged)
    Q_PROPERTY(QQuick3DMaterial::TextureChannelMapping metalnessChannel READ metalnessChannel WRITE setMetalnessChannel NOTIFY metalnessChannelChanged)

    Q_PROPERTY(float specularAmount READ specularAmount WRITE setSpecularAmount NOTIFY specularAmountChanged)
    Q_PROPERTY(QQuick3DTexture *specularMap READ specularMap WRITE setSpecularMap NOTIFY specularMapChanged)
    Q_PROPERTY(float specularTint READ specularTint WRITE setSpecularTint NOTIFY specularTintChanged)

    Q_PROPERTY(float roughness READ roughness WRITE setRoughness NOTIFY roughnessChanged)
    Q_PROPERTY(QQuick3DTexture *roughnessMap READ roughnessMap WRITE setRoughnessMap NOTIFY roughnessMapChanged)
    Q_PROPERTY(QQuick3DMaterial::TextureChannelMapping roughnessChannel READ roughnessChannel WRITE setRoughnessChannel NOTIFY roughnessChannelChanged)

    Q_PROPERTY(QVector3D emissiveFactor READ emissiveFactor WRITE setEmissiveFactor NOTIFY emissiveFactorChanged)
    Q_PROPERTY(QQuick3DTexture *emissiveMap READ emissiveMap WRITE setEmissiveMap NOTIFY emissiveMapChanged)

    Q_PROPERTY(float opacity READ opacity WRITE setOpacity NOTIFY opacityChanged)
    Q_PROPERTY(QQuick3DTexture *opacityMap READ opacityMap WRITE setOpacityMap NOTIFY opacityMapChanged)
    Q_PROPERTY(QQuick3DMaterial::TextureChannelMapping opacityChannel READ opacityChannel WRITE setOpacityChannel NOTIFY opacityChannelChanged)

    Q_PROPERTY(QQuick3DTexture *normalMap READ normalMap WRITE setNormalMap NOTIFY normalMapChanged)
    Q_PROPERTY(float normalStrength READ normalStrength WRITE setNormalStrength NOTIFY normalStrengthChanged)

    Q_PROPERTY(QQuick3DTexture *specularReflectionMap READ specularReflectionMap WRITE setSpecularReflectionMap NOTIFY specularReflectionMapChanged)

    Q_PROPERTY(QQuick3DTexture *occlusionMap READ occlusionMap WRITE setOcclusionMap NOTIFY occlusionMapChanged)
    Q_PROPERTY(QQuick3DMaterial::TextureChannelMapping occlusionChannel READ occlusionChannel WRITE setOcclusionChannel NOTIFY occlusionChannelChanged)
    Q_PROPERTY(float occlusionAmount READ occlusionAmount WRITE setOcclusionAmount NOTIFY occlusionAmountChanged)

    Q_PROPERTY(AlphaMode alphaMode READ alphaMode WRITE setAlphaMode NOTIFY alphaModeChanged)
    Q_PROPERTY(float alphaCutoff READ alphaCutoff WRITE setAlphaCutoff NOTIFY alphaCutoffChanged)

    Q_PROPERTY(float pointSize READ pointSize WRITE setPointSize NOTIFY pointSizeChanged)
    Q_PROPERTY(float lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)

    Q_PROPERTY(QQuick3DTexture *heightMap READ heightMap WRITE setHeightMap NOTIFY heightMapChanged REVISION(6, 2))
    Q_PROPERTY(QQuick3DMaterial::TextureChannelMapping heightChannel READ heightChannel WRITE setHeightChannel NOTIFY heightChannelChanged REVISION(6, 2))
    Q_PROPERTY(float heightAmount READ heightAmount WRITE setHeightAmount NOTIFY heightAmountChanged REVISION(6, 2))
    Q_PROPERTY(int minHeightMapSamples READ minHeightMapSamples WRITE setMinHeightMapSamples NOTIFY minHeightMapSamplesChanged REVISION(6, 2))
    Q_PROPERTY(int maxHeightMapSamples READ maxHeightMapSamples WRITE setMaxHeightMapSamples NOTIFY maxHeightMapSamplesChanged REVISION(6, 2))

    Q_PROPERTY(float clearcoatAmount READ clearcoatAmount WRITE setClearcoatAmount NOTIFY clearcoatAmountChanged REVISION(6, 3))
    Q_PROPERTY(QQuick3DTexture *clearcoatMap READ clearcoatMap WRITE setClearcoatMap NOTIFY clearcoatMapChanged REVISION(6, 3))
    Q_PROPERTY(QQuick3DMaterial::TextureChannelMapping clearcoatChannel READ clearcoatChannel WRITE setClearcoatChannel NOTIFY
                       clearcoatChannelChanged REVISION(6, 3))
    Q_PROPERTY(float clearcoatRoughnessAmount READ clearcoatRoughnessAmount WRITE setClearcoatRoughnessAmount NOTIFY
                       clearcoatRoughnessAmountChanged REVISION(6, 3))
    Q_PROPERTY(QQuick3DMaterial::TextureChannelMapping clearcoatRoughnessChannel READ clearcoatRoughnessChannel WRITE
                       setClearcoatRoughnessChannel NOTIFY clearcoatRoughnessChannelChanged REVISION(6, 3))
    Q_PROPERTY(QQuick3DTexture *clearcoatRoughnessMap READ clearcoatRoughnessMap WRITE setClearcoatRoughnessMap NOTIFY
                       clearcoatRoughnessMapChanged REVISION(6, 3))
    Q_PROPERTY(QQuick3DTexture *clearcoatNormalMap READ clearcoatNormalMap WRITE setClearcoatNormalMap NOTIFY
                       clearcoatNormalMapChanged REVISION(6, 3))

    Q_PROPERTY(float transmissionFactor READ transmissionFactor WRITE setTransmissionFactor NOTIFY transmissionFactorChanged)
    Q_PROPERTY(QQuick3DTexture * transmissionMap READ transmissionMap WRITE setTransmissionMap NOTIFY transmissionMapChanged)
    Q_PROPERTY(QQuick3DMaterial::TextureChannelMapping transmissionChannel READ transmissionChannel WRITE setTransmissionChannel NOTIFY transmissionChannelChanged)

    Q_PROPERTY(float thicknessFactor READ thicknessFactor WRITE setThicknessFactor NOTIFY thicknessFactorChanged REVISION(6, 3))
    Q_PROPERTY(QQuick3DTexture *thicknessMap READ thicknessMap WRITE setThicknessMap NOTIFY thicknessMapChanged REVISION(6, 3))
    Q_PROPERTY(QQuick3DMaterial::TextureChannelMapping thicknessChannel READ thicknessChannel WRITE setThicknessChannel NOTIFY
                       thicknessChannelChanged REVISION(6, 3))
    Q_PROPERTY(float attenuationDistance READ attenuationDistance WRITE setAttenuationDistance NOTIFY attenuationDistanceChanged REVISION(6, 3))
    Q_PROPERTY(QColor attenuationColor READ attenuationColor WRITE setAttenuationColor NOTIFY attenuationColorChanged REVISION(6, 3))

    Q_PROPERTY(float indexOfRefraction READ indexOfRefraction WRITE setIndexOfRefraction NOTIFY indexOfRefractionChanged REVISION(6, 3))

    Q_PROPERTY(bool vertexColorsEnabled READ vertexColorsEnabled WRITE setVertexColorsEnabled NOTIFY vertexColorsEnabledChanged REVISION(6, 5))

    QML_NAMED_ELEMENT(PrincipledMaterial)

public:
    enum Lighting {
        NoLighting = 0,
        FragmentLighting
    };
    Q_ENUM(Lighting)

    enum BlendMode {
        SourceOver = 0,
        Screen,
        Multiply
    };
    Q_ENUM(BlendMode)

    enum AlphaMode {
        Default = 0,
        Mask,
        Blend,
        Opaque
    };
    Q_ENUM(AlphaMode)

    explicit QQuick3DPrincipledMaterial(QQuick3DObject *parent = nullptr);
    ~QQuick3DPrincipledMaterial() override;

    Lighting lighting() const;
    BlendMode blendMode() const;
    QColor baseColor() const;
    QQuick3DTexture *baseColorMap() const;
    QQuick3DTexture *emissiveMap() const;
    QVector3D emissiveFactor() const;
    QQuick3DTexture *specularReflectionMap() const;
    QQuick3DTexture *specularMap() const;
    float specularTint() const;
    float specularAmount() const;
    float roughness() const;
    QQuick3DTexture *roughnessMap() const;
    float opacity() const;
    QQuick3DTexture *opacityMap() const;
    QQuick3DTexture *normalMap() const;
    float metalness() const;
    QQuick3DTexture *metalnessMap() const;
    float normalStrength() const;
    QQuick3DTexture *occlusionMap() const;
    float occlusionAmount() const;
    AlphaMode alphaMode() const;
    float alphaCutoff() const;
    TextureChannelMapping metalnessChannel() const;
    TextureChannelMapping roughnessChannel() const;
    TextureChannelMapping opacityChannel() const;
    TextureChannelMapping occlusionChannel() const;
    float pointSize() const;
    float lineWidth() const;
    Q_REVISION(6, 2) QQuick3DTexture *heightMap() const;
    Q_REVISION(6, 2) TextureChannelMapping heightChannel() const;
    Q_REVISION(6, 2) float heightAmount() const;
    Q_REVISION(6, 2) int minHeightMapSamples() const;
    Q_REVISION(6, 2) int maxHeightMapSamples() const;

    Q_REVISION(6, 3) float clearcoatAmount() const;
    Q_REVISION(6, 3) QQuick3DTexture *clearcoatMap() const;
    Q_REVISION(6, 3) TextureChannelMapping clearcoatChannel() const;
    Q_REVISION(6, 3) float clearcoatRoughnessAmount() const;
    Q_REVISION(6, 3) TextureChannelMapping clearcoatRoughnessChannel() const;
    Q_REVISION(6, 3) QQuick3DTexture *clearcoatRoughnessMap() const;
    Q_REVISION(6, 3) QQuick3DTexture *clearcoatNormalMap() const;

    Q_REVISION(6, 3) float transmissionFactor() const;
    Q_REVISION(6, 3) QQuick3DTexture *transmissionMap() const;
    Q_REVISION(6, 3) TextureChannelMapping transmissionChannel() const;

    Q_REVISION(6, 3) float thicknessFactor() const;
    Q_REVISION(6, 3) QQuick3DTexture *thicknessMap() const;
    Q_REVISION(6, 3) const TextureChannelMapping &thicknessChannel() const;
    Q_REVISION(6, 3) float attenuationDistance() const;
    Q_REVISION(6, 3) const QColor &attenuationColor() const;

    Q_REVISION(6, 3) float indexOfRefraction() const;

    Q_REVISION(6, 5) bool vertexColorsEnabled() const;

public Q_SLOTS:
    void setLighting(QQuick3DPrincipledMaterial::Lighting lighting);
    void setBlendMode(QQuick3DPrincipledMaterial::BlendMode blendMode);
    void setBaseColor(QColor baseColor);
    void setBaseColorMap(QQuick3DTexture *baseColorMap);
    void setEmissiveMap(QQuick3DTexture *emissiveMap);
    void setEmissiveFactor(QVector3D emissiveFactor);
    void setSpecularReflectionMap(QQuick3DTexture *specularReflectionMap);
    void setSpecularMap(QQuick3DTexture *specularMap);
    void setSpecularTint(float specularTint);
    void setSpecularAmount(float specularAmount);
    void setRoughness(float roughness);
    void setRoughnessMap(QQuick3DTexture *roughnessMap);
    void setOpacity(float opacity);
    void setOpacityMap(QQuick3DTexture *opacityMap);
    void setNormalMap(QQuick3DTexture *normalMap);
    void setMetalness(float metalnessAmount);
    void setMetalnessMap(QQuick3DTexture * metalnessMap);
    void setNormalStrength(float normalStrength);
    void setOcclusionMap(QQuick3DTexture *occlusionMap);
    void setOcclusionAmount(float occlusionAmount);
    void setAlphaMode(QQuick3DPrincipledMaterial::AlphaMode alphaMode);
    void setAlphaCutoff(float alphaCutoff);
    void setMetalnessChannel(QQuick3DMaterial::TextureChannelMapping channel);
    void setRoughnessChannel(QQuick3DMaterial::TextureChannelMapping channel);
    void setOpacityChannel(QQuick3DMaterial::TextureChannelMapping channel);
    void setOcclusionChannel(QQuick3DMaterial::TextureChannelMapping channel);
    void setPointSize(float size);
    void setLineWidth(float width);
    Q_REVISION(6, 2) void setHeightMap(QQuick3DTexture *heightMap);
    Q_REVISION(6, 2) void setHeightChannel(QQuick3DMaterial::TextureChannelMapping channel);
    Q_REVISION(6, 2) void setHeightAmount(float heightAmount);
    Q_REVISION(6, 2) void setMinHeightMapSamples(int samples);
    Q_REVISION(6, 2) void setMaxHeightMapSamples(int samples);

    Q_REVISION(6, 3) void setClearcoatAmount(float newClearcoatAmount);
    Q_REVISION(6, 3) void setClearcoatMap(QQuick3DTexture *newClearcoatMap);
    Q_REVISION(6, 3) void setClearcoatChannel(QQuick3DMaterial::TextureChannelMapping newClearcoatChannel);
    Q_REVISION(6, 3) void setClearcoatRoughnessAmount(float newClearcoatRoughnessAmount);
    Q_REVISION(6, 3) void setClearcoatRoughnessChannel(QQuick3DMaterial::TextureChannelMapping newClearcoatRoughnessChannel);
    Q_REVISION(6, 3) void setClearcoatRoughnessMap(QQuick3DTexture *newClearcoatRoughnessMap);
    Q_REVISION(6, 3) void setClearcoatNormalMap(QQuick3DTexture *newClearcoatNormalMap);

    Q_REVISION(6, 3) void setTransmissionFactor(float newTransmissionFactor);
    Q_REVISION(6, 3) void setTransmissionMap(QQuick3DTexture *newTransmissionMap);
    Q_REVISION(6, 3) void setTransmissionChannel(QQuick3DMaterial::TextureChannelMapping newTransmissionChannel);

    Q_REVISION(6, 3) void setThicknessFactor(float newThicknessFactor);
    Q_REVISION(6, 3) void setThicknessMap(QQuick3DTexture *newThicknessMap);
    Q_REVISION(6, 3) void setThicknessChannel(const QQuick3DMaterial::TextureChannelMapping &newThicknessChannel);
    Q_REVISION(6, 3) void setAttenuationDistance(float newAttenuationDistance);
    Q_REVISION(6, 3) void setAttenuationColor(const QColor &newAttenuationColor);

    Q_REVISION(6, 3) void setIndexOfRefraction(float indexOfRefraction);

    Q_REVISION(6, 5) void setVertexColorsEnabled(bool vertexColorsEnabled);

Q_SIGNALS:
    void lightingChanged(QQuick3DPrincipledMaterial::Lighting lighting);
    void blendModeChanged(QQuick3DPrincipledMaterial::BlendMode blendMode);
    void baseColorChanged(QColor baseColor);
    void baseColorMapChanged(QQuick3DTexture *baseColorMap);
    void emissiveMapChanged(QQuick3DTexture *emissiveMap);
    void emissiveFactorChanged(QVector3D emissiveFactor);
    void specularReflectionMapChanged(QQuick3DTexture *specularReflectionMap);
    void specularMapChanged(QQuick3DTexture *specularMap);
    void specularTintChanged(float specularTint);
    void specularAmountChanged(float specularAmount);
    void roughnessChanged(float roughness);
    void roughnessMapChanged(QQuick3DTexture *roughnessMap);
    void opacityChanged(float opacity);
    void opacityMapChanged(QQuick3DTexture *opacityMap);
    void normalMapChanged(QQuick3DTexture *normalMap);
    void metalnessChanged(float metalness);
    void metalnessMapChanged(QQuick3DTexture * metalnessMap);
    void normalStrengthChanged(float normalStrength);
    void occlusionMapChanged(QQuick3DTexture *occlusionMap);
    void occlusionAmountChanged(float occlusionAmount);
    void alphaModeChanged(QQuick3DPrincipledMaterial::AlphaMode alphaMode);
    void alphaCutoffChanged(float alphaCutoff);
    void metalnessChannelChanged(QQuick3DMaterial::TextureChannelMapping channel);
    void roughnessChannelChanged(QQuick3DMaterial::TextureChannelMapping channel);
    void opacityChannelChanged(QQuick3DMaterial::TextureChannelMapping channel);
    void occlusionChannelChanged(QQuick3DMaterial::TextureChannelMapping channel);
    void pointSizeChanged();
    void lineWidthChanged();
    Q_REVISION(6, 2) void heightMapChanged(QQuick3DTexture *heightMap);
    Q_REVISION(6, 2) void heightChannelChanged(QQuick3DMaterial::TextureChannelMapping channel);
    Q_REVISION(6, 2) void heightAmountChanged(float heightAmount);
    Q_REVISION(6, 2) void minHeightMapSamplesChanged(int samples);
    Q_REVISION(6, 2) void maxHeightMapSamplesChanged(int samples);

    Q_REVISION(6, 3) void clearcoatAmountChanged(float amount);
    Q_REVISION(6, 3) void clearcoatMapChanged(QQuick3DTexture *texture);
    Q_REVISION(6, 3) void clearcoatChannelChanged(QQuick3DMaterial::TextureChannelMapping channel);
    Q_REVISION(6, 3) void clearcoatRoughnessAmountChanged(float amount);
    Q_REVISION(6, 3) void clearcoatRoughnessChannelChanged(QQuick3DMaterial::TextureChannelMapping channel);
    Q_REVISION(6, 3) void clearcoatRoughnessMapChanged(QQuick3DTexture *texture);
    Q_REVISION(6, 3) void clearcoatNormalMapChanged(QQuick3DTexture *texture);

    Q_REVISION(6, 3) void transmissionFactorChanged(float amount);
    Q_REVISION(6, 3) void transmissionMapChanged(QQuick3DTexture *texture);
    Q_REVISION(6, 3) void transmissionChannelChanged(QQuick3DMaterial::TextureChannelMapping channel);

    Q_REVISION(6, 3) void thicknessFactorChanged(float amount);
    Q_REVISION(6, 3) void thicknessMapChanged(QQuick3DTexture *texture);
    Q_REVISION(6, 3) void thicknessChannelChanged(QQuick3DMaterial::TextureChannelMapping channel);
    Q_REVISION(6, 3) void attenuationDistanceChanged(float distance);
    Q_REVISION(6, 3) void attenuationColorChanged(QColor color);

    Q_REVISION(6, 3) void indexOfRefractionChanged(float indexOfRefraction);

    Q_REVISION(6, 5) void vertexColorsEnabledChanged(bool vertexColorsEnabled);

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void markAllDirty() override;
    void itemChange(ItemChange, const ItemChangeData &) override;
private:
    enum DirtyType {
        LightingModeDirty = 0x00000001,
        BlendModeDirty = 0x00000002,
        BaseColorDirty = 0x00000004,
        EmissiveDirty = 0x00000008,
        SpecularDirty = 0x00000010,
        OpacityDirty = 0x00000020,
        NormalDirty = 0x00000040,
        MetalnessDirty = 0x00000080,
        RoughnessDirty = 0x00000100,
        OcclusionDirty = 0x00000200,
        AlphaModeDirty = 0x00000400,
        PointSizeDirty = 0x00000800,
        LineWidthDirty = 0x00001000,
        HeightDirty = 0x00002000,
        ClearcoatDirty = 0x00004000,
        TransmissionDirty = 0x00008000,
        VolumeDirty = 0x00010000,
        VertexColorsDirty = 0x00020000
    };

    void updateSceneManager(QQuick3DSceneManager *window);

    // Note: The default values for properties that are also present in
    // QSSGShaderCustomMaterialAdapter must match the values there, because a
    // PrincipledMaterial { } and CustomMaterial { } must be identical. Same
    // goes for the custom shader defaults in generateFragmentShader(), keep
    // them in sync.

    Lighting m_lighting = FragmentLighting;
    BlendMode m_blendMode = SourceOver;
    AlphaMode m_alphaMode = Default;
    QColor m_baseColor = Qt::white;
    QQuick3DTexture *m_baseColorMap = nullptr;
    QVector3D m_emissiveFactor;
    QQuick3DTexture *m_emissiveMap = nullptr;

    QQuick3DTexture *m_specularReflectionMap = nullptr;
    QQuick3DTexture *m_specularMap = nullptr;
    QQuick3DTexture *m_roughnessMap = nullptr;
    QQuick3DTexture *m_opacityMap = nullptr;
    QQuick3DTexture *m_normalMap = nullptr;
    QQuick3DTexture *m_metalnessMap = nullptr;
    QQuick3DTexture *m_occlusionMap = nullptr;
    float m_specularTint = 0.0f;
    float m_specularAmount = 1.0f;
    float m_roughness = 0.0f;
    float m_opacity = 1.0f;
    float m_metalnessAmount = 0.0f;
    float m_normalStrength = 1.0f;
    float m_occlusionAmount = 1.0f;
    float m_alphaCutoff = 0.5f;
    TextureChannelMapping m_metalnessChannel = QQuick3DMaterial::B;
    TextureChannelMapping m_roughnessChannel = QQuick3DMaterial::G;
    TextureChannelMapping m_opacityChannel = QQuick3DMaterial::A;
    TextureChannelMapping m_occlusionChannel = QQuick3DMaterial::R;
    float m_pointSize = 1.0f;
    float m_lineWidth = 1.0f;
    QQuick3DTexture *m_heightMap = nullptr;
    TextureChannelMapping m_heightChannel = QQuick3DMaterial::R;
    float m_heightAmount = 0.0f;
    int m_minHeightMapSamples = 8;
    int m_maxHeightMapSamples = 32;
    float m_clearcoatAmount = 0.0f;
    QQuick3DTexture *m_clearcoatMap = nullptr;
    TextureChannelMapping m_clearcoatChannel = QQuick3DMaterial::R;
    float m_clearcoatRoughnessAmount = 0.0f;
    TextureChannelMapping m_clearcoatRoughnessChannel = QQuick3DMaterial::G;
    QQuick3DTexture *m_clearcoatRoughnessMap = nullptr;
    QQuick3DTexture *m_clearcoatNormalMap = nullptr;
    float m_transmissionFactor = 0.0f;
    QQuick3DTexture *m_transmissionMap = nullptr;
    TextureChannelMapping m_transmissionChannel = QQuick3DMaterial::R;
    float m_thicknessFactor = 0.0f;
    QQuick3DTexture *m_thicknessMap = nullptr;
    TextureChannelMapping m_thicknessChannel = QQuick3DMaterial::G;
    float m_attenuationDistance = std::numeric_limits<float>::infinity();
    QColor m_attenuationColor = Qt::white;
    float m_indexOfRefraction = 1.5f;
    bool m_vertexColorsEnabled = true;

    quint32 m_dirtyAttributes = 0xffffffff; // all dirty by default
    void markDirty(DirtyType type);

    static constexpr float ensureNormalized(float val) { return qBound(0.0f, val, 1.0f); }
};

QT_END_NAMESPACE

#endif // QSSGPRINCIPLEDMATERIAL_H
