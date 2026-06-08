// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICK3DSKYMATERIAL_P_H
#define QQUICK3DSKYMATERIAL_P_H

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

#include <QtQuick3D/private/qquick3deffect_p.h>

#include <QVector3D>

QT_BEGIN_NAMESPACE

struct QSSGRenderSkyMaterial;

class Q_QUICK3D_EXPORT QQuick3DSkyMaterial : public QQuick3DObject, public QQuick3DPropertyChangedTracker
{
    Q_OBJECT
    Q_PROPERTY(int radianceMapSize READ radianceMapSize WRITE setRadianceMapSize NOTIFY radianceMapSizeChanged FINAL REVISION(6, 12))
    Q_PROPERTY(QUrl fragmentShader READ fragmentShader WRITE setFragmentShader NOTIFY fragmentShaderChanged FINAL REVISION(6, 12))
    Q_PROPERTY(QString fragmentShaderCode READ fragmentShaderCode WRITE setFragmentShaderCode NOTIFY
                       fragmentShaderCodeChanged FINAL REVISION(6, 12))
    Q_PROPERTY(bool enableIBL READ enableIBL WRITE setEnableIBL NOTIFY enableIBLChanged FINAL REVISION(6, 12))
    Q_PROPERTY(int iblSampleCount READ iblSampleCount WRITE setIblSampleCount NOTIFY iblSampleCountChanged FINAL REVISION(6, 12))
    Q_PROPERTY(int iblRenderFrames READ iblRenderFrames WRITE setIblRenderFrames NOTIFY iblRenderFramesChanged FINAL REVISION(6, 12))

    QML_NAMED_ELEMENT(SkyMaterial)
    QML_ADDED_IN_VERSION(6, 12)
public:
    explicit QQuick3DSkyMaterial(QQuick3DObject *parent = nullptr);

    Q_REVISION(6, 12) int radianceMapSize() const;
    Q_REVISION(6, 12) QUrl fragmentShader() const;
    Q_REVISION(6, 12) bool enableIBL() const;
    Q_REVISION(6, 12) QString fragmentShaderCode() const;
    Q_REVISION(6, 12) int iblSampleCount() const;
    Q_REVISION(6, 12) int iblRenderFrames() const;

public Q_SLOTS:
    Q_REVISION(6, 12) void setRadianceMapSize(int radianceMapSize);
    Q_REVISION(6, 12) void setFragmentShader(const QUrl &newFragmentShader);
    Q_REVISION(6, 12) void setFragmentShaderCode(const QString &newFragmentShaderCode);
    Q_REVISION(6, 12) void setEnableIBL(bool newEnableIBL);
    Q_REVISION(6, 12) void setIblSampleCount(int newIblSampleCount);
    Q_REVISION(6, 12) void setIblRenderFrames(int newIblRenderFrames);

Q_SIGNALS:
    Q_REVISION(6, 12) void radianceMapSizeChanged();
    Q_REVISION(6, 12) void fragmentShaderChanged();
    Q_REVISION(6, 12) void fragmentShaderCodeChanged();
    Q_REVISION(6, 12) void enableIBLChanged();
    Q_REVISION(6, 12) void iblSampleCountChanged();
    Q_REVISION(6, 12) void iblRenderFramesChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void markTrackedPropertyDirty(QMetaProperty property, DirtyPropertyHint hint) override;

private:
    int m_radianceMapSize = 512;
    QUrl m_fragmentShader;

    enum Dirty : quint8 { FragmentShader = 1 << 0, TrackedProperty = 1 << 1 };

    using DirtyT = std::underlying_type_t<Dirty>;

    void markDirty(Dirty v);

    DirtyT m_dirtyFlag { Dirty::FragmentShader | Dirty::TrackedProperty };
    bool m_enableIBL = true;
    int m_iblSampleCount = 32;
    int m_iblRenderFrames = 0;
    QString m_fragmentShaderCode;
};

QT_END_NAMESPACE

#endif // QQUICK3DSKYMATERIAL_P_H
