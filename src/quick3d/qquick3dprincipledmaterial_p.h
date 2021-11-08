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
    Q_PROPERTY(TextureChannelMapping metalnessChannel READ metalnessChannel WRITE setMetalnessChannel NOTIFY metalnessChannelChanged)

    Q_PROPERTY(float specularAmount READ specularAmount WRITE setSpecularAmount NOTIFY specularAmountChanged)
    Q_PROPERTY(QQuick3DTexture *specularMap READ specularMap WRITE setSpecularMap NOTIFY specularMapChanged)
    Q_PROPERTY(float specularTint READ specularTint WRITE setSpecularTint NOTIFY specularTintChanged)

    Q_PROPERTY(float roughness READ roughness WRITE setRoughness NOTIFY roughnessChanged)
    Q_PROPERTY(QQuick3DTexture *roughnessMap READ roughnessMap WRITE setRoughnessMap NOTIFY roughnessMapChanged)
    Q_PROPERTY(TextureChannelMapping roughnessChannel READ roughnessChannel WRITE setRoughnessChannel NOTIFY roughnessChannelChanged)

    Q_PROPERTY(QVector3D emissiveFactor READ emissiveFactor WRITE setEmissiveFactor NOTIFY emissiveFactorChanged)
    Q_PROPERTY(QQuick3DTexture *emissiveMap READ emissiveMap WRITE setEmissiveMap NOTIFY emissiveMapChanged)

    Q_PROPERTY(float opacity READ opacity WRITE setOpacity NOTIFY opacityChanged)
    Q_PROPERTY(QQuick3DTexture *opacityMap READ opacityMap WRITE setOpacityMap NOTIFY opacityMapChanged)
    Q_PROPERTY(TextureChannelMapping opacityChannel READ opacityChannel WRITE setOpacityChannel NOTIFY opacityChannelChanged)

    Q_PROPERTY(QQuick3DTexture *normalMap READ normalMap WRITE setNormalMap NOTIFY normalMapChanged)
    Q_PROPERTY(float normalStrength READ normalStrength WRITE setNormalStrength NOTIFY normalStrengthChanged)

    Q_PROPERTY(QQuick3DTexture *specularReflectionMap READ specularReflectionMap WRITE setSpecularReflectionMap NOTIFY specularReflectionMapChanged)

    Q_PROPERTY(QQuick3DTexture *occlusionMap READ occlusionMap WRITE setOcclusionMap NOTIFY occlusionMapChanged)
    Q_PROPERTY(TextureChannelMapping occlusionChannel READ occlusionChannel WRITE setOcclusionChannel NOTIFY occlusionChannelChanged)
    Q_PROPERTY(float occlusionAmount READ occlusionAmount WRITE setOcclusionAmount NOTIFY occlusionAmountChanged)

    Q_PROPERTY(AlphaMode alphaMode READ alphaMode WRITE setAlphaMode NOTIFY alphaModeChanged)
    Q_PROPERTY(float alphaCutoff READ alphaCutoff WRITE setAlphaCutoff NOTIFY alphaCutoffChanged)

    Q_PROPERTY(float pointSize READ pointSize WRITE setPointSize NOTIFY pointSizeChanged)
    Q_PROPERTY(float lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)

    Q_PROPERTY(QQuick3DTexture *heightMap READ heightMap WRITE setHeightMap NOTIFY heightMapChanged REVISION(6, 2))
    Q_PROPERTY(TextureChannelMapping heightChannel READ heightChannel WRITE setHeightChannel NOTIFY heightChannelChanged REVISION(6, 2))
    Q_PROPERTY(float heightAmount READ heightAmount WRITE setHeightAmount NOTIFY heightAmountChanged REVISION(6, 2))
    Q_PROPERTY(int minHeightMapSamples READ minHeightMapSamples WRITE setMinHeightMapSamples NOTIFY minHeightMapSamplesChanged REVISION(6, 2))
    Q_PROPERTY(int maxHeightMapSamples READ maxHeightMapSamples WRITE setMaxHeightMapSamples NOTIFY maxHeightMapSamplesChanged REVISION(6, 2))

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

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void markAllDirty() override;
    void itemChange(ItemChange, const ItemChangeData &) override;
private:
    using ConnectionMap = QHash<QByteArray, QMetaObject::Connection>;

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
        HeightDirty = 0x00002000
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
    float m_specularAmount = 0.5f;
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

    quint32 m_dirtyAttributes = 0xffffffff; // all dirty by default
    void markDirty(DirtyType type);
};

QT_END_NAMESPACE

#endif // QSSGPRINCIPLEDMATERIAL_H
