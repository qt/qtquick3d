// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGSCENEENVIRONMENT_H
#define QSSGSCENEENVIRONMENT_H

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

#include <QtCore/QObject>
#include <QtCore/QVector>

#include <QtGui/QColor>

#include <QtQuick3D/private/qquick3dnode_p.h>

#include <QtQml/QQmlListProperty>

#include <QtQuick3D/private/qquick3deffect_p.h>
#include <QtQuick3D/private/qquick3dlightmapper_p.h>

QT_BEGIN_NAMESPACE

class QQuick3DTexture;
class QQuick3DCubeMapTexture;
class Q_QUICK3D_EXPORT QQuick3DSceneEnvironment : public QQuick3DObject
{
    Q_OBJECT
    Q_PROPERTY(QQuick3DEnvironmentAAModeValues antialiasingMode READ antialiasingMode WRITE setAntialiasingMode NOTIFY antialiasingModeChanged)
    Q_PROPERTY(QQuick3DEnvironmentAAQualityValues antialiasingQuality READ antialiasingQuality WRITE setAntialiasingQuality NOTIFY antialiasingQualityChanged)

    Q_PROPERTY(bool temporalAAEnabled READ temporalAAEnabled WRITE setTemporalAAEnabled NOTIFY temporalAAEnabledChanged)
    Q_PROPERTY(float temporalAAStrength READ temporalAAStrength WRITE setTemporalAAStrength NOTIFY temporalAAStrengthChanged)
    Q_PROPERTY(QQuick3DEnvironmentBackgroundTypes backgroundMode READ backgroundMode WRITE setBackgroundMode NOTIFY backgroundModeChanged)
    Q_PROPERTY(QColor clearColor READ clearColor WRITE setClearColor NOTIFY clearColorChanged)
    Q_PROPERTY(bool depthTestEnabled READ depthTestEnabled WRITE setDepthTestEnabled NOTIFY depthTestEnabledChanged)
    Q_PROPERTY(bool depthPrePassEnabled READ depthPrePassEnabled WRITE setDepthPrePassEnabled NOTIFY depthPrePassEnabledChanged)

    Q_PROPERTY(float aoStrength READ aoStrength WRITE setAoStrength NOTIFY aoStrengthChanged)
    Q_PROPERTY(float aoDistance READ aoDistance WRITE setAoDistance NOTIFY aoDistanceChanged)
    Q_PROPERTY(float aoSoftness READ aoSoftness WRITE setAoSoftness NOTIFY aoSoftnessChanged)
    Q_PROPERTY(bool aoDither READ aoDither WRITE setAoDither NOTIFY aoDitherChanged)
    Q_PROPERTY(int aoSampleRate READ aoSampleRate WRITE setAoSampleRate NOTIFY aoSampleRateChanged)
    Q_PROPERTY(float aoBias READ aoBias WRITE setAoBias NOTIFY aoBiasChanged)

    Q_PROPERTY(QQuick3DTexture *lightProbe READ lightProbe WRITE setLightProbe NOTIFY lightProbeChanged)
    Q_PROPERTY(float probeExposure READ probeExposure WRITE setProbeExposure NOTIFY probeExposureChanged)
    Q_PROPERTY(float probeHorizon READ probeHorizon WRITE setProbeHorizon NOTIFY probeHorizonChanged)
    Q_PROPERTY(QVector3D probeOrientation READ probeOrientation WRITE setProbeOrientation NOTIFY probeOrientationChanged)

    Q_PROPERTY(QQuick3DCubeMapTexture *skyBoxCubeMap READ skyBoxCubeMap WRITE setSkyBoxCubeMap NOTIFY skyBoxCubeMapChanged REVISION(6, 4))

    Q_PROPERTY(QQuick3DEnvironmentTonemapModes tonemapMode READ tonemapMode WRITE setTonemapMode NOTIFY tonemapModeChanged)

    Q_PROPERTY(QQmlListProperty<QQuick3DEffect> effects READ effects)

