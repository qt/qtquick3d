// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "bufferproviderextension.h"

#include <ssg/qssgrenderextensions.h>
#include <ssg/qssgrenderhelpers.h>
#include <ssg/qssgrendercontextcore.h>
#include <ssg/qssgrhicontext.h>
#include <ssg/qquick3dextensionhelpers.h>
#include <private/qssgrendergraphobject_p.h>

#include <rhi/qrhi.h>

// The backend QSSGRenderExtension node. Content is intentionally never
// uploaded from the CPU here -- the point of this test is that a
// ComputePass can write to (and later read back from) the buffer this
// extension provides, exactly as it would for an ordinary engine-managed
// ShaderStorageBuffer, so any GPU-visible content is entirely up to whichever
// compute shaders reference it via a ShaderStorageBuffer property.
class BufferProviderRenderer : public QSSGRenderExtension
{
    // AutoRegisterExtension is what makes this extension run at all: an
    // extension becomes active either by being listed in View3D.extensions or
    // by carrying this flag, and being an ordinary View3D child is neither. So
    // without it prepareData() never runs, the buffer is never registered, and
    // QSSGBufferManager::loadBuffer() silently hands out an engine-allocated
    // buffer instead -- which the rest of this test cannot tell apart.
    static constexpr FlagT flags = FlagT(Flags::HasGraphicsResources)
            | FlagT(QSSGRenderGraphObjectUtils::InternalFlags::AutoRegisterExtension);

public:
    explicit BufferProviderRenderer(QQuick3DObject *frontend)
        : QSSGRenderExtension(QSSGRenderGraphObject::Type::RenderExtension, flags), m_frontend(frontend) { }
    ~BufferProviderRenderer() override { delete m_buffer; }

    bool prepareData(QSSGFrameData &data) override
    {
        // NOTE: Needs to be resolved on every call; the frontend object
        // backing this extension may have changed.
        const QSSGExtensionId extensionId = m_frontend
                ? QQuick3DExtensionHelpers::getExtensionId(*m_frontend)
                : QSSGExtensionId::Invalid;
        if (QQuick3DExtensionHelpers::isNull(extensionId))
            return false;

        if (!m_buffer) {
            QRhi *rhi = data.contextInterface()->rhiContext()->rhi();
            // One vec4, all that producer.comp/consumer.comp use.
            m_buffer = rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::StorageBuffer, 16);
            m_buffer->setName(QByteArrayLiteral("BufferProviderExtension buffer"));
            if (!m_buffer->create()) {
                delete m_buffer;
                m_buffer = nullptr;
                return false;
            }
        }

        QSSGRenderExtensionHelpers::registerRenderResultBuffer(data, extensionId, registerNull ? nullptr : m_buffer);
        return true;
    }

    void prepareRender(QSSGFrameData &) override { }
    void render(QSSGFrameData &) override { }
    void resetForFrame() override { }
    RenderMode mode() const override { return RenderMode::Standalone; }
    RenderStage stage() const override { return RenderStage::PostColor; }

private:
    QQuick3DObject *m_frontend = nullptr;
    QRhiBuffer *m_buffer = nullptr;
public:
    bool registerNull = false;
};

BufferProviderExtension::BufferProviderExtension(QQuick3DObject *parent)
    : QQuick3DRenderExtension(parent)
{
}

QSSGRenderGraphObject *BufferProviderExtension::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node)
        node = new BufferProviderRenderer(this);
    static_cast<BufferProviderRenderer *>(node)->registerNull = m_registerNull;
    return node;
}
