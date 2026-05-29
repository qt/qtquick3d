// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef VOLUMETRICFOGEXTENSION_H
#define VOLUMETRICFOGEXTENSION_H

#include <QtQuick3D/qquick3drenderextensions.h>
#include <QtQuick3D/private/qquick3dtexture_p.h>
#include <QtGui/QVector3D>
#include <QtQml/QQmlListProperty>
#include <QtCore/QList>
#include "fog3dvolume.h"
#include "ieslightprofileindex.h"

class VolumetricFogExtension : public QQuick3DRenderExtension
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int froxelWidth READ froxelWidth WRITE setFroxelWidth NOTIFY froxelWidthChanged FINAL)
    Q_PROPERTY(int froxelHeight READ froxelHeight WRITE setFroxelHeight NOTIFY froxelHeightChanged FINAL)
    Q_PROPERTY(int froxelDepth READ froxelDepth WRITE setFroxelDepth NOTIFY froxelDepthChanged FINAL)
    Q_PROPERTY(float nearPlane READ nearPlane WRITE setNearPlane NOTIFY nearPlaneChanged FINAL)
    Q_PROPERTY(float farPlane READ farPlane WRITE setFarPlane NOTIFY farPlaneChanged FINAL)

    Q_PROPERTY(QQmlListProperty<Fog3DVolume> fogVolumes READ fogVolumes NOTIFY fogVolumesChanged)
    Q_PROPERTY(QQuick3DTexture* froxelTexture READ froxelTexture CONSTANT)

    Q_PROPERTY(QQuick3DTexture* iesTexture READ iesTexture WRITE setIesTexture NOTIFY iesTextureChanged FINAL)
    Q_PROPERTY(int iesCount READ iesCount WRITE setIesCount NOTIFY iesCountChanged FINAL)
    Q_PROPERTY(QQmlListProperty<IESLightProfileIndex> iesLightProfiles READ iesLightProfiles NOTIFY iesLightProfilesChanged)

public:
    explicit VolumetricFogExtension(QQuick3DObject *parent = nullptr);

    int froxelWidth() const;
    void setFroxelWidth(int v);
    int froxelHeight() const;
    void setFroxelHeight(int v);
    int froxelDepth() const;
    void setFroxelDepth(int v);
    float nearPlane() const;
    void setNearPlane(float v);
    float farPlane() const;
    void setFarPlane(float v);

    QQmlListProperty<Fog3DVolume> fogVolumes();
    QQuick3DTexture *froxelTexture() const;

    QQuick3DTexture *iesTexture() const;
    int iesCount() const;
    QQmlListProperty<IESLightProfileIndex> iesLightProfiles();

    void setIesTexture(QQuick3DTexture *texture);
    void setIesCount(int count);

signals:
    void froxelWidthChanged();
    void froxelHeightChanged();
    void froxelDepthChanged();
    void nearPlaneChanged();
    void farPlaneChanged();

    void fogVolumesChanged();

    void iesTextureChanged();
    void iesCountChanged();
    void iesLightProfilesChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;

private:
    enum Dirty : quint8 {
        Config  = 1 << 0,
        Volumes = 1 << 1,
        IES     = 1 << 2,
    };
    using DirtyT = std::underlying_type_t<Dirty>;
    void markDirty(Dirty v);
    DirtyT m_dirtyFlag{};

    int m_froxelWidth = 160;
    int m_froxelHeight = 90;
    int m_froxelDepth = 64;
    float m_nearPlane = 1.0f;
    float m_farPlane = 2000.0f;

    QList<Fog3DVolume *> m_fogVolumes;

    static void appendFogVolume(QQmlListProperty<Fog3DVolume> *list, Fog3DVolume *v);
    static qsizetype fogVolumeCount(QQmlListProperty<Fog3DVolume> *list);
    static Fog3DVolume *fogVolumeAt(QQmlListProperty<Fog3DVolume> *list, qsizetype i);
    static void clearFogVolumes(QQmlListProperty<Fog3DVolume> *list);

    QQuick3DTexture *m_froxelTexture = nullptr;

    QQuick3DTexture *m_iesQmlTexture = nullptr;
    int m_iesCount = 1;
    QList<IESLightProfileIndex *> m_iesLightProfiles;

    static void appendIESLightProfile(QQmlListProperty<IESLightProfileIndex> *list, IESLightProfileIndex *v);
    static qsizetype iesLightProfileCount(QQmlListProperty<IESLightProfileIndex> *list);
    static IESLightProfileIndex *iesLightProfileAt(QQmlListProperty<IESLightProfileIndex> *list, qsizetype i);
    static void clearIESLightProfiles(QQmlListProperty<IESLightProfileIndex> *list);
};

#endif // VOLUMETRICFOGEXTENSION_H