    Q_PROPERTY(float skyboxBlurAmount READ skyboxBlurAmount WRITE setSkyboxBlurAmount NOTIFY skyboxBlurAmountChanged REVISION(6, 4))
    Q_PROPERTY(bool specularAAEnabled READ specularAAEnabled WRITE setSpecularAAEnabled NOTIFY specularAAEnabledChanged REVISION(6, 4))

    Q_PROPERTY(QQuick3DLightmapper *lightmapper READ lightmapper WRITE setLightmapper NOTIFY lightmapperChanged REVISION(6, 4))

    QML_NAMED_ELEMENT(SceneEnvironment)

public:

    enum QQuick3DEnvironmentAAModeValues {
        NoAA = 0,
        SSAA,
        MSAA,
        ProgressiveAA
    };
    Q_ENUM(QQuick3DEnvironmentAAModeValues)

    enum QQuick3DEnvironmentAAQualityValues {
        Medium = 2,
        High = 4,
        VeryHigh = 8
    };
    Q_ENUM(QQuick3DEnvironmentAAQualityValues)

    enum QQuick3DEnvironmentBackgroundTypes {
        Transparent = 0,
        Unspecified,
        Color,
        SkyBox,
        SkyBoxCubeMap
    };
    Q_ENUM(QQuick3DEnvironmentBackgroundTypes)

    enum QQuick3DEnvironmentTonemapModes {
        TonemapModeNone = 0,
        TonemapModeLinear,
        TonemapModeAces,
        TonemapModeHejlDawson,
        TonemapModeFilmic
    };
    Q_ENUM(QQuick3DEnvironmentTonemapModes)

    explicit QQuick3DSceneEnvironment(QQuick3DObject *parent = nullptr);
    ~QQuick3DSceneEnvironment() override;

    QQuick3DEnvironmentAAModeValues antialiasingMode() const;
    QQuick3DEnvironmentAAQualityValues antialiasingQuality() const;
    bool temporalAAEnabled() const;
    float temporalAAStrength() const;

    QQuick3DEnvironmentBackgroundTypes backgroundMode() const;
    QColor clearColor() const;

    float aoStrength() const;
    float aoDistance() const;
    float aoSoftness() const;
    bool aoDither() const;
    int aoSampleRate() const;
    float aoBias() const;

    QQuick3DTexture *lightProbe() const;
    float probeExposure() const;
    float probeHorizon() const;
    QVector3D probeOrientation() const;

    bool depthTestEnabled() const;
    bool depthPrePassEnabled() const;

    QQuick3DEnvironmentTonemapModes tonemapMode() const;

    QQmlListProperty<QQuick3DEffect> effects();

    Q_REVISION(6, 4) float skyboxBlurAmount() const;
    Q_REVISION(6, 4) bool specularAAEnabled() const;
    Q_REVISION(6, 4) QQuick3DLightmapper *lightmapper() const;
    Q_REVISION(6, 4) QQuick3DCubeMapTexture *skyBoxCubeMap() const;

public Q_SLOTS:
    void setAntialiasingMode(QQuick3DSceneEnvironment::QQuick3DEnvironmentAAModeValues antialiasingMode);
    void setAntialiasingQuality(QQuick3DSceneEnvironment::QQuick3DEnvironmentAAQualityValues antialiasingQuality);
    void setTemporalAAEnabled(bool temporalAAEnabled);
    void setTemporalAAStrength(float strength);

    void setBackgroundMode(QQuick3DSceneEnvironment::QQuick3DEnvironmentBackgroundTypes backgroundMode);
    void setClearColor(const QColor &clearColor);

    void setAoStrength(float aoStrength);
    void setAoDistance(float aoDistance);
    void setAoSoftness(float aoSoftness);
    void setAoDither(bool aoDither);
    void setAoSampleRate(int aoSampleRate);
    void setAoBias(float aoBias);

    void setLightProbe(QQuick3DTexture *lightProbe);
    void setProbeExposure(float probeExposure);
    void setProbeHorizon(float probeHorizon);
    void setProbeOrientation(const QVector3D &orientation);

