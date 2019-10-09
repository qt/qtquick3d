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

#ifndef QSSGAREALIGHT_H
#define QSSGAREALIGHT_H

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

class Q_QUICK3D_EXPORT QQuick3DAreaLight : public QQuick3DNode
{
    Q_OBJECT
    Q_PROPERTY(QColor diffuseColor READ diffuseColor WRITE setDiffuseColor NOTIFY diffuseColorChanged)
    Q_PROPERTY(QColor specularColor READ specularColor WRITE setSpecularColor NOTIFY specularColorChanged)
    Q_PROPERTY(QColor ambientColor READ ambientColor WRITE setAmbientColor NOTIFY ambientColorChanged)
    Q_PROPERTY(float brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(float width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(float height READ height WRITE setHeight NOTIFY heightChanged)
    Q_PROPERTY(bool castShadow READ castShadow WRITE setCastShadow NOTIFY castShadowChanged)
    Q_PROPERTY(float shadowBias READ shadowBias WRITE setShadowBias NOTIFY shadowBiasChanged)
    Q_PROPERTY(float shadowFactor READ shadowFactor WRITE setShadowFactor NOTIFY shadowFactorChanged)
    Q_PROPERTY(int shadowMapResolution READ shadowMapResolution WRITE setShadowMapResolution NOTIFY shadowMapResolutionChanged)
    Q_PROPERTY(float shadowMapFar READ shadowMapFar WRITE setShadowMapFar NOTIFY shadowMapFarChanged)
    Q_PROPERTY(float shadowMapFieldOfView READ shadowMapFieldOfView WRITE setShadowMapFieldOfView NOTIFY shadowMapFieldOfViewChanged)
    Q_PROPERTY(float shadowFilter READ shadowFilter WRITE setShadowFilter NOTIFY shadowFilterChanged)
    Q_PROPERTY(QQuick3DNode *scope READ scope WRITE setScope NOTIFY scopeChanged)

public:
    QQuick3DAreaLight() : m_diffuseColor(Qt::white), m_specularColor(Qt::white), m_ambientColor(Qt::black) {}
    ~QQuick3DAreaLight() override {}

    QQuick3DObject::Type type() const override;
    QColor diffuseColor() const;
    QColor specularColor() const;
    QColor ambientColor() const;
    float brightness() const;
    float width() const;
    float height() const;
    bool castShadow() const;
    float shadowBias() const;
    float shadowFactor() const;
    int shadowMapResolution() const;
    float shadowMapFar() const;
    float shadowMapFieldOfView() const;
    float shadowFilter() const;
    QQuick3DNode *scope() const;

public Q_SLOTS:
    void setDiffuseColor(QColor diffuseColor);
    void setSpecularColor(QColor specularColor);
    void setAmbientColor(QColor ambientColor);
    void setBrightness(float brightness);
    void setWidth(float width);
    void setHeight(float height);
    void setCastShadow(bool castShadow);
    void setShadowBias(float shadowBias);
    void setShadowFactor(float shadowFactor);
    void setShadowMapResolution(int shadowMapResolution);
    void setShadowMapFar(float shadowMapFar);
    void setShadowMapFieldOfView(float shadowMapFieldOfView);
    void setShadowFilter(float shadowFilter);
    void setScope(QQuick3DNode * scope);

Q_SIGNALS:
    void diffuseColorChanged(QColor diffuseColor);
    void specularColorChanged(QColor specularColor);
    void ambientColorChanged(QColor ambientColor);
    void brightnessChanged(float brightness);
    void widthChanged(float width);
    void heightChanged(float height);
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
        BrightnessDirty = (1 << 2),
        AreaDirty = (1 << 3),
    };
    Q_DECLARE_FLAGS(DirtyFlags, DirtyFlag)

    QColor m_diffuseColor;
    QColor m_specularColor;
    QColor m_ambientColor;
    float m_brightness = 100.0f;
    float m_width = 100.0f;
    float m_height = 100.0f;
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
#endif // QSSGAREALIGHT_H
