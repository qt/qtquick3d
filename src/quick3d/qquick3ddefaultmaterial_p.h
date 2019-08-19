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

#ifndef QSSGDEFAULTMATERIAL_H
#define QSSGDEFAULTMATERIAL_H

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

class Q_QUICK3D_EXPORT QQuick3DDefaultMaterial : public QQuick3DMaterial
{
    Q_OBJECT
    Q_PROPERTY(QSSGDefaultMaterialLighting lighting READ lighting WRITE setLighting NOTIFY lightingChanged)
    Q_PROPERTY(QSSGDefaultMaterialBlendMode blendMode READ blendMode WRITE setBlendMode NOTIFY blendModeChanged)

    Q_PROPERTY(QColor diffuseColor READ diffuseColor WRITE setDiffuseColor NOTIFY diffuseColorChanged)
    Q_PROPERTY(QQuick3DTexture *diffuseMap READ diffuseMap WRITE setDiffuseMap NOTIFY diffuseMapChanged)
    Q_PROPERTY(QQuick3DTexture *diffuseMap2 READ diffuseMap2 WRITE setDiffuseMap2 NOTIFY diffuseMap2Changed)
    Q_PROPERTY(QQuick3DTexture *diffuseMap3 READ diffuseMap3 WRITE setDiffuseMap3 NOTIFY diffuseMap3Changed)

    Q_PROPERTY(float emissivePower READ emissivePower WRITE setEmissivePower NOTIFY emissivePowerChanged)
    Q_PROPERTY(QQuick3DTexture *emissiveMap READ emissiveMap WRITE setEmissiveMap NOTIFY emissiveMapChanged)
    Q_PROPERTY(QColor emissiveColor READ emissiveColor WRITE setEmissiveColor NOTIFY emissiveColorChanged)

    Q_PROPERTY(QQuick3DTexture *specularReflectionMap READ specularReflectionMap WRITE setSpecularReflectionMap NOTIFY specularReflectionMapChanged)
    Q_PROPERTY(QQuick3DTexture *specularMap READ specularMap WRITE setSpecularMap NOTIFY specularMapChanged)
    Q_PROPERTY(QSSGDefaultMaterialSpecularModel specularModel READ specularModel WRITE setSpecularModel NOTIFY specularModelChanged)
    Q_PROPERTY(QColor specularTint READ specularTint WRITE setSpecularTint NOTIFY specularTintChanged)

    Q_PROPERTY(float indexOfRefraction READ indexOfRefraction WRITE setIndexOfRefraction NOTIFY indexOfRefractionChanged)
    Q_PROPERTY(float fresnelPower READ fresnelPower WRITE setFresnelPower NOTIFY fresnelPowerChanged)
    Q_PROPERTY(float specularAmount READ specularAmount WRITE setSpecularAmount NOTIFY specularAmountChanged)
    Q_PROPERTY(float specularRoughness READ specularRoughness WRITE setSpecularRoughness NOTIFY specularRoughnessChanged)
    Q_PROPERTY(QQuick3DTexture *roughnessMap READ roughnessMap WRITE setRoughnessMap NOTIFY roughnessMapChanged)

    Q_PROPERTY(float opacity READ opacity WRITE setOpacity NOTIFY opacityChanged)
    Q_PROPERTY(QQuick3DTexture *opacityMap READ opacityMap WRITE setOpacityMap NOTIFY opacityMapChanged)

    Q_PROPERTY(QQuick3DTexture *bumpMap READ bumpMap WRITE setBumpMap NOTIFY bumpMapChanged)
    Q_PROPERTY(float bumpAmount READ bumpAmount WRITE setBumpAmount NOTIFY bumpAmountChanged)

    Q_PROPERTY(QQuick3DTexture *normalMap READ normalMap WRITE setNormalMap NOTIFY normalMapChanged)

    Q_PROPERTY(QQuick3DTexture *translucencyMap READ translucencyMap WRITE setTranslucencyMap NOTIFY translucencyMapChanged)
    Q_PROPERTY(float translucentFalloff READ translucentFalloff WRITE setTranslucentFalloff NOTIFY translucentFalloffChanged)

