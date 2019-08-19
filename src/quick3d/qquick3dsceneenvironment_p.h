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
#include <QtQuick3D/private/qquick3deffect_p.h>

#include <QtQml/QQmlListProperty>

QT_BEGIN_NAMESPACE

class QQuick3DTexture;
class Q_QUICK3D_EXPORT QQuick3DSceneEnvironment : public QQuick3DObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QQuick3DEffect> effects READ effectsList)
    Q_PROPERTY(QQuick3DEnvironmentAAModeValues progressiveAAMode READ progressiveAAMode WRITE setProgressiveAAMode NOTIFY progressiveAAModeChanged)
    Q_PROPERTY(QQuick3DEnvironmentAAModeValues multisampleAAMode READ multisampleAAMode WRITE setMultisampleAAMode NOTIFY multisampleAAModeChanged)
    Q_PROPERTY(bool temporalAAEnabled READ temporalAAEnabled WRITE setTemporalAAEnabled NOTIFY temporalAAEnabledChanged)
    Q_PROPERTY(QQuick3DEnvironmentBackgroundTypes backgroundMode READ backgroundMode WRITE setBackgroundMode NOTIFY backgroundModeChanged)
    Q_PROPERTY(QColor clearColor READ clearColor WRITE setClearColor NOTIFY clearColorChanged)
    Q_PROPERTY(bool isDepthTestDisabled READ isDepthTestDisabled WRITE setIsDepthTestDisabled NOTIFY isDepthTestDisabledChanged)
    Q_PROPERTY(bool isDepthPrePassDisabled READ isDepthPrePassDisabled WRITE setIsDepthPrePassDisabled NOTIFY isDepthPrePassDisabledChanged)

    Q_PROPERTY(float aoStrength READ aoStrength WRITE setAoStrength NOTIFY aoStrengthChanged)
    Q_PROPERTY(float aoDistance READ aoDistance WRITE setAoDistance NOTIFY aoDistanceChanged)
    Q_PROPERTY(float aoSoftness READ aoSoftness WRITE setAoSoftness NOTIFY aoSoftnessChanged)
    Q_PROPERTY(bool aoDither READ aoDither WRITE setAoDither NOTIFY aoDitherChanged)
    Q_PROPERTY(int aoSampleRate READ aoSampleRate WRITE setAoSampleRate NOTIFY aoSampleRateChanged)
    Q_PROPERTY(float aoBias READ aoBias WRITE setAoBias NOTIFY aoBiasChanged)

    Q_PROPERTY(float shadowStrength READ shadowStrength WRITE setShadowStrength NOTIFY shadowStrengthChanged)
    Q_PROPERTY(float shadowDistance READ shadowDistance WRITE setShadowDistance NOTIFY shadowDistanceChanged)
    Q_PROPERTY(float shadowSoftness READ shadowSoftness WRITE setShadowSoftness NOTIFY shadowSoftnessChanged)
    Q_PROPERTY(float shadowBias READ shadowBias WRITE setShadowBias NOTIFY shadowBiasChanged)

    Q_PROPERTY(QQuick3DTexture *lightProbe READ lightProbe WRITE setLightProbe NOTIFY lightProbeChanged)
    Q_PROPERTY(float probeBrightness READ probeBrightness WRITE setProbeBrightness NOTIFY probeBrightnessChanged)
    Q_PROPERTY(bool fastIBL READ fastIBL WRITE setFastIBL NOTIFY fastIBLChanged)
    Q_PROPERTY(float probeHorizon READ probeHorizon WRITE setProbeHorizon NOTIFY probeHorizonChanged)
    Q_PROPERTY(float probeFieldOfView READ probeFieldOfView WRITE setProbeFieldOfView NOTIFY probeFieldOfViewChanged)

    Q_PROPERTY(QQuick3DTexture *lightProbe2 READ lightProbe2 WRITE setLightProbe2 NOTIFY lightProbe2Changed)
    Q_PROPERTY(float probe2Fade READ probe2Fade WRITE setProbe2Fade NOTIFY probe2FadeChanged)
    Q_PROPERTY(float probe2Window READ probe2Window WRITE setProbe2Window NOTIFY probe2WindowChanged)
    Q_PROPERTY(float probe2Postion READ probe2Postion WRITE setProbe2Postion NOTIFY probe2PostionChanged)

public:
    enum QQuick3DEnvironmentAAModeValues {
        NoAA = 0,
        SSAA = 1,
        X2 = 2,
        X4 = 4,
        X8 = 8
    };
    Q_ENUM(QQuick3DEnvironmentAAModeValues)
    enum QQuick3DEnvironmentBackgroundTypes {
        Transparent = 0,
        Unspecified,
        Color,
        SkyBox
    };
    Q_ENUM(QQuick3DEnvironmentBackgroundTypes)

    explicit QQuick3DSceneEnvironment(QQuick3DObject *parent = nullptr);
    ~QQuick3DSceneEnvironment() override;

    QQuick3DEnvironmentAAModeValues progressiveAAMode() const;
    QQuick3DEnvironmentAAModeValues multisampleAAMode() const;
    bool temporalAAEnabled() const;

    QQuick3DEnvironmentBackgroundTypes backgroundMode() const;
    QColor clearColor() const;

    float aoStrength() const;
    float aoDistance() const;
    float aoSoftness() const;
    bool aoDither() const;
    int aoSampleRate() const;
    float aoBias() const;

    float shadowStrength() const;
    float shadowDistance() const;
    float shadowSoftness() const;
    float shadowBias() const;

    QQuick3DTexture *lightProbe() const;
    float probeBrightness() const;
    bool fastIBL() const;
    float probeHorizon() const;
    float probeFieldOfView() const;

    QQuick3DTexture *lightProbe2() const;
    float probe2Fade() const;
    float probe2Window() const;
    float probe2Postion() const;

    QQmlListProperty<QQuick3DEffect> effectsList();

    bool isDepthTestDisabled() const;
    bool isDepthPrePassDisabled() const;

    QQuick3DObject::Type type() const override;

