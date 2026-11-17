// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "computeresultextension.h"

#include <ssg/qssgrenderextensions.h>
#include <ssg/qssgrenderhelpers.h>
#include <ssg/qssgrendercontextcore.h>
#include <ssg/qssgrhicontext.h>
#include <ssg/qquick3dextensionhelpers.h>
#include <private/qssgrendergraphobject_p.h>

#include <rhi/qrhi.h>

// Written into the pass' probeWrittenBuffer by computeresult_fill.comp, and
// checked against the readback below.
static constexpr float s_expectedBufferValues[4] = { 0.25f, 0.5f, 0.75f, 1.0f };

static const QSize s_smallImageSize(8, 8);
static const QSize s_wideImageSize(32, 4);

class ComputeResultProbe : public QSSGRenderExtension
{
    static constexpr FlagT flags = FlagT(Flags::HasGraphicsResources) | FlagT(QSSGRenderGraphObjectUtils::InternalFlags::AutoRegisterExtension);
public:
    ComputeResultProbe(std::shared_ptr<ComputeResultProbeState> state, QQuick3DObject *frontend)
        : QSSGRenderExtension(QSSGRenderGraphObject::Type::RenderExtension, flags),
            m_state(std::move(state)), m_frontend(frontend)
    {
    }
    ~ComputeResultProbe() override { delete m_buffer; }

    bool prepareData(QSSGFrameData &data) override;
    void prepareRender(QSSGFrameData &data) override;
    void render(QSSGFrameData &) override { }
    void resetForFrame() override { }
    // Main/PostColor, so that in the same View3D as the ComputePass the
    // lookups and the readback come after the pass has been recorded.
    RenderMode mode() const override { return RenderMode::Main; }
    RenderStage stage() const override { return RenderStage::PostColor; }

    QPointer<QQuick3DObject> image;
    QPointer<QQuick3DObject> wideImage;
    QPointer<QQuick3DObject> writtenBuffer;
    QPointer<QQuick3DObject> engineBuffer;
    bool providesWrittenBuffer = false;

private:
    static QSSGResourceId resourceId(const QPointer<QQuick3DObject> &object)
    {
        return object ? QQuick3DExtensionHelpers::getResourceId(*object) : QSSGResourceId::Invalid;
    }

    void issueReadback(QSSGFrameData &data, QRhiBuffer *buffer);

    std::shared_ptr<ComputeResultProbeState> m_state;
    QQuick3DObject *m_frontend = nullptr;
    QRhiBuffer *m_buffer = nullptr;
    bool m_readbackIssued = false;
};

bool ComputeResultProbe::prepareData(QSSGFrameData &data)
{
    // NOTE: Needs to be resolved on every call; the frontend object backing
    // this extension may have changed.
    const QSSGExtensionId extensionId = m_frontend ? QQuick3DExtensionHelpers::getExtensionId(*m_frontend)
                                                   : QSSGExtensionId::Invalid;
    if (QQuick3DExtensionHelpers::isNull(extensionId))
        return false;

    if (!providesWrittenBuffer)
        return true;

    // Provide the QRhiBuffer backing writtenBuffer, so that the handle
    // getShaderStorageBuffer() returns for it can be compared for identity
    // instead of merely tested for non-null.
    if (!m_buffer) {
        QRhi *rhi = data.contextInterface()->rhiContext()->rhi();
        // One vec4, all the probe uses.
        m_buffer = rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::StorageBuffer, 16);
        m_buffer->setName(QByteArrayLiteral("ComputeResultExtension probe buffer"));
        if (!m_buffer->create()) {
            delete m_buffer;
            m_buffer = nullptr;
            return false;
        }
    }
    QSSGRenderExtensionHelpers::registerRenderResultBuffer(data, extensionId, m_buffer);

    return true;
}

void ComputeResultProbe::prepareRender(QSSGFrameData &data)
{
    int checks = ComputeResultExtension::ProbeRan;

    QRhiTexture *smallTex = data.getShaderImageTexture(resourceId(image));
    QRhiTexture *wideTex = data.getShaderImageTexture(resourceId(wideImage));
    QRhiBuffer *writtenBuf = data.getShaderStorageBuffer(resourceId(writtenBuffer));
    QRhiBuffer *engineBuf = data.getShaderStorageBuffer(resourceId(engineBuffer));

    if (smallTex) {
        checks |= ComputeResultExtension::ImageResolved;
        if (smallTex->pixelSize() == s_smallImageSize)
            checks |= ComputeResultExtension::ImageSizeMatches;
        if (smallTex->format() == QRhiTexture::RGBA8)
            checks |= ComputeResultExtension::ImageFormatMatches;
    }

    if (wideTex) {
        checks |= ComputeResultExtension::SecondImageResolved;
        // Two ShaderImages of different size and format, so an id that
        // resolved to the wrong one would be caught here.
        if (wideTex != smallTex && wideTex->pixelSize() == s_wideImageSize
            && wideTex->format() == QRhiTexture::R32UI) {
            checks |= ComputeResultExtension::ImagesAreDistinct;
        }
    }

    if (writtenBuf) {
        checks |= ComputeResultExtension::WrittenBufferResolved;
        if (m_buffer && writtenBuf == m_buffer)
            checks |= ComputeResultExtension::ProvidedBufferIsOurs;
    }

    if (engineBuf) {
        checks |= ComputeResultExtension::EngineBufferResolved;
        if (engineBuf != writtenBuf)
            checks |= ComputeResultExtension::BuffersAreDistinct;
    }

    if (!data.getShaderImageTexture(QSSGResourceId::Invalid) && !data.getShaderStorageBuffer(QSSGResourceId::Invalid))
        checks |= ComputeResultExtension::InvalidIdIsEmpty;

    if (data.contextInterface()->rhiContext()->rhi()->isFeatureSupported(QRhi::ReadBackNonUniformBuffer))
        checks |= ComputeResultExtension::ReadbackSupported;

    m_state->addChecks(checks);

    if (writtenBuf)
        issueReadback(data, writtenBuf);
}

