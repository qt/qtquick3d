// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSG_RENDER_CONTEXT_CORE_H
#define QSSG_RENDER_CONTEXT_CORE_H

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

#include <QtQuick3DRuntimeRender/private/qssgrendershaderlibrarymanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>
#include <QtQuick3DRuntimeRender/private/qssgperframeallocator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgdebugdrawsystem_p.h>

#include <QtCore/QPair>
#include <QtCore/QSize>

QT_BEGIN_NAMESPACE

class QSSGCustomMaterialSystem;
class QSSGRendererInterface;
class QQuickWindow;
class QSSGDebugDrawSystem;

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderContextInterface
{
    Q_DISABLE_COPY(QSSGRenderContextInterface)
public:
    // The commonly used version (from QQuick3DSceneRenderer). There is one
    // rendercontext per QQuickWindow (and so scenegraph render thread).
    explicit QSSGRenderContextInterface(QRhi *rhi);

    // This overload must only be used in special cases, e.g. by the genshaders tool.
    QSSGRenderContextInterface(std::unique_ptr<QSSGBufferManager> &&bufferManager,
                               std::unique_ptr<QSSGRenderer> renderer,
                               std::shared_ptr<QSSGShaderLibraryManager> shaderLibraryManager,
                               std::unique_ptr<QSSGShaderCache> shaderCache,
                               std::unique_ptr<QSSGCustomMaterialSystem> customMaterialSystem,
                               std::unique_ptr<QSSGProgramGenerator> shaderProgramGenerator,
                               std::unique_ptr<QSSGRhiContext> ctx,
                               std::unique_ptr<QSSGDebugDrawSystem> debugDrawSystem = nullptr);

    ~QSSGRenderContextInterface();

    const std::unique_ptr<QSSGRenderer> &renderer() const;
    const std::unique_ptr<QSSGBufferManager> &bufferManager() const;
    const std::unique_ptr<QSSGRhiContext> &rhiContext() const;
    const std::unique_ptr<QSSGShaderCache> &shaderCache() const;
    const std::shared_ptr<QSSGShaderLibraryManager> &shaderLibraryManager() const;
    const std::unique_ptr<QSSGCustomMaterialSystem> &customMaterialSystem() const;
    const std::unique_ptr<QSSGProgramGenerator> &shaderProgramGenerator() const;
    const std::unique_ptr<QSSGDebugDrawSystem> &debugDrawSystem() const;
    QRhi *rhi() const;

    // The memory used for the per frame allocator is released as the first step in BeginFrame.
    // This is useful for short lived objects and datastructures.
    QSSGPerFrameAllocator &perFrameAllocator() { return m_perFrameAllocator; }

    // Get the number of times EndFrame has been called
    quint32 frameCount() { return m_frameCount; }

    void setSceneColor(const QColor &inSceneColor) { m_sceneColor = inSceneColor; }

    void setViewport(QRect inViewport) { m_viewport = inViewport; }
    QRect viewport() const { return m_viewport; }

    void setDpr(float dpr) { m_dpr = dpr; }
    float dpr() const { return m_dpr; }

    void setScissorRect(QRect inScissorRect) { m_scissorRect = inScissorRect; }
    QRect scissorRect() const { return m_scissorRect; }

    void cleanupResources(QList<QSSGRenderGraphObject*> &resources);
    void cleanupResources(QSet<QSSGRenderGraphObject *> &resources);
    void cleanupUnreferencedBuffers(QSSGRenderLayer *inLayer);
    void resetResourceCounters(QSSGRenderLayer *inLayer);

    // Steps needed to render:
    // 1.  beginFrame - Reset the per-frame allocator
    // 2.  Make sure state is set (viewport, windowDimensions, dpr, sceneColor, scissorRect)
    // 3.  prepareLayerForRenderer - process the layer scene (prepare the scene for rendering)
    // 4.  rhiPrepare - start rendering necessary sub-scenes and prepare resources
    // 5.  rhiRender - render the layer scene
    // 6.  endFrame

    // Clients need to call this every frame in order for various subsystems to release
    // temporary per-frame allocated objects.
    void beginFrame(QSSGRenderLayer *layer, bool allowRecursion = true);

    bool prepareLayerForRender(QSSGRenderLayer &inLayer);

    void rhiPrepare(QSSGRenderLayer &inLayer);
    void rhiRender(QSSGRenderLayer &inLayer);

    // When allowRecursion is true, the cleanup is only done when all
    // beginFrames got their corresponding endFrame. This is indicated by the
    // return value (false if nothing's been done due to pending "frames")
    bool endFrame(QSSGRenderLayer *layer, bool allowRecursion = true);

private:
    friend class QQuick3DWindowAttachment;

    void init();
    void releaseCachedResources();

    std::unique_ptr<QSSGRhiContext> m_rhiContext;
    std::unique_ptr<QSSGShaderCache> m_shaderCache;
    std::unique_ptr<QSSGBufferManager> m_bufferManager;
    std::unique_ptr<QSSGRenderer> m_renderer;
    std::shared_ptr<QSSGShaderLibraryManager> m_shaderLibraryManager;
    std::unique_ptr<QSSGCustomMaterialSystem> m_customMaterialSystem;
    std::unique_ptr<QSSGProgramGenerator> m_shaderProgramGenerator;
    std::unique_ptr<QSSGDebugDrawSystem> m_debugDrawSystem;

    QSSGPerFrameAllocator m_perFrameAllocator;
    quint32 m_activeFrameRef = 0;
    quint32 m_frameCount = 0;

    // Viewport that this render context should use
    QRect m_viewport;
    float m_dpr = 1.0;
    QRect m_scissorRect;
    QColor m_sceneColor;
};
QT_END_NAMESPACE

#endif
