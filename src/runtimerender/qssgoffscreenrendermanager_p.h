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

#ifndef QSSG_OFFSCREEN_RENDER_MANAGER_H
#define QSSG_OFFSCREEN_RENDER_MANAGER_H

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

#include <QtQuick3DUtils/private/qssgoption_p.h>
#include <QtQuick3DRender/private/qssgrendertexture2d_p.h>
#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>

QT_BEGIN_NAMESPACE
class QSSGResourceManager;
struct QSSGRenderPickResult;
class QSSGGraphObjectPickQueryInterface;
class QSSGRenderContextInterface;
class QSSGRenderContext;

enum class QSSGOffscreenRendererDepthValues
{
    NoDepthBuffer = 0,
    Depth16, // 16 bit depth buffer
    Depth24, // 24 bit depth buffer
    Depth32, // 32 bit depth buffer
    Depth24Stencil8 // 24 bit depth buffer 8 bit stencil buffer
};

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGOffscreenRendererEnvironment
{
    qint32 width = 0;
    qint32 height = 0;
    QSSGRenderTextureFormat format = QSSGRenderTextureFormat::Unknown;
    QSSGOffscreenRendererDepthValues depth = QSSGOffscreenRendererDepthValues::NoDepthBuffer;
    bool stencil = false;
    QSSGRenderLayer::AAMode msaaMode = QSSGRenderLayer::AAMode::NoAA;

    QSSGOffscreenRendererEnvironment() = default;

    QSSGOffscreenRendererEnvironment(qint32 inWidth, qint32 inHeight, QSSGRenderTextureFormat inFormat)
        : width(inWidth)
        , height(inHeight)
        , format(inFormat)
        , depth(QSSGOffscreenRendererDepthValues::Depth16)
    {
    }

    QSSGOffscreenRendererEnvironment(qint32 inWidth,
                                       qint32 inHeight,
                                       QSSGRenderTextureFormat inFormat,
                                       QSSGOffscreenRendererDepthValues inDepth,
                                       bool inStencil,
                                       QSSGRenderLayer::AAMode inAAMode)
        : width(inWidth), height(inHeight), format(inFormat), depth(inDepth), stencil(inStencil), msaaMode(inAAMode)
    {
    }
};

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGOffscreenRenderFlags
{
    bool hasTransparency = false;
    bool hasChangedSinceLastFrame = false;
    constexpr QSSGOffscreenRenderFlags() = default;
    constexpr QSSGOffscreenRenderFlags(bool transparency, bool hasChanged)
        : hasTransparency(transparency), hasChangedSinceLastFrame(hasChanged)
    {
    }
};

