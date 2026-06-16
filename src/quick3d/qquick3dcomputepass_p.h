// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QQUICK3DCOMPUTEPASS_P_H
#define QQUICK3DCOMPUTEPASS_P_H

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

#include <QtQuick3D/qtquick3dglobal.h>
#include <QtQuick3D/private/qquick3dobject_p.h>
#include <QtQuick3D/private/qquick3dshaderutils_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3D_EXPORT QQuick3DComputePass : public QQuick3DObject, public QQuick3DPropertyChangedTracker
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> commands READ commands FINAL)
    Q_PROPERTY(QUrl computeShader READ computeShader WRITE setComputeShader NOTIFY computeShaderChanged FINAL)
    QML_NAMED_ELEMENT(ComputePass)
    QML_ADDED_IN_VERSION(6, 13)
public:

    explicit QQuick3DComputePass(QQuick3DObject *parent = nullptr);

    QQmlListProperty<QQuick3DShaderUtilsRenderCommand> commands();

    QUrl computeShader() const;

public Q_SLOTS:
    void setComputeShader(const QUrl &computeShader);

Q_SIGNALS:
    void computeShaderChanged();

private Q_SLOTS:
    void onShaderResourceDirty();

protected:
    void markAllDirty() override { m_dirtyAttributes = AllDirty; }
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void itemChange(QQuick3DObject::ItemChange, const QQuick3DObject::ItemChangeData &) override;
    void markTrackedPropertyDirty(QMetaProperty property, QQuick3DPropertyChangedTracker::DirtyPropertyHint hint) override;

private:
    static void qmlAppendCommand(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list, QQuick3DShaderUtilsRenderCommand *command);
    static QQuick3DShaderUtilsRenderCommand *qmlCommandAt(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list, qsizetype index);
    static qsizetype qmlCommandCount(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list);
    static void qmlCommandClear(QQmlListProperty<QQuick3DShaderUtilsRenderCommand> *list);

    void onCommandChanged();
    void markResourcesDirty();

    enum Dirty {
        ResourceDirty = 0x1,
        PropertyDirty = 0x2,
        CommandsDirty = 0x4,
        ComputeShaderDirty = 0x8,
    };

    using DirtyT = std::underlying_type_t<Dirty>;
    static constexpr DirtyT AllDirty { std::numeric_limits<DirtyT>::max() };

    void markDirty(QQuick3DComputePass::DirtyT type, bool requestSecondaryUpdate = false);
    void clearDirty(QQuick3DComputePass::DirtyT type);

    quint32 m_dirtyAttributes = AllDirty;
    QVector<QQuick3DShaderUtilsRenderCommand *> m_commands;
    QUrl m_computeShader;
    QQuick3DShaderResourceTracker m_shaderResources { this, [this]() { markResourcesDirty(); } };
};

QT_END_NAMESPACE

#endif // QQUICK3DCOMPUTEPASS_P_H
