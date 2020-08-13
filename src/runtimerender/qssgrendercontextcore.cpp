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
#include <QtQuick3DRuntimeRender/private/qssgrenderresourcemanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderinputstreamfactory_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderthreadpool_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderlibrarymanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgperframeallocator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererutil_p.h>

#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

static int idealThreadCount()
{
    static const int threads = qEnvironmentVariableIntValue("QT_QUICK3D_THREAD_COUNT");
    return (threads > 0) ? threads : QThread::idealThreadCount();
}

static bool loadPregenratedShaders()
{
    return qEnvironmentVariableIntValue("QT_QUICK3D_DISABLE_GENSHADERS") == 0;
}

QSSGRenderContextInterface::QSSGRenderContextInterface(const QSSGRef<QSSGRhiContext> &ctx, const QString &inApplicationDirectory)
    : m_rhiContext(ctx)
    , m_inputStreamFactory(new QSSGInputStreamFactory)
    , m_bufferManager(new QSSGBufferManager(ctx, m_inputStreamFactory))
    , m_resourceManager(new QSSGResourceManager(ctx))
    , m_renderer(new QSSGRenderer)
    , m_shaderLibraryManager(new QSSGShaderLibraryManager(m_inputStreamFactory))
    , m_shaderCache(new QSSGShaderCache(ctx, m_inputStreamFactory))
    , m_threadPool(QSSGAbstractThreadPool::createThreadPool(idealThreadCount()))
    , m_customMaterialSystem(new QSSGCustomMaterialSystem)
    , m_shaderProgramGenerator(new QSSGProgramGenerator)
{
    m_renderer->setRenderContextInterface(this);

    if (!inApplicationDirectory.isEmpty())
        m_inputStreamFactory->addSearchDirectory(inApplicationDirectory);

    m_customMaterialSystem->setRenderContextInterface(this);
    if (loadPregenratedShaders())
        m_shaderLibraryManager->loadPregeneratedShaderInfo(":/genshaders.keys");
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
    m_renderer->releaseResources();

    for (int i = 0; i < g_renderContexts->size(); ++i) {
        if (g_renderContexts->at(i).ctx == this) {
            g_renderContexts->removeAt(i);
            break;
        }
    }
}

QSSGRef<QSSGRenderContextInterface> QSSGRenderContextInterface::getRenderContextInterface(const QSSGRef<QSSGRhiContext> &ctx, const QString &inApplicationDirectory, quintptr wid)
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

const QSSGRef<QSSGRenderer> &QSSGRenderContextInterface::renderer() const { return m_renderer; }

const QSSGRef<QSSGBufferManager> &QSSGRenderContextInterface::bufferManager() const { return m_bufferManager; }

const QSSGRef<QSSGResourceManager> &QSSGRenderContextInterface::resourceManager() const { return m_resourceManager; }

const QSSGRef<QSSGRhiContext> &QSSGRenderContextInterface::rhiContext() const { return m_rhiContext; }

const QSSGRef<QSSGInputStreamFactory> &QSSGRenderContextInterface::inputStreamFactory() const { return m_inputStreamFactory; }

const QSSGRef<QSSGShaderCache> &QSSGRenderContextInterface::shaderCache() const { return m_shaderCache; }

const QSSGRef<QSSGAbstractThreadPool> &QSSGRenderContextInterface::threadPool() const { return m_threadPool; }

const QSSGRef<QSSGShaderLibraryManager> &QSSGRenderContextInterface::shaderLibraryManager() const { return m_shaderLibraryManager; }

const QSSGRef<QSSGCustomMaterialSystem> &QSSGRenderContextInterface::customMaterialSystem() const { return m_customMaterialSystem; }

const QSSGRef<QSSGProgramGenerator> &QSSGRenderContextInterface::shaderProgramGenerator() const
{
    return m_shaderProgramGenerator;
}

QVector2D QSSGRenderContextInterface::mousePickViewport() const
{
    return QVector2D((float)m_windowDimensions.width(), (float)m_windowDimensions.height());
}

QVector2D QSSGRenderContextInterface::mousePickMouseCoords(const QVector2D &inMouseCoords) const
{
    return inMouseCoords;
}

void QSSGRenderContextInterface::cleanupResources(QList<QSSGRenderGraphObject *> &resources)
{
    m_renderer->cleanupResources(resources);
}

void QSSGRenderContextInterface::beginFrame(bool allowRecursion)
{
    if (allowRecursion) {
        if (m_activeFrameRef++ != 0)
            return;
    }

    m_perFrameAllocator.reset();
    m_renderer->beginFrame();
}

bool QSSGRenderContextInterface::prepareLayerForRender(QSSGRenderLayer &inLayer)
{
    return m_renderer->prepareLayerForRender(inLayer, m_windowDimensions);
}

void QSSGRenderContextInterface::rhiPrepare(QSSGRenderLayer &inLayer)
{
    m_renderer->rhiPrepare(inLayer);
}

void QSSGRenderContextInterface::rhiRender(QSSGRenderLayer &inLayer)
{
    m_renderer->rhiRender(inLayer);
}

bool QSSGRenderContextInterface::endFrame(bool allowRecursion)
{
    if (allowRecursion) {
        if (--m_activeFrameRef != 0)
            return false;
    }

    m_renderer->endFrame();
    ++m_frameCount;

    return true;
}

QT_END_NAMESPACE