typedef void *QSSGRenderInstanceId;

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGOffscreenRendererInterface
{
public:
    QAtomicInt ref;

    virtual ~QSSGOffscreenRendererInterface();

public:
    // Arbitrary const char* returned to indicate the type of this renderer
    // Can be overloaded to form the basis of an RTTI type system.
    // Not currently used by the rendering system.
    virtual QString getOffscreenRendererType() = 0;
    virtual QSSGOffscreenRendererEnvironment getDesiredEnvironment(QVector2D inPresentationScaleFactor) = 0;
    // Returns true of this object needs to be rendered, false if this object is not dirty
    virtual QSSGOffscreenRenderFlags needsRender(const QSSGOffscreenRendererEnvironment &inEnvironment,
                                                   QVector2D inPresentationScaleFactor,
                                                   const QSSGRenderInstanceId instanceId) = 0;
    // Returns true if the rendered result image has transparency, or false
    // if it should be treated as a completely opaque image.
    // It is the IOffscreenRenderer's job to clear any buffers (color, depth, stencil) that it
    // needs to.  It should not assume that it's buffers are clear;
    // Sometimes we scale the width and height of the main presentation in order to fit a
    // window.
    // If we do so, the scale factor tells the subpresentation renderer how much the system has
    // scaled.
    virtual void render(const QSSGOffscreenRendererEnvironment &inEnvironment,
                        QSSGRenderContext &inRenderContext,
                        QVector2D inPresentationScaleFactor,
                        const QSSGRenderInstanceId instanceId) = 0;
    virtual void renderWithClear(const QSSGOffscreenRendererEnvironment &inEnvironment,
                                 QSSGRenderContext &inRenderContext,
                                 QVector2D inPresentationScaleFactor,
                                 QVector3D inclearColor,
                                 const QSSGRenderInstanceId instanceId) = 0;

    // Implementors should implement one of the two interfaces below.

    // If this renderer supports picking that can return graph objects
    // then return an interface here.
    virtual QSSGGraphObjectPickQueryInterface *getGraphObjectPickQuery(const QSSGRenderInstanceId instanceId) = 0;

    // If you *don't* support the GraphObjectPickIterator interface, then you should implement
    // this interface
    // The system will just ask you to pick.
    // If you return true, then we will assume that you swallowed the pick and will continue no
    // further.
    // else we will assume you did not and will continue the picking algorithm.
    virtual bool pick(const QVector2D &inMouseCoords, const QVector2D &inViewportDimensions, const QSSGRenderInstanceId instanceId) = 0;
};

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGOffscreenRenderResult
{
    QSSGRef<QSSGOffscreenRendererInterface> renderer;
    QSSGRef<QSSGRenderTexture2D> texture;
    bool hasTransparency = false;
    bool hasChangedSinceLastFrame = false;

    QSSGOffscreenRenderResult(QSSGRef<QSSGOffscreenRendererInterface> inRenderer,
                                QSSGRef<QSSGRenderTexture2D> inTexture,
                                bool inTrans,
                                bool inDirty)
        : renderer(inRenderer), texture(inTexture), hasTransparency(inTrans), hasChangedSinceLastFrame(inDirty)
    {
    }
    QSSGOffscreenRenderResult() = default;
};

struct QSSGOffscreenRendererKey;
struct QSSGRendererData;

/**
 *	The offscreen render manager attempts to satisfy requests for a given image under a given key.
 *	Renderers are throttled such that they render at most once per frame and potentially less than
 *	that if they don't require a new render.
 */
class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGOffscreenRenderManager
{
    Q_DISABLE_COPY(QSSGOffscreenRenderManager)
public:
    QAtomicInt ref;
private:
    typedef QHash<QSSGOffscreenRendererKey, QSSGRendererData> TRendererMap;
    QSSGRenderContextInterface *m_context;
    QSSGRef<QSSGResourceManager> m_resourceManager;
    TRendererMap m_renderers;
    quint32 m_frameCount; // cheap per-

public:
    QSSGOffscreenRenderManager(const QSSGRef<QSSGResourceManager> &inManager, QSSGRenderContextInterface *inContext);
    ~QSSGOffscreenRenderManager();
    // returns true if the renderer has not been registered.
    // No return value means there was an error registering this id.
    QSSGOption<bool> maybeRegisterOffscreenRenderer(const QSSGOffscreenRendererKey &inKey,
                                                              const QSSGRef<QSSGOffscreenRendererInterface> &inRenderer);
    void registerOffscreenRenderer(const QSSGOffscreenRendererKey &inKey,
                                           const QSSGRef<QSSGOffscreenRendererInterface> &inRenderer);
    bool hasOffscreenRenderer(const QSSGOffscreenRendererKey &inKey);
    QSSGRef<QSSGOffscreenRendererInterface> getOffscreenRenderer(const QSSGOffscreenRendererKey &inKey);
    void releaseOffscreenRenderer(const QSSGOffscreenRendererKey &inKey);

    // This doesn't trigger rendering right away.  A node is added to the render graph that
    // points to this item.
    // Thus rendering is deffered until the graph is run but we promise to render to this
    // resource.
    QSSGOffscreenRenderResult getRenderedItem(const QSSGOffscreenRendererKey &inKey);
    // Called by the UICRenderContext, clients don't need to call this.
    void beginFrame();
    void endFrame();

    void renderItem(QSSGRendererData &theData, QSSGOffscreenRendererEnvironment theDesiredEnvironment);

    static QSSGRef<QSSGOffscreenRenderManager> createOffscreenRenderManager(const QSSGRef<QSSGResourceManager> &inManager,
                                                                                         QSSGRenderContextInterface *inContext);
};

QT_END_NAMESPACE
#endif