void ComputeResultProbe::issueReadback(QSSGFrameData &data, QRhiBuffer *buffer)
{
    if (m_readbackIssued)
        return;

    const auto &rhiCtx = data.contextInterface()->rhiContext();
    QRhi *rhi = rhiCtx->rhi();
    // The buffer is a StorageBuffer, not a UniformBuffer, so readback needs this.
    if (!rhi->isFeatureSupported(QRhi::ReadBackNonUniformBuffer))
        return;

    m_readbackIssued = true;

    // Both the state and the result are held by the completion callback, so a
    // result arriving after this extension has been torn down still lands
    // somewhere valid. That keeps the (small) state alive for the rest of the
    // process once a readback has been issued, which is fine for a test.
    auto state = m_state;
    auto result = std::make_shared<QRhiReadbackResult>();
    result->completed = [state, result]() {
        int checks = ComputeResultExtension::ReadbackCompleted;
        if (size_t(result->data.size()) >= sizeof(s_expectedBufferValues)) {
            const float *values = reinterpret_cast<const float *>(result->data.constData());
            if (qFuzzyCompare(values[0], s_expectedBufferValues[0])
                && qFuzzyCompare(values[1], s_expectedBufferValues[1])
                && qFuzzyCompare(values[2], s_expectedBufferValues[2])
                && qFuzzyCompare(values[3], s_expectedBufferValues[3])) {
                checks |= ComputeResultExtension::ReadbackContentOk;
            }
        }
        state->addChecks(checks);
    };

    // Legal here: prepareRender() for the Main/PostColor stage runs in the
    // prepare phase, after the compute pass' dispatch has been recorded (user
    // passes are Type::Standalone, so they render inline in the prepare loop)
    // but before the main render pass begins, so resource updates can still be
    // enqueued and the readback observes this frame's compute output.
    QRhiResourceUpdateBatch *rub = rhi->nextResourceUpdateBatch();
    rub->readBackBuffer(buffer, 0, int(sizeof(s_expectedBufferValues)), result.get());
    rhiCtx->commandBuffer()->resourceUpdate(rub);
}

ComputeResultExtension::ComputeResultExtension(QQuick3DObject *parent) : QQuick3DRenderExtension(parent) { }

void ComputeResultExtension::setImage(QQuick3DObject *image)
{
    if (m_image == image)
        return;
    m_image = image;
    update();
    Q_EMIT imageChanged();
}

void ComputeResultExtension::setWideImage(QQuick3DObject *image)
{
    if (m_wideImage == image)
        return;
    m_wideImage = image;
    update();
    Q_EMIT wideImageChanged();
}

void ComputeResultExtension::setWrittenBuffer(QQuick3DObject *buffer)
{
    if (m_writtenBuffer == buffer)
        return;
    m_writtenBuffer = buffer;
    update();
    Q_EMIT writtenBufferChanged();
}

void ComputeResultExtension::setEngineBuffer(QQuick3DObject *buffer)
{
    if (m_engineBuffer == buffer)
        return;
    m_engineBuffer = buffer;
    update();
    Q_EMIT engineBufferChanged();
}

void ComputeResultExtension::setProvidesWrittenBuffer(bool provides)
{
    if (m_providesWrittenBuffer == provides)
        return;
    m_providesWrittenBuffer = provides;
    update();
    Q_EMIT providesWrittenBufferChanged();
}

QSSGRenderGraphObject *ComputeResultExtension::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node)
        node = new ComputeResultProbe(m_state, this);

    auto *probe = static_cast<ComputeResultProbe *>(node);
    probe->image = m_image;
    probe->wideImage = m_wideImage;
    probe->writtenBuffer = m_writtenBuffer;
    probe->engineBuffer = m_engineBuffer;
    probe->providesWrittenBuffer = m_providesWrittenBuffer;

    return node;
}

QByteArray ComputeResultExtension::describeMissingChecks(int passed, int expected)
{
    static const struct {
        Check bit;
        const char *name;
    } names[] = {
        { ProbeRan, "the probe extension never ran" },
        { ImageResolved, "getShaderImageTexture() did not resolve image" },
        { ImageSizeMatches, "image has the wrong pixel size" },
        { ImageFormatMatches, "image has the wrong format" },
        { SecondImageResolved, "getShaderImageTexture() did not resolve wideImage" },
        { ImagesAreDistinct, "wideImage did not resolve to its own 32x4 R32UI texture" },
        { WrittenBufferResolved, "getShaderStorageBuffer() did not resolve writtenBuffer" },
        { ProvidedBufferIsOurs, "writtenBuffer is not the QRhiBuffer this extension registered" },
        { EngineBufferResolved, "getShaderStorageBuffer() did not resolve engineBuffer" },
        { BuffersAreDistinct, "engineBuffer and writtenBuffer resolved to the same buffer" },
        { InvalidIdIsEmpty, "an invalid id returned a texture or buffer" },
        { ReadbackCompleted, "the writtenBuffer readback never completed" },
        { ReadbackContentOk, "the writtenBuffer readback did not contain the written values" },
    };

    const int missing = expected & ~passed;
    if (!missing)
        return QByteArrayLiteral("no checks missing");

    QByteArrayList failures;
    for (const auto &entry : names) {
        if (missing & entry.bit)
            failures.append(entry.name);
    }
    return QByteArrayLiteral("getShaderImageTexture()/getShaderStorageBuffer() checks failed: ") + failures.join(QByteArrayLiteral("; "));
}
