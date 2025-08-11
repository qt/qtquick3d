// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DRENDEROUTPUTPROVIDER_P_H
#define QQUICK3DRENDEROUTPUTPROVIDER_P_H

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
#include <QQmlEngine>

QT_BEGIN_NAMESPACE

class QQuick3DRenderPass;

class QQuick3DRenderOutputProvider : public QQuick3DTextureProviderExtension
{
    Q_OBJECT
    Q_PROPERTY(TextureSource textureSource READ textureSource WRITE setTextureSource NOTIFY textureSourceChanged FINAL)
    QML_NAMED_ELEMENT(RenderOutputProvider)
    QML_ADDED_IN_VERSION(6, 11)
public:
    enum class TextureSource : quint32
    {
        None = 0,
        AoTexture = 1,
        DepthTexture,
        ScreenTexture,
    };
    Q_ENUM(TextureSource)

    explicit QQuick3DRenderOutputProvider(QQuick3DObject *parent = nullptr);
    TextureSource textureSource() const;
    void setTextureSource(TextureSource newTextureSource);

signals:
    void textureSourceChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void markAllDirty() override;

private:
    enum DirtyType : quint32 {
        TextureSourceDirty = 0x1,
    };

    using DirtyTypeT = std::underlying_type_t<DirtyType>;
    static constexpr DirtyTypeT AllDirty = 0xffffffff;

    TextureSource m_textureSource = TextureSource::None;
    DirtyTypeT m_dirtyAttributes = AllDirty;
    void markDirty(DirtyType type);
};

QT_END_NAMESPACE

#endif // QQUICK3DRENDEROUTPUTPROVIDER_P_H
