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

QSSGRenderContextInterface::QSSGRenderContextInterface(std::unique_ptr<QSSGBufferManager> &&bufferManager,
                                                       std::unique_ptr<QSSGRenderer> renderer,
                                                       std::shared_ptr<QSSGShaderLibraryManager> shaderLibraryManager,
                                                       std::unique_ptr<QSSGShaderCache> shaderCache,
                                                       std::unique_ptr<QSSGCustomMaterialSystem> customMaterialSystem,
                                                       std::unique_ptr<QSSGProgramGenerator> shaderProgramGenerator,
                                                       std::unique_ptr<QSSGRhiContext> ctx,
                                                       std::unique_ptr<QSSGDebugDrawSystem> debugDrawSystem)
    : m_rhiContext(std::move(ctx))
    , m_shaderCache(std::move(shaderCache))
    , m_bufferManager(std::move(bufferManager))
    , m_renderer(std::move(renderer))
    , m_shaderLibraryManager(std::move(shaderLibraryManager))
    , m_customMaterialSystem(std::move(customMaterialSystem))
    , m_shaderProgramGenerator(std::move(shaderProgramGenerator))
    , m_debugDrawSystem(std::move(debugDrawSystem))
{
    init();
}

// The shader library is a global object, not per-QQuickWindow, hence not owned
// by the QSSGRenderContextInterface.
static const std::shared_ptr<QSSGShaderLibraryManager> &q3ds_shaderLibraryManager()
{
    static auto shaderLibraryManager = std::make_shared<QSSGShaderLibraryManager>();
    return shaderLibraryManager;
}

QSSGRenderContextInterface::QSSGRenderContextInterface(QRhi *rhi)
    : m_rhiContext(new QSSGRhiContext(rhi))
    , m_shaderCache(new QSSGShaderCache(*m_rhiContext))
    , m_bufferManager(new QSSGBufferManager())
    , m_renderer(new QSSGRenderer())
    , m_shaderLibraryManager(q3ds_shaderLibraryManager())
    , m_customMaterialSystem(new QSSGCustomMaterialSystem())
    , m_shaderProgramGenerator(new QSSGProgramGenerator())
    , m_debugDrawSystem(new QSSGDebugDrawSystem())
{
    init();
}

QSSGRenderContextInterface::~QSSGRenderContextInterface()
{
    m_renderer->releaseCachedResources();
}

const std::unique_ptr<QSSGRenderer> &QSSGRenderContextInterface::renderer() const
{
    return m_renderer;
}

const std::unique_ptr<QSSGBufferManager> &QSSGRenderContextInterface::bufferManager() const
{
    return m_bufferManager;
}

const std::unique_ptr<QSSGRhiContext> &QSSGRenderContextInterface::rhiContext() const
{
    return m_rhiContext;
}

const std::unique_ptr<QSSGShaderCache> &QSSGRenderContextInterface::shaderCache() const
{
    return m_shaderCache;
}

const std::shared_ptr<QSSGShaderLibraryManager> &QSSGRenderContextInterface::shaderLibraryManager() const
{
    return m_shaderLibraryManager;
}

const std::unique_ptr<QSSGCustomMaterialSystem> &QSSGRenderContextInterface::customMaterialSystem() const
{
    return m_customMaterialSystem;
}

const std::unique_ptr<QSSGProgramGenerator> &QSSGRenderContextInterface::shaderProgramGenerator() const
{
    return m_shaderProgramGenerator;
}

const std::unique_ptr<QSSGDebugDrawSystem> &QSSGRenderContextInterface::debugDrawSystem() const
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

