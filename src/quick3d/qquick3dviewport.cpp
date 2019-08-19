/****************************************************************************
**
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

#include "qquick3dviewport_p.h"
#include "qquick3dsceneenvironment_p.h"
#include "qquick3dobject_p_p.h"
#include "qquick3dscenemanager_p.h"
#include "qquick3dtexture_p.h"
#include "qquick3dscenerenderer_p.h"
#include "qquick3dcamera_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QOpenGLFunctions>

#include <qsgtextureprovider.h>
#include <QSGSimpleTextureNode>
#include <QSGRendererInterface>
#include <QQuickWindow>
#include <QtQuick/private/qquickitem_p.h>

#include <QtQml>

QT_BEGIN_NAMESPACE

/*!
    \qmltype View3D
    \inherits QQuickItem
    \instantiates QQuick3DViewport
    \inqmlmodule QtQuick3D
    \brief Provides a viewport on which to render a 3D scene.

    View3D provides a 2D surface for 3D content to be rendered to. Before 3D
    content can be displayed in a Qt Quick scene, it must first be flattend.

    There are two ways to define a 3D scene for View3D to view.  The first
    and easiest is to just define a higharchy of QtQuick3D::Node based items as
    children of the View3D.  This becomes the implicit scene of the viewport.

    It is also possible to reference an existing scene by using the
    View3D::scene property to set the root of the scene you want to render.
    This scene does not have to exist as a child of the current View3D.

    There is also a combination approach where you define both a scene with
    children nodes, as well as define a scene that is being referenced. In this
    case you can treat the referenced scene as a sibling of the child scene.

    This is demonstrated in {View3D example}

    To control how a scene is rendered, it is necessary to define a
    SceneEnvironment to the View3D::environment property.

    To project a 3D scene to a 2D viewport, it is necessary to view the scene
    from a camera. If a scene has more than one camera it is possible to set
    which camera is used to render the scene to this viewport by setting the
    View3D::Camera property.

    It is also possible to define where the 3D scene is rendered to using
    the View3D::renderMode property. This can be necessary for performance
    reasons on certain platforms where it is expensive to render to
    intermediate offscreen surfaces.  There are certain tradeoffs to rendering
    directly to the window though, so this is not the default.
*/

QQuick3DViewport::QQuick3DViewport(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents);
    m_camera = nullptr;
    m_sceneRoot = new QQuick3DNode();
    m_environment = new QQuick3DSceneEnvironment(m_sceneRoot);
    QQuick3DObjectPrivate::get(m_sceneRoot)->sceneManager = new QQuick3DSceneManager(m_sceneRoot);
    connect(QQuick3DObjectPrivate::get(m_sceneRoot)->sceneManager, &QQuick3DSceneManager::needsUpdate,
            this, &QQuickItem::update);
}

QQuick3DViewport::~QQuick3DViewport()
{
    for (const auto &connection : qAsConst(m_connections))
        disconnect(connection);
}

static void ssgn_append(QQmlListProperty<QObject> *property, QObject *obj)
{
    if (!obj)
        return;
    QQuick3DViewport *view3d = static_cast<QQuick3DViewport *>(property->object);
    QQmlListProperty<QObject> itemProperty = QQuick3DObjectPrivate::get(view3d->scene())->data();
    itemProperty.append(&itemProperty, obj);
}

static int ssgn_count(QQmlListProperty<QObject> *property)
{
    QQuick3DViewport *view3d = static_cast<QQuick3DViewport *>(property->object);
    if (!view3d || !view3d->scene() || !QQuick3DObjectPrivate::get(view3d->scene())->data().count)
        return 0;
    QQmlListProperty<QObject> itemProperty = QQuick3DObjectPrivate::get(view3d->scene())->data();
    return itemProperty.count(&itemProperty);
}

static QObject *ssgn_at(QQmlListProperty<QObject> *property, int i)
{
    QQuick3DViewport *view3d = static_cast<QQuick3DViewport *>(property->object);
    QQmlListProperty<QObject> itemProperty = QQuick3DObjectPrivate::get(view3d->scene())->data();
    return itemProperty.at(&itemProperty, i);
}

