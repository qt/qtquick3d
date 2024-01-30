// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGRENDEREXTENSIIONS_H
#define QSSGRENDEREXTENSIIONS_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QtQuick3D API, with limited compatibility guarantees.
// Usage of this API may make your code source and binary incompatible with
// future versions of Qt.
//

#include <QtQuick3DRuntimeRender/qtquick3druntimerenderexports.h>
#include <ssg/qssgrenderbasetypes.h>
#include <ssg/qssgrendergraphobject.h>
#include <ssg/qssgrhicontext.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QSSGRenderContextInterface;
class QSSGLayerRenderData;
class QRhiTexture;
class QRhiRenderBuffer;

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGFrameData
{
public:
    enum class RenderResult : quint32
    {
        AoTexture,
        DepthTexture,
        ScreenTexture
    };

    struct Result
    {
        QRhiTexture *texture = nullptr;
        QRhiRenderBuffer *buffer = nullptr;
    };

    Result getRenderResult(RenderResult id) const;

    [[nodiscard]] QSSGRhiGraphicsPipelineState getPipelineState() const;

    [[nodiscard]] QSSGCameraId activeCamera() const;

    [[nodiscard]] QSSGRenderContextInterface *contextInterface() const;

private:
    friend class QSSGLayerRenderData;
    friend class QSSGRenderHelpers;

    void clear();

    [[nodiscard]] QSSGLayerRenderData *getCurrent() const;

    QSSGFrameData() = default;
    explicit QSSGFrameData(QSSGRenderContextInterface *ctx);
    QSSGRenderContextInterface *m_ctx = nullptr;
};

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderExtension : public QSSGRenderGraphObject
{
public:
    enum class RenderMode
    {
        Standalone,
        Main
    };

    enum class RenderStage
    {
        PreColor,
        PostColor
    };

    QSSGRenderExtension();
    virtual ~QSSGRenderExtension();

    virtual bool prepareData(QSSGFrameData &data) = 0;
    virtual void prepareRender(QSSGFrameData &data) = 0;
    virtual void render(QSSGFrameData &data) = 0;

    virtual void resetForFrame() = 0;

    virtual RenderMode mode() const = 0;
    virtual RenderStage stage() const = 0;
};

QT_END_NAMESPACE

#endif // QSSGRENDEREXTENSIIONS_H
