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

#ifndef QSSGLIGHT_H
#define QSSGLIGHT_H

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

#include <QtQuick3D/private/qquick3dnode_p.h>

#include <QColor>

QT_BEGIN_NAMESPACE

class Q_QUICK3D_EXPORT QQuick3DLight : public QQuick3DNode
{
    Q_OBJECT
    Q_PROPERTY(QSSGRenderLightTypes lightType READ lightType WRITE setLightType NOTIFY lightTypeChanged)
    Q_PROPERTY(QColor diffuseColor READ diffuseColor WRITE setDiffuseColor NOTIFY diffuseColorChanged)
    Q_PROPERTY(QColor specularColor READ specularColor WRITE setSpecularColor NOTIFY specularColorChanged)
    Q_PROPERTY(QColor ambientColor READ ambientColor WRITE setAmbientColor NOTIFY ambientColorChanged)
    Q_PROPERTY(float brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(float linearFade READ linearFade WRITE setLinearFade NOTIFY linearFadeChanged)
    Q_PROPERTY(float exponentialFade READ exponentialFade WRITE setExponentialFade NOTIFY exponentialFadeChanged)
    Q_PROPERTY(float areaWidth READ areaWidth WRITE setAreaWidth NOTIFY areaWidthChanged)
    Q_PROPERTY(float areaHeight READ areaHeight WRITE setAreaHeight NOTIFY areaHeightChanged)
    Q_PROPERTY(bool castShadow READ castShadow WRITE setCastShadow NOTIFY castShadowChanged)
    Q_PROPERTY(float shadowBias READ shadowBias WRITE setShadowBias NOTIFY shadowBiasChanged)
    Q_PROPERTY(float shadowFactor READ shadowFactor WRITE setShadowFactor NOTIFY shadowFactorChanged)
    Q_PROPERTY(int shadowMapResolution READ shadowMapResolution WRITE setShadowMapResolution NOTIFY shadowMapResolutionChanged)
    Q_PROPERTY(float shadowMapFar READ shadowMapFar WRITE setShadowMapFar NOTIFY shadowMapFarChanged)
    Q_PROPERTY(float shadowMapFieldOfView READ shadowMapFieldOfView WRITE setShadowMapFieldOfView NOTIFY shadowMapFieldOfViewChanged)
    Q_PROPERTY(float shadowFilter READ shadowFilter WRITE setShadowFilter NOTIFY shadowFilterChanged)
    Q_PROPERTY(QQuick3DNode *scope READ scope WRITE setScope NOTIFY scopeChanged)

public:
    enum QSSGRenderLightTypes {
        Unknown = 0,
        Directional,
        Point,
        Area,
    };
    Q_ENUM(QSSGRenderLightTypes)

    QQuick3DLight();
    ~QQuick3DLight() override;

    QQuick3DObject::Type type() const override;
    QSSGRenderLightTypes lightType() const;
    QColor diffuseColor() const;
    QColor specularColor() const;
    QColor ambientColor() const;
    float brightness() const;
    float linearFade() const;
    float exponentialFade() const;
    float areaWidth() const;
    float areaHeight() const;
    bool castShadow() const;
    float shadowBias() const;
    float shadowFactor() const;
    int shadowMapResolution() const;
    float shadowMapFar() const;
    float shadowMapFieldOfView() const;
    float shadowFilter() const;
    QQuick3DNode *scope() const;

public Q_SLOTS:
    void setLightType(QSSGRenderLightTypes lightType);
    void setDiffuseColor(QColor diffuseColor);
    void setSpecularColor(QColor specularColor);
    void setAmbientColor(QColor ambientColor);
    void setBrightness(float brightness);
    void setLinearFade(float linearFade);
    void setExponentialFade(float exponentialFade);
    void setAreaWidth(float areaWidth);
    void setAreaHeight(float areaHeight);
    void setCastShadow(bool castShadow);
    void setShadowBias(float shadowBias);
    void setShadowFactor(float shadowFactor);
    void setShadowMapResolution(int shadowMapResolution);
    void setShadowMapFar(float shadowMapFar);
    void setShadowMapFieldOfView(float shadowMapFieldOfView);
    void setShadowFilter(float shadowFilter);
    void setScope(QQuick3DNode * scope);

Q_SIGNALS:
    void lightTypeChanged(QSSGRenderLightTypes lightType);
    void diffuseColorChanged(QColor diffuseColor);
    void specularColorChanged(QColor specularColor);
    void ambientColorChanged(QColor ambientColor);
    void brightnessChanged(float brightness);
    void linearFadeChanged(float linearFade);
    void exponentialFadeChanged(float exponentialFade);
    void areaWidthChanged(float areaWidth);
    void areaHeightChanged(float areaHeight);
    void castShadowChanged(bool castShadow);
    void shadowBiasChanged(float shadowBias);
    void shadowFactorChanged(float shadowFactor);
    void shadowMapResolutionChanged(int shadowMapResolution);
    void shadowMapFarChanged(float shadowMapFar);
    void shadowMapFieldOfViewChanged(float shadowMapFieldOfView);
    void shadowFilterChanged(float shadowFilter);
    void scopeChanged(QQuick3DNode *scope);

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;

private:
    enum class DirtyFlag {
        ShadowDirty = (1 << 0),
        ColorDirty = (1 << 1),
        BrightnessDirty = (1 << 2), // Including fade
        AreaDirty = (1 << 3),
    };
    Q_DECLARE_FLAGS(DirtyFlags, DirtyFlag)

    QSSGRenderLightTypes m_lightType = Directional;
    QColor m_diffuseColor;
    QColor m_specularColor;
    QColor m_ambientColor;
    float m_brightness = 100.0f;
    float m_linearFade = 0.0f;
    float m_exponentialFade = 0.0f;
    float m_areaWidth = 100.0f;
    float m_areaHeight = 100.0f;
    bool m_castShadow = false;
    float m_shadowBias = 0.0f;
    float m_shadowFactor = 5.0f;
    int m_shadowMapResolution = 9;
    float m_shadowMapFar = 5000.0f;
    float m_shadowMapFieldOfView = 90.0f;
    float m_shadowFilter = 35.0f;
    QQuick3DNode *m_scope = nullptr;
    DirtyFlags m_dirtyFlags = DirtyFlags(DirtyFlag::ShadowDirty)
                              | DirtyFlags(DirtyFlag::ColorDirty)
                              | DirtyFlags(DirtyFlag::BrightnessDirty)
                              | DirtyFlags(DirtyFlag::AreaDirty);
};

QT_END_NAMESPACE
#endif // QSSGLIGHT_H