static void ssgn_clear(QQmlListProperty<QObject> *property)
{
    QQuick3DViewport *view3d = static_cast<QQuick3DViewport *>(property->object);
    QQmlListProperty<QObject> itemProperty = QQuick3DObjectPrivate::get(view3d->scene())->data();
    return itemProperty.clear(&itemProperty);
}


QQmlListProperty<QObject> QQuick3DViewport::data()
{
    return QQmlListProperty<QObject>(this,
                                     nullptr,
                                     ssgn_append,
                                     ssgn_count,
                                     ssgn_at,
                                     ssgn_clear);
}

/*!
    \qmlproperty QtQuick3D::Camera QtQuick3D::View3D::camera

    This property specifies which camera is used to render the scene. It is
    possible for this value to be undefined, in which case the first enabled
    camera in the scene will be used instead.

    If it is desired to not render anything in the scene, then make sure all
    cameras are disabled.

    \sa QtQuick3D::Camera
*/
QQuick3DCamera *QQuick3DViewport::camera() const
{
    return m_camera;
}


/*!
    \qmlproperty QtQuick3D::SceneEnvironment QtQuick3D::View3D::environment

    This property specifies the SceneEnvironment used to render the scene.

    \sa QtQuick3D::SceneEnvironment
*/
QQuick3DSceneEnvironment *QQuick3DViewport::environment() const
{
    return m_environment;
}

/*!
    \qmlproperty QtQuick3D::Node QtQuick3D::View3D::scene

    This property defines the root nood of the scene to render to the
    viewport.

    \sa QtQuick3D::Node
*/
QQuick3DNode *QQuick3DViewport::scene() const
{
    return m_sceneRoot;
}

QQuick3DNode *QQuick3DViewport::referencedScene() const
{
    return m_referencedScene;
}

/*!
    \qmlproperty enumeration QtQuick3D::View3D::renderMode

    This property determines how the scene is rendered to the viewport.

    \table
    \header \li Display \li Result
    \row \li \c View3D.Texture \li Scene is rendered to a texture. Comes with no limitations.
    \row \li \c View3D.Underlay \li Scene is rendered directly to the window before Qt Quick is rendered.
    \row \li \c View3D.Overlay \li Scene is rendered directly to the window after Qt Quick is rendered.
    \row \li \c View3D.RenderNode \li Scene is rendered to the current render target using QSGRenderNode.
    \endtable

    The default mode is \c View3D.Texture as this is the offers the best compatiblity.
*/
QQuick3DViewport::QQuick3DViewportRenderMode QQuick3DViewport::renderMode() const
{
    return m_renderMode;
}

QQuick3DSceneRenderer *QQuick3DViewport::createRenderer() const
{
    return new QQuick3DSceneRenderer(this->window());
}

bool QQuick3DViewport::isTextureProvider() const
{
    // We can only be a texture provider if we are rendering to a texture first
    if (m_renderMode == QQuick3DViewport::Texture)
        return true;

    return false;
}

QSGTextureProvider *QQuick3DViewport::textureProvider() const
{
    // When Item::layer::enabled == true, QQuickItem will be a texture
    // provider. In this case we should prefer to return the layer rather
    // than the fbo texture.
    if (QQuickItem::isTextureProvider())
        return QQuickItem::textureProvider();

    // We can only be a texture provider if we are rendering to a texture first
    if (m_renderMode != QQuick3DViewport::Texture)
        return nullptr;

    QQuickWindow *w = window();
    if (!w || !w->openglContext() || QThread::currentThread() != w->openglContext()->thread()) {
        qWarning("QSSGView3D::textureProvider: can only be queried on the rendering thread of an exposed window");
        return nullptr;
    }
    if (!m_node)
        m_node = new SGFramebufferObjectNode;
    return m_node;
}

void QQuick3DViewport::releaseResources()
{
    m_node = nullptr;
}

void QQuick3DViewport::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChanged(newGeometry, oldGeometry);

    if (newGeometry.size() != oldGeometry.size())
        update();
}