    void setDepthTestEnabled(bool depthTestEnabled);
    void setDepthPrePassEnabled(bool depthPrePassEnabled);

    void setTonemapMode(QQuick3DSceneEnvironment::QQuick3DEnvironmentTonemapModes tonemapMode);

    Q_REVISION(6, 4) void setSkyboxBlurAmount(float newSkyboxBlurAmount);
    Q_REVISION(6, 4) void setSpecularAAEnabled(bool enabled);
    Q_REVISION(6, 4) void setSkyBoxCubeMap(QQuick3DCubeMapTexture *newSkyBoxCubeMap);

    Q_REVISION(6, 4) void setLightmapper(QQuick3DLightmapper *lightmapper);

Q_SIGNALS:
    void antialiasingModeChanged();
    void antialiasingQualityChanged();
    void temporalAAEnabledChanged();
    void temporalAAStrengthChanged();

    void backgroundModeChanged();
    void clearColorChanged();

    void aoStrengthChanged();
    void aoDistanceChanged();
    void aoSoftnessChanged();
    void aoDitherChanged();
    void aoSampleRateChanged();
    void aoBiasChanged();

    void lightProbeChanged();
    void probeExposureChanged();
    void probeHorizonChanged();
    void probeOrientationChanged();

    void depthTestEnabledChanged();
    void depthPrePassEnabledChanged();

    void tonemapModeChanged();

    Q_REVISION(6, 4) void skyboxBlurAmountChanged();
    Q_REVISION(6, 4) void specularAAEnabledChanged();
    Q_REVISION(6, 4) void lightmapperChanged();
    Q_REVISION(6, 4) void skyBoxCubeMapChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void itemChange(ItemChange, const ItemChangeData &) override;

private:
    friend class QQuick3DSceneRenderer;

    QVector<QQuick3DEffect *> m_effects;

    static void qmlAppendEffect(QQmlListProperty<QQuick3DEffect> *list, QQuick3DEffect *effect);
    static QQuick3DEffect *qmlEffectAt(QQmlListProperty<QQuick3DEffect> *list, qsizetype index);
    static qsizetype qmlEffectsCount(QQmlListProperty<QQuick3DEffect> *list);
    static void qmlClearEffects(QQmlListProperty<QQuick3DEffect> *list);

    void updateSceneManager(QQuick3DSceneManager *manager);

    QQuick3DEnvironmentAAModeValues m_antialiasingMode = NoAA;
    QQuick3DEnvironmentAAQualityValues m_antialiasingQuality = High;
    bool m_temporalAAEnabled = false;
    float m_temporalAAStrength = 0.3f;
    bool m_specularAAEnabled = false;

    QQuick3DEnvironmentBackgroundTypes m_backgroundMode = Transparent;
    QColor m_clearColor = Qt::black;

    float m_aoStrength = 0.0f;
    float m_aoDistance = 5.0f;
    float m_aoSoftness = 50.0f;
    bool m_aoDither = false;
    int m_aoSampleRate = 2;
    float m_aoBias = 0.0f;
    QQuick3DTexture *m_lightProbe = nullptr;
    float m_probeExposure = 1.0f;
    float m_probeHorizon = 0.0f;
    QVector3D m_probeOrientation;

    QHash<QByteArray, QMetaObject::Connection> m_connections;
    bool m_depthTestEnabled = true;
    bool m_depthPrePassEnabled = false;
    QQuick3DEnvironmentTonemapModes m_tonemapMode = QQuick3DEnvironmentTonemapModes::TonemapModeLinear;
    float m_skyboxBlurAmount = 0.0f;
    QQuick3DLightmapper *m_lightmapper = nullptr;
    QMetaObject::Connection m_lightmapperSignalConnection;
    QQuick3DCubeMapTexture *m_skyBoxCubeMap = nullptr;
};

QT_END_NAMESPACE

#endif // QSSGSCENEENVIRONMENT_H
