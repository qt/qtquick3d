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
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderlibrarymanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgperframeallocator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererutil_p.h>

#include <QtGui/qwindow.h>

#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

using Binding = QPair<const QWindow *, QSSGRenderContextInterface *>;
using Bindings = QVarLengthArray<Binding, 32>;
Q_GLOBAL_STATIC(Bindings, g_windowReg)

static bool loadPregenratedShaders()
{
    return qEnvironmentVariableIntValue("QT_QUICK3D_DISABLE_GENSHADERS") == 0;
}

void QSSGRenderContextInterface::init()
{
    m_renderer->setRenderContextInterface(this);

    m_customMaterialSystem->setRenderContextInterface(this);
    if (loadPregenratedShaders())
        m_shaderLibraryManager->loadPregeneratedShaderInfo();
}

QSSGRenderContextInterface *QSSGRenderContextInterface::renderContextForWindow(const QWindow &window)
{
    auto it = g_windowReg->cbegin();
    const auto end = g_windowReg->cend();
    for (; it != end; ++it) {
        if (it->first == &window)
            break;
    }

    return (it != end) ? it->second : nullptr;
}

QSSGRenderContextInterface::QSSGRenderContextInterface(const QSSGRef<QSSGRhiContext> &ctx,
                                                       const QSSGRef<QSSGBufferManager> &bufferManager,
                                                       const QSSGRef<QSSGResourceManager> &resourceManager,
                                                       const QSSGRef<QSSGRenderer> &renderer,
                                                       const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager,
                                                       const QSSGRef<QSSGShaderCache> &shaderCache,
                                                       const QSSGRef<QSSGCustomMaterialSystem> &customMaterialSystem,
                                                       const QSSGRef<QSSGProgramGenerator> &shaderProgramGenerator)
    : m_rhiContext(ctx)
    , m_shaderCache(shaderCache)
    , m_bufferManager(bufferManager)
    , m_resourceManager(resourceManager)
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

QSSGRenderContextInterface::QSSGRenderContextInterface(QWindow *window,
                                                       const QSSGRef<QSSGRhiContext> &ctx)
    : m_rhiContext(ctx)
    , m_shaderCache(new QSSGShaderCache(ctx))
    , m_bufferManager(new QSSGBufferManager(ctx, m_shaderCache))
    , m_resourceManager(new QSSGResourceManager(ctx))
    , m_renderer(new QSSGRenderer)
    , m_shaderLibraryManager(q3ds_shaderLibraryManager())
    , m_customMaterialSystem(new QSSGCustomMaterialSystem)
    , m_shaderProgramGenerator(new QSSGProgramGenerator)
{
    init();
    if (window) {
        g_windowReg->append({ window, this });
        QObject::connect(window, &QWindow::destroyed, [&](QObject *o){
            g_windowReg->removeIf([o](const Binding &b) { return (b.first == o); });
        });
    }
}

QSSGRenderContextInterface::~QSSGRenderContextInterface()
{
    m_renderer->releaseResources();
    g_windowReg->removeIf([this](const Binding &b) { return (b.second == this); });
}

const QSSGRef<QSSGRenderer> &QSSGRenderContextInterface::renderer() const { return m_renderer; }

const QSSGRef<QSSGBufferManager> &QSSGRenderContextInterface::bufferManager() const { return m_bufferManager; }

const QSSGRef<QSSGResourceManager> &QSSGRenderContextInterface::resourceManager() const { return m_resourceManager; }

const QSSGRef<QSSGRhiContext> &QSSGRenderContextInterface::rhiContext() const { return m_rhiContext; }

const QSSGRef<QSSGShaderCache> &QSSGRenderContextInterface::shaderCache() const { return m_shaderCache; }

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

