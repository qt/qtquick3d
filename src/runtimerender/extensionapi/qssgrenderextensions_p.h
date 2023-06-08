// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGRENDEREXTENSIIONS_P_H
#define QSSGRENDEREXTENSIIONS_P_H

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

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderableobjects_p.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QSSGRenderer;
class QSSGLayerRenderData;

// NOTE: We might want to move this to the base types header
using QSSGNodeId = quintptr;
using QSSGResourceId = quintptr;

struct QSSGRhiRenderableTexture;

struct QSSGRenderNode;
struct QSSGRenderMesh;

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGFrameData
{
public:
    enum class RenderResult : quint32
    {
        AoTexture,
        DepthTexture,
        ScreenTexture
    };

    using RenderResultT = std::underlying_type_t<RenderResult>;

    const QSSGRhiRenderableTexture *getRenderResult(RenderResult id) const;

    [[nodiscard]] QSSGRhiGraphicsPipelineState getPipelineState() const;

    [[nodiscard]] QSSGRenderableNodeEntry getNode(QSSGNodeId id) const;
    [[nodiscard]] QSSGRenderableNodeEntry takeNode(QSSGNodeId id);

    [[nodiscard]] QSSGRenderGraphObject *getResource(QSSGResourceId id) const;

    [[nodiscard]] QSSGRenderCamera *camera() const;

    [[nodiscard]] QSSGRenderer *renderer() const { return m_renderer; }

private:
    friend class QSSGLayerRenderData;
    friend class QSSGRenderHelpers;

    void clear();

    [[nodiscard]] QSSGLayerRenderData *getCurrent() const;

    QSSGFrameData() = default;
    explicit QSSGFrameData(QSSGRenderer *renderer);
    QSSGRenderer *m_renderer = nullptr;
};

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderExtension : public QSSGRenderGraphObject
{
public:
    enum class Type
    {
        Standalone,
        Main
    };

    enum class RenderMode
    {
        Underlay,
        Overlay
    };

    QSSGRenderExtension();
    virtual ~QSSGRenderExtension();

    virtual bool prepareData(QSSGFrameData &data) = 0;
    virtual void prepareRender(const QSSGRenderer &renderer, QSSGFrameData &data) = 0;
    virtual void render(const QSSGRenderer &renderer) = 0;

    virtual void release() = 0;

    virtual Type type() const = 0;
    virtual RenderMode mode() const = 0;
};

QT_END_NAMESPACE

#endif // QSSGRENDEREXTENSIIONS_P_H
