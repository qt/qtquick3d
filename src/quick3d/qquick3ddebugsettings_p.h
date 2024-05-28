// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DDEBUGSETTINGS_H
#define QQUICK3DDEBUGSETTINGS_H

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

#include <QtQuick3D/private/qquick3dobject_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3D_EXPORT QQuick3DDebugSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuick3DMaterialOverrides materialOverride READ materialOverride WRITE setMaterialOverride NOTIFY materialOverrideChanged)
    Q_PROPERTY(bool wireframeEnabled READ wireframeEnabled WRITE setWireframeEnabled NOTIFY wireframeEnabledChanged)
    Q_PROPERTY(bool drawDirectionalLightShadowBoxes READ drawDirectionalLightShadowBoxes WRITE
                       setDrawDirectionalLightShadowBoxes NOTIFY drawDirectionalLightShadowBoxesChanged FINAL)
    Q_PROPERTY(bool drawShadowCastingBounds READ drawShadowCastingBounds WRITE setDrawShadowCastingBounds NOTIFY drawShadowCastingBoundsChanged FINAL)
    Q_PROPERTY(bool drawShadowReceivingBounds READ drawShadowReceivingBounds WRITE setDrawShadowReceivingBounds NOTIFY drawShadowReceivingBoundsChanged FINAL)
    Q_PROPERTY(bool drawCascades READ drawCascades WRITE setDrawCascades NOTIFY drawCascadesChanged FINAL)
    Q_PROPERTY(bool drawSceneCascadeIntersection READ drawSceneCascadeIntersection WRITE setDrawSceneCascadeIntersection NOTIFY drawSceneCascadeIntersectionChanged FINAL)
    Q_PROPERTY(bool disableShadowCameraUpdate READ disableShadowCameraUpdate WRITE setDisableShadowCameraUpdate NOTIFY disableShadowCameraUpdateChanged FINAL)

    QML_NAMED_ELEMENT(DebugSettings)
public:
    enum QQuick3DMaterialOverrides {
        None,
        BaseColor,
        Roughness,
        Metalness,
        Diffuse,
        Specular,
        ShadowOcclusion,
        Emission,
        AmbientOcclusion,
        Normals,
        Tangents,
        Binormals,
        F0
    };
    Q_ENUM(QQuick3DMaterialOverrides)

    explicit QQuick3DDebugSettings(QObject *parent = nullptr);

    QQuick3DMaterialOverrides materialOverride() const;
    void setMaterialOverride(QQuick3DMaterialOverrides newMaterialOverride);

    bool wireframeEnabled() const;
    void setWireframeEnabled(bool newWireframeEnabled);

    bool drawDirectionalLightShadowBoxes() const;
    void setDrawDirectionalLightShadowBoxes(bool newDrawDirectionalLightShadowBoxes);

    bool drawShadowCastingBounds() const;
    void setDrawShadowCastingBounds(bool newDrawShadowCastingBounds);

    bool drawShadowReceivingBounds() const;
    void setDrawShadowReceivingBounds(bool newDrawShadowReceivingBounds);

    bool drawCascades() const;
    void setDrawCascades(bool newDrawCascades);

    bool drawSceneCascadeIntersection() const;
    void setDrawSceneCascadeIntersection(bool newDrawSceneCascadeIntersection);

    bool disableShadowCameraUpdate() const;
    void setDisableShadowCameraUpdate(bool newDisableShadowCameraUpdate);

Q_SIGNALS:
    void materialOverrideChanged();
    void wireframeEnabledChanged();
    void drawDirectionalLightShadowBoxesChanged();
    void drawShadowCastingBoundsChanged();
    void drawShadowReceivingBoundsChanged();
    void drawCascadesChanged();
    void drawSceneCascadeIntersectionChanged();
    void disableShadowCameraUpdateChanged();
    void changed();

private:
    void update();
    QQuick3DMaterialOverrides m_materialOverride = None;

    bool m_wireframeEnabled = false;
    bool m_drawDirectionalLightShadowBoxes = false;
    bool m_drawShadowCastingBounds = false;
    bool m_drawShadowReceivingBounds = false;
    bool m_drawCascades = false;
    bool m_drawSceneCascadeIntersection = false;
    bool m_disableShadowCameraUpdate = false;
};

QT_END_NAMESPACE

#endif // QQUICK3DDEBUGSETTINGS_H
