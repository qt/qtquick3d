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

#include "qssgrendercontextcore_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrendernode_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderresourcemanager_p.h>
#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderinputstreamfactory_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRender/private/qssgrenderframebuffer_p.h>
#include <QtQuick3DRender/private/qssgrenderrenderbuffer_p.h>
#include <QtQuick3DRender/private/qssgrendertexture2d_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderthreadpool_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimagebatchloader_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdynamicobjectsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegeneratorv2_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgperframeallocator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererimpl_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererutil_p.h>

#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

static int idealThreadCount()
{
    static const int threads = qEnvironmentVariableIntValue("QT_QUICK3D_THREAD_COUNT");
    return (threads > 0) ? threads : QThread::idealThreadCount();
}

QSSGRenderContextInterface::QSSGRenderContextInterface(const QSSGRef<QSSGRenderContext> &ctx, const QString &inApplicationDirectory)
    : m_renderContext(ctx)
    , m_inputStreamFactory(new QSSGInputStreamFactory)
    , m_bufferManager(new QSSGBufferManager(ctx, m_inputStreamFactory, &m_perfTimer))
    , m_resourceManager(new QSSGResourceManager(ctx))
    , m_renderer(QSSGRendererInterface::createRenderer(this))
    , m_dynamicObjectSystem(new QSSGDynamicObjectSystem(this))
    , m_effectSystem(new QSSGEffectSystem(this))
    , m_shaderCache(QSSGShaderCache::createShaderCache(ctx, m_inputStreamFactory, &m_perfTimer))
    , m_threadPool(QSSGAbstractThreadPool::createThreadPool(idealThreadCount()))
    , m_customMaterialSystem(new QSSGMaterialSystem(this))
    , m_shaderProgramGenerator(QSSGShaderProgramGeneratorInterface::createProgramGenerator(this))
    , m_defaultMaterialShaderGenerator(QSSGDefaultMaterialShaderGeneratorInterface::createDefaultMaterialShaderGenerator(this))
    , m_customMaterialShaderGenerator(QSSGMaterialShaderGeneratorInterface::createCustomMaterialShaderGenerator(this))
{
    if (!inApplicationDirectory.isEmpty())
        m_inputStreamFactory->addSearchDirectory(inApplicationDirectory);

    const_cast<QSSGRef<IImageBatchLoader> &>(m_imageBatchLoader) = IImageBatchLoader::createBatchLoader(m_inputStreamFactory, m_bufferManager, m_threadPool, &m_perfTimer);
    m_customMaterialSystem->setRenderContextInterface(this);


    const char *versionString = nullptr;
    switch (ctx->renderContextType()) {
    case QSSGRenderContextType::GLES2:
        versionString = "gles2";
        break;
    case QSSGRenderContextType::GL2:
        versionString = "gl2";
        break;
    case QSSGRenderContextType::GLES3:
        versionString = "gles3";
        break;
    case QSSGRenderContextType::GL3:
        versionString = "gl3";
        break;
    case QSSGRenderContextType::GLES3PLUS:
        versionString = "gles3x";
        break;
    case QSSGRenderContextType::GL4:
        versionString = "gl4";
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    dynamicObjectSystem()->setShaderCodeLibraryVersion(versionString);
#if defined(QSSG_SHADER_PLATFORM_LIBRARY_DIR)
    const QString platformDirectory;
#if defined(_WIN32)
    platformDirectory = QStringLiteral("res/platform/win");
#elif defined(_LINUX)
    platformDirectory = QStringLiteral("res/platform/linux");
#elif defined(_MACOSX)
    platformDirectory = QStringLiteral("res/platform/macos");
#endif
    GetDynamicObjectSystem().setShaderCodeLibraryPlatformDirectory(platformDirectory);
#endif
}

struct QSSGRenderContextInterfaceHandle
{
    QSSGRenderContextInterface *ctx;
    quintptr m_wid;
};
Q_DECLARE_TYPEINFO(QSSGRenderContextInterfaceHandle, Q_PRIMITIVE_TYPE);

Q_GLOBAL_STATIC(QVector<QSSGRenderContextInterfaceHandle>, g_renderContexts)

QSSGRenderContextInterface::~QSSGRenderContextInterface()
{
    m_renderContext->releaseResources();
    static_cast<QSSGRendererImpl *>(m_renderer.data())->releaseResources();

    for (int i = 0; i < g_renderContexts->size(); ++i) {
        if (g_renderContexts->at(i).ctx == this) {
            g_renderContexts->removeAt(i);
            break;
        }
    }
}

QSSGRef<QSSGRenderContextInterface> QSSGRenderContextInterface::getRenderContextInterface(const QSSGRef<QSSGRenderContext> &ctx, const QString &inApplicationDirectory, quintptr wid)
{
    auto it = g_renderContexts->cbegin();
    const auto end = g_renderContexts->cend();
    for (; it != end; ++it) {
        if (it->m_wid == wid)
            break;
    }

    if (it != end)
        return it->ctx;

    const auto rci = QSSGRef<QSSGRenderContextInterface>(new QSSGRenderContextInterface(ctx, inApplicationDirectory));
    g_renderContexts->push_back(QSSGRenderContextInterfaceHandle { rci.data(), wid });

    return rci;
}

QSSGRef<QSSGRenderContextInterface> QSSGRenderContextInterface::getRenderContextInterface(quintptr wid)
{
    auto it = g_renderContexts->cbegin();
    const auto end = g_renderContexts->cend();
    for (; it != end; ++it) {
        if (it->m_wid == wid)
            break;
    }

    if (it != end)
        return it->ctx;

    return QSSGRef<QSSGRenderContextInterface>();
}

const QSSGRef<QSSGRendererInterface> &QSSGRenderContextInterface::renderer() const { return m_renderer; }

const QSSGRef<QSSGBufferManager> &QSSGRenderContextInterface::bufferManager() const { return m_bufferManager; }

const QSSGRef<QSSGResourceManager> &QSSGRenderContextInterface::resourceManager() const { return m_resourceManager; }

const QSSGRef<QSSGRenderContext> &QSSGRenderContextInterface::renderContext() const { return m_renderContext; }

const QSSGRef<QSSGInputStreamFactory> &QSSGRenderContextInterface::inputStreamFactory() const { return m_inputStreamFactory; }

const QSSGRef<QSSGEffectSystem> &QSSGRenderContextInterface::effectSystem() const { return m_effectSystem; }

const QSSGRef<QSSGShaderCache> &QSSGRenderContextInterface::shaderCache() const { return m_shaderCache; }

const QSSGRef<QSSGAbstractThreadPool> &QSSGRenderContextInterface::threadPool() const { return m_threadPool; }

const QSSGRef<IImageBatchLoader> &QSSGRenderContextInterface::imageBatchLoader() const { return m_imageBatchLoader; }

const QSSGRef<QSSGDynamicObjectSystem> &QSSGRenderContextInterface::dynamicObjectSystem() const { return m_dynamicObjectSystem; }

const QSSGRef<QSSGMaterialSystem> &QSSGRenderContextInterface::customMaterialSystem() const { return m_customMaterialSystem; }

const QSSGRef<QSSGShaderProgramGeneratorInterface> &QSSGRenderContextInterface::shaderProgramGenerator() const
{
    return m_shaderProgramGenerator;
}

const QSSGRef<QSSGDefaultMaterialShaderGeneratorInterface> &QSSGRenderContextInterface::defaultMaterialShaderGenerator() const
{
    return m_defaultMaterialShaderGenerator;
}

const QSSGRef<QSSGMaterialShaderGeneratorInterface> &QSSGRenderContextInterface::customMaterialShaderGenerator() const
{
    return m_customMaterialShaderGenerator;
}

QVector2D QSSGRenderContextInterface::mousePickViewport() const
{
    return QVector2D((float)m_windowDimensions.width(), (float)m_windowDimensions.height());
}

QRect QSSGRenderContextInterface::contextViewport() const
{
    QRect retval;
    if (!m_viewport.isNull())
        retval = m_viewport;
    else
        retval = QRect(0, 0, m_windowDimensions.width(), m_windowDimensions.height());

    return retval;
}

QVector2D QSSGRenderContextInterface::mousePickMouseCoords(const QVector2D &inMouseCoords) const
{
    return inMouseCoords;
}


void QSSGRenderContextInterface::dumpGpuProfilerStats()
{
    m_renderer->dumpGpuProfilerStats();
}

void QSSGRenderContextInterface::beginFrame()
{
    m_perFrameAllocator.reset();
    m_renderer->beginFrame();
    m_imageBatchLoader->beginFrame();
}

bool QSSGRenderContextInterface::prepareLayerForRender(QSSGRenderLayer &inLayer)
{
    return renderer()->prepareLayerForRender(inLayer, m_windowDimensions);
}

void QSSGRenderContextInterface::renderLayer(QSSGRenderLayer &inLayer, bool needsClear)
{
    renderer()->renderLayer(inLayer, m_windowDimensions, needsClear, m_sceneColor);
}

void QSSGRenderContextInterface::endFrame()
{
    m_imageBatchLoader->endFrame();
    m_renderer->endFrame();
    m_customMaterialSystem->endFrame();
    ++m_frameCount;
}

QT_END_NAMESPACE

