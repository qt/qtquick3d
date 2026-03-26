// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef QQUICK3DQUADTEXTUREPROVIDER_P_H
#define QQUICK3DQUADTEXTUREPROVIDER_P_H
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

#include <QQuick3DTextureProviderExtension>
#include <QQuick3DTextureData>

#include <QtQuick3D/private/qquick3dshaderutils_p.h>

QT_BEGIN_NAMESPACE

class QSSGQuadTextureProvider;

class QQuick3DQuadTextureProvider : public QQuick3DTextureProviderExtension, public QQuick3DPropertyChangedTracker
{
    Q_OBJECT
    Q_PROPERTY(QUrl fragmentShader READ fragmentShader WRITE setFragmentShader NOTIFY fragmentShaderChanged FINAL REVISION(6, 12))
    Q_PROPERTY(QString fragmentShaderCode READ fragmentShaderCode WRITE setFragmentShaderCode NOTIFY
                       fragmentShaderCodeChanged FINAL REVISION(6, 12))
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged FINAL REVISION(6, 12))
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged FINAL REVISION(6, 12))
    Q_PROPERTY(QQuick3DTextureData::Format format READ format WRITE setFormat NOTIFY formatChanged FINAL REVISION(6, 12))

    QML_NAMED_ELEMENT(QuadTextureProvider)
    QML_ADDED_IN_VERSION(6, 12)
public:
    explicit QQuick3DQuadTextureProvider(QQuick3DObject *parent = nullptr);
    ~QQuick3DQuadTextureProvider() override;

    Q_REVISION(6,12) QUrl fragmentShader() const;
    Q_REVISION(6,12) void setFragmentShader(const QUrl &newFragmentShader);

    Q_REVISION(6,12) QString fragmentShaderCode() const;
    Q_REVISION(6,12) void setFragmentShaderCode(const QString &newFragmentShaderCode);

    Q_REVISION(6,12) int width() const;
    Q_REVISION(6,12) void setWidth(int newWidth);

    Q_REVISION(6,12) int height() const;
    Q_REVISION(6,12) void setHeight(int newHeight);

    Q_REVISION(6,12) QQuick3DTextureData::Format format() const;
    Q_REVISION(6,12) void setFormat(QQuick3DTextureData::Format newFormat);

signals:
    void fragmentShaderChanged();
    void fragmentShaderCodeChanged();
    void widthChanged();
    void heightChanged();
    void formatChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void markTrackedPropertyDirty(QMetaProperty property, DirtyPropertyHint hint) override;

private:
    QUrl m_fragmentShader;
    QString m_fragmentShaderCode;
    int m_width = 128;
    int m_height = 128;
    QQuick3DTextureData::Format m_format = QQuick3DTextureData::Format::RGBA16F;

    enum Dirty : quint8 {
        Dimensions      = 1 << 0,
        TrackedProperty = 1 << 1,
        Format          = 1 << 2,
        FragmentShader  = 1 << 3
    };

    using DirtyT = std::underlying_type_t<Dirty>;
    void markDirty(Dirty v);
    DirtyT m_dirtyFlag { 0xff };

    friend QSSGQuadTextureProvider;
};

QT_END_NAMESPACE

#endif // QQUICK3DQUADTEXTUREPROVIDER_P_H