QSGNode *QQuick3DViewport::updatePaintNode(QSGNode *node, QQuickItem::UpdatePaintNodeData *)
{
    // When changing render modes
    if (m_renderModeDirty) {
        if (node) {
            delete node;
            node = nullptr;
        }
        if (m_directRenderer) {
            delete m_directRenderer;
            m_directRenderer = nullptr;
        }
    }


    m_renderModeDirty = false;

    if (m_renderMode == Texture) {
        SGFramebufferObjectNode *n = static_cast<SGFramebufferObjectNode *>(node);

        if (!n) {
            if (!m_node)
                m_node = new SGFramebufferObjectNode;
            n = m_node;
        }

        if (!n->renderer) {
            n->window = window();
            n->renderer = createRenderer();
            n->renderer->data = n;
            n->quickFbo = this;
            connect(window(), SIGNAL(screenChanged(QScreen*)), n, SLOT(handleScreenChange()));
        }
        QSize minFboSize = QQuickItemPrivate::get(this)->sceneGraphContext()->minimumFBOSize();
        QSize desiredFboSize(qMax<int>(minFboSize.width(), width()),
                             qMax<int>(minFboSize.height(), height()));

        n->devicePixelRatio = window()->effectiveDevicePixelRatio();
        desiredFboSize *= n->devicePixelRatio;

        n->renderer->synchronize(this, desiredFboSize);

        // Update QSGDynamicTextures that are used for source textures
        // TODO: could be optimized to not update textures that aren't used or are on culled
        // geometry.
        auto *sceneManager = QQuick3DObjectPrivate::get(m_sceneRoot)->sceneManager;
        for (auto *texture : qAsConst(sceneManager->qsgDynamicTextures)) {
            texture->updateTexture();
        }

        n->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
        n->setFiltering(smooth() ? QSGTexture::Linear : QSGTexture::Nearest);
        n->setRect(0, 0, width(), height());

        n->scheduleRender();

        return n;
    } else if (m_renderMode == Underlay) {
        if (!m_directRenderer)
            m_directRenderer = new QQuick3DSGDirectRenderer(createRenderer(), window(), QQuick3DSGDirectRenderer::Underlay);
        const QSizeF targetSize = window()->effectiveDevicePixelRatio() * QSizeF(width(), height());
        m_directRenderer->renderer()->synchronize(this, targetSize.toSize(), false);
        m_directRenderer->setViewport(QRectF(window()->effectiveDevicePixelRatio() * mapToScene(QPointF(0, 0)), targetSize));
        m_directRenderer->requestRender();
        if (window()->clearBeforeRendering())
            window()->setClearBeforeRendering(false);
        window()->update();
        return node; // node should be nullptr
    } else if (m_renderMode == Overlay) {
        if (!m_directRenderer)
            m_directRenderer = new QQuick3DSGDirectRenderer(createRenderer(), window(), QQuick3DSGDirectRenderer::Overlay);
        const QSizeF targetSize = window()->effectiveDevicePixelRatio() * QSizeF(width(), height());
        m_directRenderer->renderer()->synchronize(this, targetSize.toSize(), false);
        m_directRenderer->setViewport(QRectF(window()->effectiveDevicePixelRatio() * mapToScene(QPointF(0, 0)), targetSize));
        m_directRenderer->requestRender();
        return node; // node should be nullptr
    } else {
        // Render Node
        QQuick3DSGRenderNode *n = static_cast<QQuick3DSGRenderNode *>(node);
        if (!n) {
            if (!m_renderNode)
                m_renderNode = new QQuick3DSGRenderNode();
            n = m_renderNode;
        }

        if (!n->renderer) {
            n->window = window();
            n->renderer = createRenderer();
            n->renderer->data = n;
        }

        const QSize targetSize = window()->effectiveDevicePixelRatio() * QSize(width(), height());

        n->renderer->synchronize(this, targetSize, false);
        n->markDirty(QSGNode::DirtyMaterial);

        return n;
    }
}

void QQuick3DViewport::setCamera(QQuick3DCamera *camera)
{
    if (m_camera == camera)
        return;

    m_camera = camera;
    emit cameraChanged(m_camera);
    update();
}

void QQuick3DViewport::setEnvironment(QQuick3DSceneEnvironment *environment)
{
    if (m_environment == environment)
        return;

    m_environment = environment;
    if (!m_environment->parentItem())
        m_environment->setParentItem(m_sceneRoot);
    emit environmentChanged(m_environment);
    update();
}

