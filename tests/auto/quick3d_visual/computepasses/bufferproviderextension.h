// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BUFFERPROVIDEREXTENSION_H
#define BUFFERPROVIDEREXTENSION_H

#include <QQuick3DRenderExtension>
#include <QtQmlIntegration>

// A minimal QQuick3DRenderExtension whose backend QSSGRenderExtension
// creates its own QRhiBuffer and registers it via
// QSSGRenderExtensionHelpers::registerRenderResultBuffer(), to exercise
// ShaderStorageBuffer::bufferProvider. The only in-tree user of this
// mechanism (BufferOutputProvider in examples/quick3d/clusteredlighting)
// is never actually wired up to a bufferProvider property, so this is
// genuinely new, real coverage rather than a duplicate of an existing
// example.
class BufferProviderExtension : public QQuick3DRenderExtension
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool registerNull MEMBER m_registerNull)

public:
    explicit BufferProviderExtension(QQuick3DObject *parent = nullptr);

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;

private:
    bool m_registerNull = false;
};

#endif // BUFFERPROVIDEREXTENSION_H