    Q_PROPERTY(float diffuseLightWrap READ diffuseLightWrap WRITE setDiffuseLightWrap NOTIFY diffuseLightWrapChanged)

    Q_PROPERTY(bool vertexColors READ vertexColors WRITE setVertexColors NOTIFY vertexColorsChanged)

public:
    enum QSSGDefaultMaterialLighting { NoLighting = 0, VertexLighting, FragmentLighting };
    Q_ENUM(QSSGDefaultMaterialLighting)

    enum QSSGDefaultMaterialBlendMode { Normal = 0, Screen, Multiply, Overlay, ColorBurn, ColorDodge };
    Q_ENUM(QSSGDefaultMaterialBlendMode)

    enum QSSGDefaultMaterialSpecularModel { Default = 0, KGGX, KWard };
    Q_ENUM(QSSGDefaultMaterialSpecularModel)

    enum QSSGDefaultMaterialDirtyType {
        LightingModeDirty = 0x00000001,
        BlendModeDirty = 0x00000002,
        DiffuseDirty = 0x00000004,
        EmissiveDirty = 0x00000008,
        SpecularDirty = 0x00000010,
        OpacityDirty = 0x00000020,
        BumpDirty = 0x00000040,
        NormalDirty = 0x00000080,
        TranslucencyDirty = 0x00000100,
        VertexColorsDirty = 0x00000200
    };

    using ConnectionMap = QHash<QObject*, QMetaObject::Connection>;

    QQuick3DDefaultMaterial();
    ~QQuick3DDefaultMaterial() override;

    QQuick3DObject::Type type() const override;

    QSSGDefaultMaterialLighting lighting() const;
    QSSGDefaultMaterialBlendMode blendMode() const;
    QColor diffuseColor() const;
    QQuick3DTexture *diffuseMap() const;
    QQuick3DTexture *diffuseMap2() const;
    QQuick3DTexture *diffuseMap3() const;
    float emissivePower() const;
    QQuick3DTexture *emissiveMap() const;
    QColor emissiveColor() const;
    QQuick3DTexture *specularReflectionMap() const;
    QQuick3DTexture *specularMap() const;
    QSSGDefaultMaterialSpecularModel specularModel() const;
    QColor specularTint() const;
    float indexOfRefraction() const;
    float fresnelPower() const;
    float specularAmount() const;
    float specularRoughness() const;
    QQuick3DTexture *roughnessMap() const;
    float opacity() const;
    QQuick3DTexture *opacityMap() const;
    QQuick3DTexture *bumpMap() const;
    float bumpAmount() const;
    QQuick3DTexture *normalMap() const;

    QQuick3DTexture *translucencyMap() const;
    float translucentFalloff() const;
    float diffuseLightWrap() const;
    bool vertexColors() const;

public Q_SLOTS:

    void setLighting(QSSGDefaultMaterialLighting lighting);
    void setBlendMode(QSSGDefaultMaterialBlendMode blendMode);
    void setDiffuseColor(QColor diffuseColor);
    void setDiffuseMap(QQuick3DTexture *diffuseMap);
    void setDiffuseMap2(QQuick3DTexture *diffuseMap2);
    void setDiffuseMap3(QQuick3DTexture *diffuseMap3);
    void setEmissivePower(float emissivePower);
    void setEmissiveMap(QQuick3DTexture *emissiveMap);

    void setEmissiveColor(QColor emissiveColor);
    void setSpecularReflectionMap(QQuick3DTexture *specularReflectionMap);
    void setSpecularMap(QQuick3DTexture *specularMap);
    void setSpecularModel(QSSGDefaultMaterialSpecularModel specularModel);
    void setSpecularTint(QColor specularTint);
    void setIndexOfRefraction(float indexOfRefraction);
    void setFresnelPower(float fresnelPower);
    void setSpecularAmount(float specularAmount);
    void setSpecularRoughness(float specularRoughness);
    void setRoughnessMap(QQuick3DTexture *roughnessMap);
    void setOpacity(float opacity);
    void setOpacityMap(QQuick3DTexture *opacityMap);
    void setBumpMap(QQuick3DTexture *bumpMap);
    void setBumpAmount(float bumpAmount);
    void setNormalMap(QQuick3DTexture *normalMap);

