// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QQUICK3DSHADERSTORAGEBUFFER_P_H
#define QQUICK3DSHADERSTORAGEBUFFER_P_H

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

#include <QtQuick3D/qquick3dobject.h>
#include <QtQuick3D/qquick3dshaderstoragebufferdata.h>

QT_BEGIN_NAMESPACE

class QQuick3DRenderExtension;
class Q_QUICK3D_EXPORT QQuick3DShaderStorageBuffer : public QQuick3DObject
{
    Q_OBJECT
    Q_PROPERTY(quint32 size READ size WRITE setSize NOTIFY sizeChanged FINAL)
    Q_PROPERTY(QQuick3DRenderExtension *bufferProvider READ bufferProvider WRITE setBufferProvider NOTIFY bufferProviderChanged FINAL)
    Q_PROPERTY(QQuick3DShaderStorageBufferData *bufferData READ bufferData WRITE setBufferData NOTIFY bufferDataChanged FINAL)
    QML_NAMED_ELEMENT(ShaderStorageBuffer)
    QML_ADDED_IN_VERSION(6, 13)

public:
    explicit QQuick3DShaderStorageBuffer(QQuick3DObject *parent = nullptr);
    ~QQuick3DShaderStorageBuffer() override;

    quint32 size() const;
    QQuick3DRenderExtension *bufferProvider() const;
    QQuick3DShaderStorageBufferData *bufferData() const;

public Q_SLOTS:
    void setSize(quint32 size);
    void setBufferProvider(QQuick3DRenderExtension *provider);
    void setBufferData(QQuick3DShaderStorageBufferData *data);

Q_SIGNALS:
    void sizeChanged();
    void bufferProviderChanged();
    void bufferDataChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void markAllDirty() override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;

private:
    void setSizePriv(quint32 size);
    enum class DirtyFlag {
        SizeDirty = 1,
        ExtensionDirty = 1 << 1,
        DataDirty = 1 << 2
    };
    Q_DECLARE_FLAGS(DirtyFlags, DirtyFlag)
    DirtyFlags m_dirtyFlags = DirtyFlags(DirtyFlag::DataDirty) | DirtyFlags(DirtyFlag::SizeDirty) | DirtyFlags(DirtyFlag::ExtensionDirty);
    quint32 m_size = 0;
    QQuick3DShaderStorageBufferData *m_data = nullptr;
    QQuick3DRenderExtension *m_provider = nullptr;
    QMetaObject::Connection m_modifiedConnection;
    QMetaObject::Connection m_destroyedConnection;
};

QT_END_NAMESPACE

#endif // QQUICK3DSHADERSTORAGEBUFFER_P_H
