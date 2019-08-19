/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

#include <QtQuick3DRuntimeRender/private/qssgrenderinputstreamfactory_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderthreadpool_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdynamicobjectsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendereffectsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderwidgets_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimagebatchloader_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderpixelgraphicsrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderrenderlist_p.h>
#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderinputstreamfactory_p.h>
#include <QtQuick3DRuntimeRender/private/qssgperframeallocator_p.h>

#include <QtQuick3DUtils/private/qssgperftimer_p.h>

#include <QtCore/QPair>
#include <QtCore/QSize>

QT_BEGIN_NAMESPACE

enum class ScaleModes
{
    ExactSize = 0, // Ensure the viewport is exactly same size as application
    ScaleToFit = 1, // Resize viewport keeping aspect ratio
    ScaleToFill = 2, // Resize viewport to entire window
    FitSelected = 3, // Resize presentation to fit into viewport
};

class QSSGPathManagerInterface;
class QSSGMaterialSystem;
class QSSGRendererInterface;
class QSSGShaderCache;
class QSSGOffscreenRenderManager;

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderContextInterface
{
    Q_DISABLE_COPY(QSSGRenderContextInterface)
public:
    QAtomicInt ref;
private:
    const QSSGRef<QSSGRenderContext> m_renderContext;
    QSSGPerfTimer m_perfTimer;

    const QSSGRef<QSSGInputStreamFactory> m_inputStreamFactory;
    const QSSGRef<QSSGBufferManager> m_bufferManager;
    const QSSGRef<QSSGResourceManager> m_resourceManager;
    const QSSGRef<QSSGOffscreenRenderManager> m_offscreenRenderManager;
    const QSSGRef<QSSGRendererInterface> m_renderer;
    const QSSGRef<QSSGDynamicObjectSystem> m_dynamicObjectSystem;
    const QSSGRef<QSSGEffectSystem> m_effectSystem;
    const QSSGRef<QSSGShaderCache> m_shaderCache;
    const QSSGRef<QSSGAbstractThreadPool> m_threadPool;
    const QSSGRef<IImageBatchLoader> m_imageBatchLoader;
    const QSSGRef<QSSGMaterialSystem> m_customMaterialSystem;
    const QSSGRef<QSSGPixelGraphicsRendererInterface> m_pixelGraphicsRenderer;
    const QSSGRef<QSSGPathManagerInterface> m_pathManager;
    const QSSGRef<QSSGShaderProgramGeneratorInterface> m_shaderProgramGenerator;
    const QSSGRef<QSSGDefaultMaterialShaderGeneratorInterface> m_defaultMaterialShaderGenerator;
    const QSSGRef<QSSGMaterialShaderGeneratorInterface> m_customMaterialShaderGenerator;
    QSSGPerFrameAllocator m_perFrameAllocator;
    const QSSGRef<QSSGRenderList> m_renderList;
    quint32 m_frameCount = 0;
    // Viewport that this render context should use
    QSSGOption<QRect> m_viewport;
    QSize m_windowDimensions {800, 480};
    ScaleModes m_scaleMode = ScaleModes::ExactSize;
    bool m_wireframeMode = false;
    bool m_isInSubPresentation = false;
    QSSGOption<QVector4D> m_sceneColor;
    QSSGOption<QVector4D> m_matteColor;
    QSSGRef<QSSGRenderFrameBuffer> m_rotationFbo;
    QSSGRef<QSSGRenderTexture2D> m_rotationTexture;
    QSSGRef<QSSGRenderRenderBuffer> m_rotationDepthBuffer;
    QSSGRef<QSSGRenderFrameBuffer> m_contextRenderTarget;
    QRect m_presentationViewport;
    QSize m_presentationDimensions;
    QSize m_renderPresentationDimensions;
    QSize m_preRenderPresentationDimensions;
    QVector2D m_presentationScale {0.0f, 0.0f};
    QRect m_virtualViewport;
    QPair<float, int> m_fps = qMakePair(0.0f, 0);
    bool m_authoringMode = false;

    struct BeginFrameResult
    {
        bool renderOffscreen;
        QSize presentationDimensions;
        bool scissorTestEnabled;
        QRect scissorRect;
        QRect viewport;
        QSize fboDimensions;
        BeginFrameResult(bool ro, QSize presDims, bool scissorEnabled, QRect inScissorRect, QRect inViewport, QSize fboDims)
            : renderOffscreen(ro)
            , presentationDimensions(presDims)
            , scissorTestEnabled(scissorEnabled)
            , scissorRect(inScissorRect)
            , viewport(inViewport)
            , fboDimensions(fboDims)
        {
        }
        BeginFrameResult() = default;
    };

    // Calculated values passed from beginframe to setupRenderTarget.
    // Trying to avoid duplicate code as much as possible.
    BeginFrameResult m_beginFrameResult;

    QPair<QRect, QRect> getPresentationViewportAndOuterViewport() const;
    QRect presentationViewport(const QRect &inViewerViewport, ScaleModes inScaleToFit, const QSize &inPresDimensions) const;
    void setupRenderTarget();
    void teardownRenderTarget();
    QSSGRenderContextInterface(const QSSGRef<QSSGRenderContext> &ctx, const QString &inApplicationDirectory);

    static void releaseRenderContextInterface(quintptr wid);

public:
    // TODO: temp workaround for now
    struct QSSGRenderContextInterfacePtr
    {
        QSSGRenderContextInterfacePtr() = default;
        QSSGRenderContextInterfacePtr(QSSGRenderContextInterface *ptr, quintptr wid) : m_ptr(ptr), m_wid(wid) {}
        QSSGRenderContextInterface * operator-> () const { return m_ptr.data(); }
        ~QSSGRenderContextInterfacePtr() { if (!m_ptr.isNull() && m_ptr->ref == 0) QSSGRenderContextInterface::releaseRenderContextInterface(m_wid); }
        bool isNull() const { return m_ptr.data() == nullptr; }
    private:
        friend QSSGRenderContextInterface;
        QSSGRef<QSSGRenderContextInterface> m_ptr;
        quintptr m_wid = 0;
    };

    static QSSGRenderContextInterface::QSSGRenderContextInterfacePtr getRenderContextInterface(const QSSGRef<QSSGRenderContext> &ctx, const QString &inApplicationDirectory, quintptr wid);
    static QSSGRenderContextInterface::QSSGRenderContextInterfacePtr getRenderContextInterface(quintptr wid);

    ~QSSGRenderContextInterface();
    const QSSGRef<QSSGRendererInterface> &renderer() const;
    QSSGRef<QSSGRendererImpl> renderWidgetContext();
    const QSSGRef<QSSGBufferManager> &bufferManager() const;
    const QSSGRef<QSSGResourceManager> &resourceManager() const;
    const QSSGRef<QSSGRenderContext> &renderContext() const;
    const QSSGRef<QSSGOffscreenRenderManager> &offscreenRenderManager() const;
    const QSSGRef<QSSGInputStreamFactory> &inputStreamFactory() const;
    const QSSGRef<QSSGEffectSystem> &effectSystem() const;
    const QSSGRef<QSSGShaderCache> &shaderCache() const;
    const QSSGRef<QSSGAbstractThreadPool> &threadPool() const;
    const QSSGRef<IImageBatchLoader> &imageBatchLoader() const;
    const QSSGRef<QSSGDynamicObjectSystem> &dynamicObjectSystem() const;
    const QSSGRef<QSSGMaterialSystem> &customMaterialSystem() const;
    const QSSGRef<QSSGPixelGraphicsRendererInterface> &pixelGraphicsRenderer() const;
    QSSGPerfTimer *performanceTimer() { return &m_perfTimer; }
    const QSSGRef<QSSGRenderList> &renderList() const;
    const QSSGRef<QSSGPathManagerInterface> &pathManager() const;
    const QSSGRef<QSSGShaderProgramGeneratorInterface> &shaderProgramGenerator() const;
    const QSSGRef<QSSGDefaultMaterialShaderGeneratorInterface> &defaultMaterialShaderGenerator() const;
    const QSSGRef<QSSGMaterialShaderGeneratorInterface> &customMaterialShaderGenerator() const;
    // The memory used for the per frame allocator is released as the first step in BeginFrame.
    // This is useful for short lived objects and datastructures.
    QSSGPerFrameAllocator &perFrameAllocator() { return m_perFrameAllocator; }

    // Get the number of times EndFrame has been called
    quint32 frameCount() { return m_frameCount; }

    // Get fps
    QPair<float, int> getFPS() { return m_fps; }
    // Set fps by higher level, etc application
    void setFPS(QPair<float, int> inFPS) { m_fps = inFPS; }

    // Currently there are a few things that need to work differently
    // in authoring mode vs. runtime.  The particle effects, for instance
    // need to be framerate-independent at runtime but framerate-dependent during
    // authoring time assuming virtual 16 ms frames.
    // Defaults to falst.
    bool isAuthoringMode() { return m_authoringMode; }
    void setAuthoringMode(bool inMode) { m_authoringMode = inMode; }

    // Sub presentations change the rendering somewhat.
    bool isInSubPresentation() { return m_isInSubPresentation; }
    void setInSubPresentation(bool inValue) { m_isInSubPresentation = inValue; }
    void setSceneColor(QSSGOption<QVector4D> inSceneColor) { m_sceneColor = inSceneColor; }
    void setMatteColor(QSSGOption<QVector4D> inMatteColor) { m_matteColor = inMatteColor; }

    // render Gpu profiler values
    void dumpGpuProfilerStats();

    // The reason you can set both window dimensions and an overall viewport is that the mouse
    // needs to be inverted
    // which requires the window height, and then the rest of the system really requires the
    // viewport.
    void setWindowDimensions(const QSize &inWindowDimensions) { m_windowDimensions = inWindowDimensions; }
    QSize windowDimensions() { return m_windowDimensions; }

    // In addition to the window dimensions which really have to be set, you can optionally
    // set the viewport which will force the entire viewer to render specifically to this
    // viewport.
    void setViewport(QSSGOption<QRect> inViewport) { m_viewport = inViewport; }
    QSSGOption<QRect> viewport() const { return m_viewport; }
    QRect contextViewport() const;
    // Only valid between calls to Begin,End.
    QRect presentationViewport() const { return m_presentationViewport; }

    void setScaleMode(ScaleModes inMode) { m_scaleMode = inMode; }
    ScaleModes scaleMode() { return m_scaleMode; }

    void setWireframeMode(bool inEnable) { m_wireframeMode = inEnable; }
    bool wireframeMode() { return m_wireframeMode; }

    // Return the viewport the system is using to render data to.  This gives the the dimensions
    // of the rendered system.  It is dependent on but not equal to the viewport.
    QRectF displayViewport() const { return getPresentationViewportAndOuterViewport().first; }

    // Layers require the current presentation dimensions in order to render.
    void setPresentationDimensions(const QSize &inPresentationDimensions)
    {
        m_presentationDimensions = inPresentationDimensions;
    }
    QSize currentPresentationDimensions() const { return m_presentationDimensions; }

    QVector2D mousePickViewport() const;
    QVector2D mousePickMouseCoords(const QVector2D &inMouseCoords) const;

    // Valid during and just after prepare for render.
    QVector2D presentationScaleFactor() const { return m_presentationScale; }

    // Steps needed to render:
    // 1.  BeginFrame - sets up new target in render graph
    // 2.  Add everything you need to the render graph
    // 3.  RunRenderGraph - runs the graph, rendering things to main render target
    // 4.  Render any additional stuff to main render target on top of previously rendered
    // information
    // 5.  EndFrame

    // Clients need to call this every frame in order for various subsystems to release
    // temporary per-frame allocated objects.
    // Also sets up the viewport according to SetViewportInfo
    // and the topmost presentation dimensions.  Expects there to be exactly one presentation
    // dimension pushed at this point.
    // This also starts a render target in the render graph.
    void beginFrame();

    // This runs through the added tasks in reverse order.  This is used to render dependencies
    // before rendering to the main render target.
    void runRenderTasks();
    // Now you can render to the main render target if you want to render over the top
    // of everything.
    // Next call end frame.
    void endFrame();
};
QT_END_NAMESPACE

#endif