    void setTranslucencyMap(QQuick3DTexture *translucencyMap);
    void setTranslucentFalloff(float translucentFalloff);
    void setDiffuseLightWrap(float diffuseLightWrap);
    void setVertexColors(bool vertexColors);

Q_SIGNALS:
    void lightingChanged(QSSGDefaultMaterialLighting lighting);
    void blendModeChanged(QSSGDefaultMaterialBlendMode blendMode);
    void diffuseColorChanged(QColor diffuseColor);
    void diffuseMapChanged(QQuick3DTexture *diffuseMap);
    void diffuseMap2Changed(QQuick3DTexture *diffuseMap2);
    void diffuseMap3Changed(QQuick3DTexture *diffuseMap3);
    void emissivePowerChanged(float emissivePower);
    void emissiveMapChanged(QQuick3DTexture *emissiveMap);
    void emissiveColorChanged(QColor emissiveColor);
    void specularReflectionMapChanged(QQuick3DTexture *specularReflectionMap);
    void specularMapChanged(QQuick3DTexture *specularMap);
    void specularModelChanged(QSSGDefaultMaterialSpecularModel specularModel);
    void specularTintChanged(QColor specularTint);
    void indexOfRefractionChanged(float indexOfRefraction);
    void fresnelPowerChanged(float fresnelPower);
    void specularAmountChanged(float specularAmount);
    void specularRoughnessChanged(float specularRoughness);
    void roughnessMapChanged(QQuick3DTexture *roughnessMap);
    void opacityChanged(float opacity);
    void opacityMapChanged(QQuick3DTexture *opacityMap);
    void bumpMapChanged(QQuick3DTexture *bumpMap);
    void bumpAmountChanged(float bumpAmount);
    void normalMapChanged(QQuick3DTexture *normalMap);
    void translucencyMapChanged(QQuick3DTexture *translucencyMap);
    void translucentFalloffChanged(float translucentFalloff);
    void diffuseLightWrapChanged(float diffuseLightWrap);
    void vertexColorsChanged(bool vertexColors);

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void itemChange(ItemChange, const ItemChangeData &) override;
private:
    void updateSceneRenderer(QQuick3DSceneManager *window);
    QSSGDefaultMaterialLighting m_lighting = VertexLighting;
    QSSGDefaultMaterialBlendMode m_blendMode = Normal;
    QColor m_diffuseColor;
    QQuick3DTexture *m_diffuseMap = nullptr;
    QQuick3DTexture *m_diffuseMap2 = nullptr;
    QQuick3DTexture *m_diffuseMap3 = nullptr;
    float m_emissivePower = 0;
    QQuick3DTexture *m_emissiveMap = nullptr;

    QColor m_emissiveColor;
    QQuick3DTexture *m_specularReflectionMap = nullptr;
    QQuick3DTexture *m_specularMap = nullptr;
    QSSGDefaultMaterialSpecularModel m_specularModel = Default;
    QColor m_specularTint;
    float m_indexOfRefraction = 0.2f;
    float m_fresnelPower = 0.0f;
    float m_specularAmount = 0.0f;
    float m_specularRoughness = 50.0f;
    QQuick3DTexture *m_roughnessMap = nullptr;
    float m_opacity = 1.0f;
    QQuick3DTexture *m_opacityMap = nullptr;
    QQuick3DTexture *m_bumpMap = nullptr;
    float m_bumpAmount = 0.0f;
    QQuick3DTexture *m_normalMap = nullptr;

    QQuick3DTexture *m_translucencyMap = nullptr;
    float m_translucentFalloff = 0.0f;
    float m_diffuseLightWrap = 0.0f;
    bool m_vertexColors = false;

    quint32 m_dirtyAttributes = 0xffffffff; // all dirty by default
    void markDirty(QSSGDefaultMaterialDirtyType type);


    ConnectionMap m_connections;

};

QT_END_NAMESPACE

#endif // QSSGDEFAULTMATERIAL_H
