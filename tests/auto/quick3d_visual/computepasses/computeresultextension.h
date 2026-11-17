// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef COMPUTERESULTEXTENSION_H
#define COMPUTERESULTEXTENSION_H

#include <QQuick3DRenderExtension>
#include <QtCore/QAtomicInt>
#include <QtCore/QPointer>
#include <QtQmlIntegration>

#include <memory>

// Shared between the frontend object, its backend QSSGRenderExtension node and
// the readback completion callback, so that a result arriving after any of them
// has been destroyed still has somewhere valid to land.
struct ComputeResultProbeState
{
    void addChecks(int bits) { passedChecks.fetchAndOrOrdered(bits); }
    int checks() const { return passedChecks.loadAcquire(); }

    QAtomicInt passedChecks;
};

// A QQuick3DRenderExtension that looks up the GPU resources behind a set of
// ShaderImage and ShaderStorageBuffer objects through
// QSSGFrameData::getShaderImageTexture() and getShaderStorageBuffer(), which
// is how a C++ RenderExtension gets at what a ComputePass wrote.
//
// The extension is deliberately Main/PostColor: when it is in the same View3D
// as the ComputePass, that is the only extension stage ordered after the QML
// user passes, so the buffer readback observes this frame's compute output.
class ComputeResultExtension : public QQuick3DRenderExtension
{
    Q_OBJECT
    Q_PROPERTY(QQuick3DObject *image READ image WRITE setImage NOTIFY imageChanged)
    Q_PROPERTY(QQuick3DObject *wideImage READ wideImage WRITE setWideImage NOTIFY wideImageChanged)
    Q_PROPERTY(QQuick3DObject *writtenBuffer READ writtenBuffer WRITE setWrittenBuffer NOTIFY writtenBufferChanged)
    Q_PROPERTY(QQuick3DObject *engineBuffer READ engineBuffer WRITE setEngineBuffer NOTIFY engineBufferChanged)
    // When true, the extension registers its own QRhiBuffer as the result for
    // writtenBuffer's bufferProvider (which must then be this extension).
    Q_PROPERTY(bool providesWrittenBuffer READ providesWrittenBuffer WRITE setProvidesWrittenBuffer NOTIFY providesWrittenBufferChanged)
    QML_ELEMENT

public:
    // One bit per property of the lookups that held, so a failure names what
    // broke instead of just reporting a mismatched total.
    enum Check {
        ProbeRan              = 1 << 0,
        ImageResolved         = 1 << 1,
        ImageSizeMatches      = 1 << 2,
        ImageFormatMatches    = 1 << 3,
        SecondImageResolved   = 1 << 4,
        ImagesAreDistinct     = 1 << 5,
        WrittenBufferResolved = 1 << 6,
        ProvidedBufferIsOurs  = 1 << 7,
        EngineBufferResolved  = 1 << 8,
        BuffersAreDistinct    = 1 << 9,
        InvalidIdIsEmpty      = 1 << 10,
        ReadbackSupported     = 1 << 11,
        ReadbackCompleted     = 1 << 12,
        ReadbackContentOk     = 1 << 13,
    };
    Q_ENUM(Check)

    // Everything that must hold on any backend.
    static constexpr int CoreChecks = ProbeRan | ImageResolved | ImageSizeMatches | ImageFormatMatches
            | SecondImageResolved | ImagesAreDistinct | WrittenBufferResolved | EngineBufferResolved
            | BuffersAreDistinct | InvalidIdIsEmpty;
    // Additionally required where QRhi::ReadBackNonUniformBuffer is supported.
    static constexpr int ReadbackChecks = ReadbackCompleted | ReadbackContentOk;

    explicit ComputeResultExtension(QQuick3DObject *parent = nullptr);

    QQuick3DObject *image() const { return m_image; }
    void setImage(QQuick3DObject *image);
    QQuick3DObject *wideImage() const { return m_wideImage; }
    void setWideImage(QQuick3DObject *image);
    QQuick3DObject *writtenBuffer() const { return m_writtenBuffer; }
    void setWrittenBuffer(QQuick3DObject *buffer);
    QQuick3DObject *engineBuffer() const { return m_engineBuffer; }
    void setEngineBuffer(QQuick3DObject *buffer);
    bool providesWrittenBuffer() const { return m_providesWrittenBuffer; }
    void setProvidesWrittenBuffer(bool provides);

    // Union of the Check bits that have held in any frame so far. Written on
    // the render thread, read by the test on the main thread.
    int passedChecks() const { return m_state->checks(); }

    // Names the bits of \a expected that are missing from \a passed.
    static QByteArray describeMissingChecks(int passed, int expected);

Q_SIGNALS:
    void imageChanged();
    void wideImageChanged();
    void writtenBufferChanged();
    void engineBufferChanged();
    void providesWrittenBufferChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;

private:
    QPointer<QQuick3DObject> m_image;
    QPointer<QQuick3DObject> m_wideImage;
    QPointer<QQuick3DObject> m_writtenBuffer;
    QPointer<QQuick3DObject> m_engineBuffer;
    bool m_providesWrittenBuffer = false;
    std::shared_ptr<ComputeResultProbeState> m_state = std::make_shared<ComputeResultProbeState>();
};

#endif // COMPUTERESULTEXTENSION_H
