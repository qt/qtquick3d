// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgrendercontextcore_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrendernode_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderlibrarymanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgperframeallocator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererutil_p.h>
#include <QtQuick3DRuntimeRender/private/qssgdebugdrawsystem_p.h>

#include <QtQuick/private/qquickwindow_p.h>

QT_BEGIN_NAMESPACE

static bool loadPregenratedShaders()
{
    return qEnvironmentVariableIntValue("QT_QUICK3D_DISABLE_GENSHADERS") == 0;
}

void QSSGRenderContextInterface::init()
{
    m_renderer->setRenderContextInterface(this);

    m_bufferManager->setRenderContextInterface(this);

    m_customMaterialSystem->setRenderContextInterface(this);
    if (loadPregenratedShaders())
        m_shaderLibraryManager->loadPregeneratedShaderInfo();
}

void QSSGRenderContextInterface::releaseCachedResources()
{
    m_renderer->releaseCachedResources();
    m_shaderCache->releaseCachedResources();
    m_customMaterialSystem->releaseCachedResources();
    m_bufferManager->releaseCachedResources();
    m_rhiContext->releaseCachedResources();
}

QSSGRenderContextInterface::QSSGRenderContextInterface(const QSSGRef<QSSGRhiContext> &ctx,
                                                       const QSSGRef<QSSGBufferManager> &bufferManager,
                                                       const QSSGRef<QSSGRenderer> &renderer,
                                                       const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager,
                                                       const QSSGRef<QSSGShaderCache> &shaderCache,
                                                       const QSSGRef<QSSGCustomMaterialSystem> &customMaterialSystem,
                                                       const QSSGRef<QSSGProgramGenerator> &shaderProgramGenerator)
    : m_rhiContext(ctx)
    , m_shaderCache(shaderCache)
    , m_bufferManager(bufferManager)
    , m_renderer(renderer)
    , m_shaderLibraryManager(shaderLibraryManager)
    , m_customMaterialSystem(customMaterialSystem)
    , m_shaderProgramGenerator(shaderProgramGenerator)
{
    init();
}

// The shader library is a global object, not per-QQuickWindow, hence not owned
// by the QSSGRenderContextInterface.
static const QSSGRef<QSSGShaderLibraryManager> &q3ds_shaderLibraryManager()
{
    static QSSGRef<QSSGShaderLibraryManager> shaderLibraryManager;
    if (!shaderLibraryManager)
        shaderLibraryManager = new QSSGShaderLibraryManager;
    return shaderLibraryManager;
}

QSSGRenderContextInterface::QSSGRenderContextInterface(const QSSGRef<QSSGRhiContext> &ctx)
    : m_rhiContext(ctx)
    , m_shaderCache(new QSSGShaderCache(ctx))
    , m_bufferManager(new QSSGBufferManager)
    , m_renderer(new QSSGRenderer)
    , m_shaderLibraryManager(q3ds_shaderLibraryManager())
    , m_customMaterialSystem(new QSSGCustomMaterialSystem)
    , m_shaderProgramGenerator(new QSSGProgramGenerator)
    , m_debugDrawSystem(new QSSGDebugDrawSystem)
{
    init();
}

QSSGRenderContextInterface::~QSSGRenderContextInterface()
{
    m_renderer->releaseCachedResources();
}

const QSSGRef<QSSGRenderer> &QSSGRenderContextInterface::renderer() const
{
    return m_renderer;
}

const QSSGRef<QSSGBufferManager> &QSSGRenderContextInterface::bufferManager() const
{
    return m_bufferManager;
}

const QSSGRef<QSSGRhiContext> &QSSGRenderContextInterface::rhiContext() const
{
    return m_rhiContext;
}

const QSSGRef<QSSGShaderCache> &QSSGRenderContextInterface::shaderCache() const
{
    return m_shaderCache;
}

const QSSGRef<QSSGShaderLibraryManager> &QSSGRenderContextInterface::shaderLibraryManager() const
{
    return m_shaderLibraryManager;
}

const QSSGRef<QSSGCustomMaterialSystem> &QSSGRenderContextInterface::customMaterialSystem() const
{
    return m_customMaterialSystem;
}

const QSSGRef<QSSGProgramGenerator> &QSSGRenderContextInterface::shaderProgramGenerator() const
{
    return m_shaderProgramGenerator;
}

const QSSGRef<QSSGDebugDrawSystem> &QSSGRenderContextInterface::debugDrawSystem() const
{
    return m_debugDrawSystem;
}

QRhi *QSSGRenderContextInterface::rhi() const
{
    return m_rhiContext->rhi();
}

void QSSGRenderContextInterface::cleanupResources(QList<QSSGRenderGraphObject *> &resources)
{
    m_renderer->cleanupResources(resources);
}

void QSSGRenderContextInterface::cleanupResources(QSet<QSSGRenderGraphObject *> &resources)
{
    m_renderer->cleanupResources(resources);
}

void QSSGRenderContextInterface::cleanupUnreferencedBuffers(QSSGRenderLayer *inLayer)
{
    // Now check for unreferenced buffers and release them if necessary
    m_bufferManager->cleanupUnreferencedBuffers(m_frameCount, inLayer);
}


void QSSGRenderContextInterface::resetResourceCounters(QSSGRenderLayer *inLayer)
{
    m_bufferManager->resetUsageCounters(m_frameCount, inLayer);
}

void QSSGRenderContextInterface::beginFrame(QSSGRenderLayer *layer, bool allowRecursion)
{
    if (allowRecursion) {
        if (m_activeFrameRef++ != 0)
            return;
    }

    m_perFrameAllocator.reset();
    m_renderer->beginFrame(layer);
    resetResourceCounters(layer);
}

bool QSSGRenderContextInterface::prepareLayerForRender(QSSGRenderLayer &inLayer)
{
    return m_renderer->prepareLayerForRender(inLayer);
}

void QSSGRenderContextInterface::rhiPrepare(QSSGRenderLayer &inLayer)
{
    m_renderer->rhiPrepare(inLayer);
}

void QSSGRenderContextInterface::rhiRender(QSSGRenderLayer &inLayer)
{
    m_renderer->rhiRender(inLayer);
}

bool QSSGRenderContextInterface::endFrame(QSSGRenderLayer *layer, bool allowRecursion)
{
    if (allowRecursion) {
        if (--m_activeFrameRef != 0)
            return false;
    }

    cleanupUnreferencedBuffers(layer);

    m_renderer->endFrame(layer);
    ++m_frameCount;

    return true;
}

QT_END_NAMESPACE