void QQuick3DViewport::setScene(QQuick3DNode *sceneRoot)
{
    // ### We may need consider the case where there is
    // already a scene tree here
    if (m_referencedScene) {
        // if there was previously a reference scene, disconnect
        if (!QQuick3DObjectPrivate::get(m_referencedScene)->sceneManager)
            disconnect(QQuick3DObjectPrivate::get(m_referencedScene)->sceneManager, &QQuick3DSceneManager::needsUpdate, this, &QQuickItem::update);
    }
    m_referencedScene = sceneRoot;
    if (m_referencedScene) {
        // If the referenced scene doesn't have a manager, add one (scenes defined outside of an view3d)
        auto privateObject = QQuick3DObjectPrivate::get(m_referencedScene);
        // ### BUG: This will probably leak, need to think harder about this
        if (!privateObject->sceneManager)
            privateObject->refSceneRenderer(new QQuick3DSceneManager(m_referencedScene));
        connect(QQuick3DObjectPrivate::get(m_referencedScene)->sceneManager, &QQuick3DSceneManager::needsUpdate, this, &QQuickItem::update);
    }

}

void QQuick3DViewport::setRenderMode(QQuick3DViewport::QQuick3DViewportRenderMode renderMode)
{
    if (m_renderMode == renderMode)
        return;

    m_renderMode = renderMode;
    m_renderModeDirty = true;
    emit renderModeChanged(m_renderMode);
    update();
}

void QQuick3DViewport::setEnableWireframeMode(bool enableWireframeMode)
{
    if (m_enableWireframeMode == enableWireframeMode)
        return;

    m_enableWireframeMode = enableWireframeMode;
    emit enableWireframeModeChanged(m_enableWireframeMode);
    update();
}

static QSurfaceFormat findIdealGLVersion()
{
    QSurfaceFormat fmt;
    fmt.setProfile(QSurfaceFormat::CoreProfile);

    // Advanced: Try 4.3 core (so we get compute shaders for instance)
    fmt.setVersion(4, 3);
    QOpenGLContext ctx;
    ctx.setFormat(fmt);
    if (ctx.create() && ctx.format().version() >= qMakePair(4, 3)) {
        qDebug("Requesting OpenGL 4.3 core context succeeded");
        return ctx.format();
    }

    // Basic: Stick with 3.3 for now to keep less fortunate, Mesa-based systems happy
    fmt.setVersion(3, 3);
    ctx.setFormat(fmt);
    if (ctx.create() && ctx.format().version() >= qMakePair(3, 3)) {
        qDebug("Requesting OpenGL 3.3 core context succeeded");
        return ctx.format();
    }

    qDebug("Impending doom");
    return fmt;
}

static bool isBlackListedES3Driver(QOpenGLContext &ctx) {
    auto glFunctions = ctx.functions();
    static bool hasBeenTested = false;
    static bool result = false;
    if (!hasBeenTested) {
        QOffscreenSurface offscreenSurface;
        offscreenSurface.create();
        ctx.makeCurrent(&offscreenSurface);
        QString vendorString = QString::fromLatin1(reinterpret_cast<const char *>(glFunctions->glGetString(GL_RENDERER)));
        ctx.doneCurrent();
        if (vendorString == QStringLiteral("PowerVR Rogue GE8300"))
            result = true;
        hasBeenTested = true;
    }
    return result;
}