public Q_SLOTS:
    void setProgressiveAAMode(QQuick3DEnvironmentAAModeValues progressiveAAMode);
    void setMultisampleAAMode(QQuick3DEnvironmentAAModeValues multisampleAAMode);
    void setTemporalAAEnabled(bool temporalAAEnabled);

    void setBackgroundMode(QQuick3DEnvironmentBackgroundTypes backgroundMode);
    void setClearColor(QColor clearColor);

    void setAoStrength(float aoStrength);
    void setAoDistance(float aoDistance);
    void setAoSoftness(float aoSoftness);
    void setAoDither(bool aoDither);
    void setAoSampleRate(int aoSampleRate);
    void setAoBias(float aoBias);

    void setShadowStrength(float shadowStrength);
    void setShadowDistance(float shadowDistance);
    void setShadowSoftness(float shadowSoftness);
    void setShadowBias(float shadowBias);

    void setLightProbe(QQuick3DTexture *lightProbe);
    void setProbeBrightness(float probeBrightness);
    void setFastIBL(bool fastIBL);
    void setProbeHorizon(float probeHorizon);
    void setProbeFieldOfView(float probeFieldOfView);

    void setLightProbe2(QQuick3DTexture *lightProbe2);
    void setProbe2Fade(float probe2Fade);
    void setProbe2Window(float probe2Window);
    void setProbe2Postion(float probe2Postion);

    void setIsDepthTestDisabled(bool isDepthTestDisabled);
    void setIsDepthPrePassDisabled(bool isDepthPrePassDisabled);

Q_SIGNALS:
    void progressiveAAModeChanged(QQuick3DEnvironmentAAModeValues progressiveAAMode);
    void multisampleAAModeChanged(QQuick3DEnvironmentAAModeValues multisampleAAMode);
    void temporalAAEnabledChanged(bool temporalAAEnabled);

    void backgroundModeChanged(QQuick3DEnvironmentBackgroundTypes backgroundMode);
    void clearColorChanged(QColor clearColor);

    void aoStrengthChanged(float aoStrength);
    void aoDistanceChanged(float aoDistance);
    void aoSoftnessChanged(float aoSoftness);
    void aoDitherChanged(bool aoDither);
    void aoSampleRateChanged(int aoSampleRate);
    void aoBiasChanged(float aoBias);

    void shadowStrengthChanged(float shadowStrength);
    void shadowDistanceChanged(float shadowDistance);
    void shadowSoftnessChanged(float shadowSoftness);
    void shadowBiasChanged(float shadowBias);

    void lightProbeChanged(QQuick3DTexture *lightProbe);
    void probeBrightnessChanged(float probeBrightness);
    void fastIBLChanged(bool fastIBL);
    void probeHorizonChanged(float probeHorizon);
    void probeFieldOfViewChanged(float probeFieldOfView);

    void lightProbe2Changed(QQuick3DTexture *lightProbe2);
    void probe2FadeChanged(float probe2Fade);
    void probe2WindowChanged(float probe2Window);
    void probe2PostionChanged(float probe2Postion);

    void isDepthTestDisabledChanged(bool isDepthTestDisabled);
    void isDepthPrePassDisabledChanged(bool isDepthPrePassDisabled);

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void itemChange(ItemChange, const ItemChangeData &) override;

private:
    void updateSceneManager(QQuick3DSceneManager *manager);

    QQuick3DEnvironmentAAModeValues m_progressiveAAMode = NoAA;
    QQuick3DEnvironmentAAModeValues m_multisampleAAMode = NoAA;
    bool m_temporalAAEnabled = false;

    QQuick3DEnvironmentBackgroundTypes m_backgroundMode = Transparent;
    QColor m_clearColor = Qt::black;

    float m_aoStrength = 0.0f;
    float m_aoDistance = 5.0f;
    float m_aoSoftness = 50.0f;
    bool m_aoDither = false;
    int m_aoSampleRate = 2;
    float m_aoBias = 0.0f;
    float m_shadowStrength = 0.0f;
    float m_shadowDistance = 10.0f;
    float m_shadowSoftness = 100.0f;
    float m_shadowBias = 0.0f;
    QQuick3DTexture *m_lightProbe = nullptr;
    float m_probeBrightness = 100.0f;
    bool m_fastIBL = false;
    float m_probeHorizon = -1.0f;
    float m_probeFieldOfView = 180.0f;
    QQuick3DTexture *m_lightProbe2 = nullptr;
    float m_probe2Fade = 1.0f;
    float m_probe2Window = 1.0f;
    float m_probe2Postion = 0.5f;

    QVector<QQuick3DEffect *> m_effects;

    static void qmlAppendEffect(QQmlListProperty<QQuick3DEffect> *list, QQuick3DEffect *effect);
    static QQuick3DEffect *qmlEffectAt(QQmlListProperty<QQuick3DEffect> *list, int index);
    static int qmlEffectsCount(QQmlListProperty<QQuick3DEffect> *list);
    static void qmlClearEffects(QQmlListProperty<QQuick3DEffect> *list);

    QHash<QObject*, QMetaObject::Connection> m_connections;
    bool m_isDepthTestDisabled = false;
    bool m_isDepthPrePassDisabled = true;
};

QT_END_NAMESPACE

#endif // QSSGSCENEENVIRONMENT_H