static QSurfaceFormat findIdealGLESVersion()
{
    QSurfaceFormat fmt;

    // Advanced: Try 3.1 (so we get compute shaders for instance)
    fmt.setVersion(3, 1);
    fmt.setRenderableType(QSurfaceFormat::OpenGLES);
    QOpenGLContext ctx;
    ctx.setFormat(fmt);

    // Now, it's important to check the format with the actual version (parsed
    // back from GL_VERSION) since some implementations, ANGLE for instance,
    // are broken and succeed the 3.1 context request even though they only
    // support and return a 3.0 context. This is against the spec since 3.0 is
    // obviously not backwards compatible with 3.1, but hey...
    if (ctx.create() && ctx.format().version() >= qMakePair(3, 1)) {
        qDebug("Requesting OpenGL ES 3.1 context succeeded");
        return ctx.format();
    }

    // Basic: OpenGL ES 3.0 is a hard requirement at the moment since we can
    // only generate 300 es shaders, uniform buffers are mandatory.
    fmt.setVersion(3, 0);
    ctx.setFormat(fmt);
    if (ctx.create() && ctx.format().version() >= qMakePair(3, 0) && !isBlackListedES3Driver(ctx)) {
        qDebug("Requesting OpenGL ES 3.0 context succeeded");
        return ctx.format();
    }

    fmt.setVersion(2, 0);
    ctx.setFormat(fmt);
    if (ctx.create()) {
        qDebug("Requesting OpenGL ES 2.0 context succeeded");
        return fmt;
    }

    qDebug("Impending doom");
    return fmt;
}

QSurfaceFormat QQuick3DViewport::idealSurfaceFormat()
{
    static const QSurfaceFormat f = [] {
        QSurfaceFormat fmt;
        if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL) { // works in dynamic gl builds too because there's a qguiapp already
            fmt = findIdealGLVersion();
        } else {
            fmt = findIdealGLESVersion();
        }
        fmt.setDepthBufferSize(24);
        fmt.setStencilBufferSize(8);
        // Ignore MSAA here as that is a per-layer setting.
        return fmt;
    }();
    return f;
}

/*!
 * Transforms \a worldPos from world space into view space. The returned x-, and y values
 * will be be in view coordinates. The returned z value will contain the distance from the
 * back of the frustum (clipNear) to \a worldPos.
 * If \a worldPos cannot be mapped to a position in the world, a position of [0, 0, 0] is
 * returned. This function requires that a camera is assigned to the view.
 *
 * \sa QQuick3DCamera::worldToViewport QQuick3DViewport::viewToWorld
 */
QVector3D QQuick3DViewport::worldToView(const QVector3D &worldPos) const
{
    if (!m_camera) {
        qmlWarning(this) << "Cannot resolve view position without a camera assigned!";
        return QVector3D(-1, -1, -1);
    }

    const QVector3D normalizedPos = m_camera->worldToViewport(worldPos);
    if (normalizedPos.x() < 0)
        return normalizedPos;
    return normalizedPos * QVector3D(float(width()), float(height()), 1);
}

/*!
 * Transforms \a viewPos from view space into world space. The x-, and y values of
 * \l viewPos should be in view coordinates. The z value should be
 * the distance from the back of the frustum (clipNear) into the world. If \a viewPos
 * cannot be mapped to a position in the world, a position of [0, 0, 0] is returned.
 *
 * \sa QQuick3DCamera::viewportToWorld QQuick3DViewport::worldToView
 */
QVector3D QQuick3DViewport::viewToWorld(const QVector3D &viewPos) const
{
    if (!m_camera) {
        qmlWarning(this) << "Cannot resolve world position without a camera assigned!";
        return QVector3D(-1, -1, -1);
    }

    const QVector3D normalizedPos = viewPos / QVector3D(float(width()), float(height()), 1);
    return m_camera->viewportToWorld(normalizedPos);
}

QQuick3DPickResult QQuick3DViewport::pick(float x, float y) const
{
    const QPointF position(x * window()->effectiveDevicePixelRatio(), y * window()->effectiveDevicePixelRatio());
    // Some non-thread safe stuff to do input
    // First need to get a handle to the renderer

    QQuick3DSceneRenderer *renderer = getRenderer();
    if (renderer) {
        return renderer->pick(position);
    }
    return QQuick3DPickResult();
}

bool QQuick3DViewport::enableWireframeMode() const
{
    return m_enableWireframeMode;
}

void QQuick3DViewport::invalidateSceneGraph()
{
    m_node = nullptr;
}

QQuick3DSceneRenderer *QQuick3DViewport::getRenderer() const
{
    QQuick3DSceneRenderer *renderer = nullptr;
    if (m_node) {
        renderer = m_node->renderer;
    } else if (m_renderNode) {
        renderer = m_renderNode->renderer;
    } else if (m_directRenderer) {
        renderer = m_directRenderer->renderer();
    }
    return renderer;
}

QT_END_NAMESPACE
