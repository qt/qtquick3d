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
#include "qquick3dobject_p.h"
#include "qquick3dscenemanager_p.h"
#include "qquick3dtexture_p.h"
#include "qquick3dscenerenderer_p.h"
#include "qquick3dscenerootnode_p.h"
#include "qquick3dcamera_p.h"
#include "qquick3dmodel_p.h"
#include "qquick3drenderstats_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>

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
    \inqmlmodule QtQuick3D
    \brief Provides a viewport on which to render a 3D scene.

    View3D provides a 2D surface on which a 3D scene can be rendered. This
    surface is a Qt Quick \l Item and can be placed in a Qt Quick scene.

    There are two ways to define the 3D scene that is visualized on the View3D:
    If you define a hierarchy of \l{Node}{Node-based} items as children of
    the View3D directly, then this will become the implicit scene of the View3D.

    It is also possible to reference an existing scene by using the \l importScene
    property and setting it to the root \l Node of the scene you want to visualize.
    This \l Node does not have to be an ancestor of the View3D, and you can have
    multiple View3Ds that import the same scene.

    This is demonstrated in \l {Qt Quick 3D - View3D example}{View3D example}.

    If the View3D both has child \l{Node}{Nodes} and the \l importScene property is
    set simultaneously, then both scenes will be rendered as if they were sibling
    subtrees in the same scene.

    To control how a scene is rendered, you can set the \l environment property.
    The type \l SceneEnvironment has a number of visual properties that can be
    adjusted, such as background color, tone mapping, anti-aliasing and more.

    In addition, in order for anything to be rendered in the View3D, the scene
    needs a \l Camera. If there is only a single \l Camera in the scene, then
    this will automatically be picked. Otherwise, the \l camera property can
    be used to select the camera. The \l Camera decides which parts of the scene
    are visible, and how they are projected onto the 2D surface.

    By default, the 3D scene will first be rendered into an off-screen buffer and
    then composited with the rest of the Qt Quick scene when it is done. This provides
    the maximum level of compatibility, but may have performance implications on some
    graphics hardware. If this is the case, the \l renderMode property can be used to
    switch how the View3D is rendered into the window.

    \sa {Qt Quick 3D - View3D example}
*/

QQuick3DViewport::QQuick3DViewport(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents);
    m_camera = nullptr;
    m_sceneRoot = new QQuick3DSceneRootNode(this);
    m_environment = new QQuick3DSceneEnvironment(m_sceneRoot);
    m_renderStats = new QQuick3DRenderStats(m_sceneRoot);
    QQuick3DSceneManager *sceneManager = new QQuick3DSceneManager(m_sceneRoot);
    QQuick3DObjectPrivate::get(m_sceneRoot)->refSceneManager(*sceneManager);
    Q_ASSERT(sceneManager == QQuick3DObjectPrivate::get(m_sceneRoot)->sceneManager);
    connect(sceneManager, &QQuick3DSceneManager::needsUpdate,
            this, &QQuickItem::update);
}

QQuick3DViewport::~QQuick3DViewport()
{
    for (const auto &connection : qAsConst(m_connections))
        disconnect(connection);
    auto sceneManager = QQuick3DObjectPrivate::get(m_sceneRoot)->sceneManager;
    if (sceneManager)
        sceneManager->setParent(nullptr);

    delete m_sceneRoot;
    m_sceneRoot = nullptr;

    // We need to defere the deletion of the scene manager or items might
    // try to deref using the now destroyed scene manaager.
    connect(this, &QObject::destroyed, [sceneManager](QObject *) { delete sceneManager; });
    sceneManager = nullptr;

    // m_directRenderer must be destroyed on the render thread at the proper time, not here.
    // That's handled in releaseResources() + upon sceneGraphInvalidated
}

static void ssgn_append(QQmlListProperty<QObject> *property, QObject *obj)
{
    if (!obj)
        return;
    QQuick3DViewport *view3d = static_cast<QQuick3DViewport *>(property->object);

    if (QQuick3DObject *sceneObject = qmlobject_cast<QQuick3DObject *>(obj)) {
        QQmlListProperty<QObject> itemProperty = QQuick3DObjectPrivate::get(view3d->scene())->data();
        itemProperty.append(&itemProperty, sceneObject);
    } else if (QQuickItem *item = qmlobject_cast<QQuickItem *>(obj)) {
        // TODO: Should probably also setup the rest of the methods for this case
        item->setParentItem(view3d);
    }
}

static qsizetype ssgn_count(QQmlListProperty<QObject> *property)
{
    QQuick3DViewport *view3d = static_cast<QQuick3DViewport *>(property->object);
    if (!view3d || !view3d->scene() || !QQuick3DObjectPrivate::get(view3d->scene())->data().count)
        return 0;
    QQmlListProperty<QObject> itemProperty = QQuick3DObjectPrivate::get(view3d->scene())->data();
    return itemProperty.count(&itemProperty);
}

static QObject *ssgn_at(QQmlListProperty<QObject> *property, qsizetype i)
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

    This property specifies which \l Camera is used to render the scene. If this
    property is not set, then the first enabled camera in the scene will be used.

    \sa PerspectiveCamera, OrthographicCamera, FrustumCamera, CustomCamera
*/
QQuick3DCamera *QQuick3DViewport::camera() const
{
    return m_camera;
}

/*!
    \qmlproperty QtQuick3D::SceneEnvironment QtQuick3D::View3D::environment

    This property specifies the SceneEnvironment used to render the scene.

    \sa SceneEnvironment
*/
QQuick3DSceneEnvironment *QQuick3DViewport::environment() const
{
    return m_environment;
}

/*!
    \qmlproperty QtQuick3D::Node QtQuick3D::View3D::scene
    \readonly

    Holds the root \l Node of the View3D's scene.

    \sa importScene
*/
QQuick3DNode *QQuick3DViewport::scene() const
{
    return m_sceneRoot;
}

/*!
    \qmlproperty QtQuick3D::Node QtQuick3D::View3D::importScene

    This property defines the reference node of the scene to render to the viewport.
    The node does not have to be a child of the View3D. This referenced node becomes
    a sibling with child nodes of View3D, if there are any.

    \note This property can only be set once, and subsequent changes will have no
    effect.

    \sa Node
*/
QQuick3DNode *QQuick3DViewport::importScene() const
{
    return m_importScene;
}

/*!
    \qmlproperty enumeration QtQuick3D::View3D::renderMode

    This property determines how the View3D is combined with the other parts of the
    Qt Quick scene.

    By default, the scene will be rendered into an off-screen buffer as an intermediate
    step. This off-screen buffer is then rendered into the window (or render target) like any
    other Qt Quick \l Item.

    For most users, there will be no need to change the render mode, and this property can
    safely be ignored. But on some graphics hardware, the use of an off-screen buffer can be
    a performance bottleneck. If this is the case, it might be worth experimenting with other
    modes.

    \value View3D.Offscreen The scene is rendered into an off-screen buffer as an intermediate
    step. This off-screen buffer is then composited with the rest of the Qt Quick scene.

    \value View3D.Underlay The scene is rendered directly to the window before the rest of
    the Qt Quick scene is rendered. With this mode, the View3D cannot be placed on top of
    other Qt Quick items.

    \value View3D.Overlay The scene is rendered directly to the window after Qt Quick is
    rendered.  With this mode, the View3D will always be on top of other Qt Quick items.

    \value View3D.Inline The View3D's scene graph is embedded into the main scene graph,
    and the same ordering semantics are applied as to any other Qt Quick \l Item. As this
    mode can lead to subtle issues, depending on the contents of the scene, due to
    injecting depth-based 3D content into a 2D scene graph, it is not recommended to be
    used, unless a specific need arises.

    The default is \c{View3D.Offscreen}.

    \note When changing the render mode, it is important to note that \c{View3D.Offscreen} (the
    default) is the only mode which guarantees perfect graphics fidelity. The other modes
    all have limitations that can cause visual glitches, so it is important to check that
    the visual output still looks correct when changing this property.

    \note When using the Underlay, Overlay, or Inline modes, it can be useful, and in some
    cases, required, to disable the Qt Quick scene graph's depth buffer writes via
    QQuickGraphicsConfiguration::setDepthBufferFor2D() before showing the QQuickWindow or
    QQuickView hosting the View3D item.
*/
QQuick3DViewport::RenderMode QQuick3DViewport::renderMode() const
{
    return m_renderMode;
}

/*!
    \qmlproperty QtQuick3D::RenderStats QtQuick3D::View3D::renderStats
    \readonly

    This property provides statistics about the rendering of a frame, such as
    \l {RenderStats::fps}{fps}, \l {RenderStats::frameTime}{frameTime},
    \l {RenderStats::renderTime}{renderTime}, \l {RenderStats::syncTime}{syncTime},
    and \l {RenderStats::maxFrameTime}{maxFrameTime}.
*/
QQuick3DRenderStats *QQuick3DViewport::renderStats() const
{
    return m_renderStats;
}

QQuick3DSceneRenderer *QQuick3DViewport::createRenderer() const
{
    return new QQuick3DSceneRenderer(this->window());
}

bool QQuick3DViewport::isTextureProvider() const
{
    // We can only be a texture provider if we are rendering to a texture first
    if (m_renderMode == QQuick3DViewport::Offscreen)
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
    if (m_renderMode != QQuick3DViewport::Offscreen)
        return nullptr;

    QQuickWindow *w = window();
    if (!w) {
        qWarning("QSSGView3D::textureProvider: can only be queried on the rendering thread of an exposed window");
        return nullptr;
    }

    if (!m_node)
        m_node = new SGFramebufferObjectNode;
    return m_node;
}

class CleanupJob : public QRunnable
{
public:
    CleanupJob(QQuick3DSGDirectRenderer *renderer) : m_renderer(renderer) { }
    void run() override { delete m_renderer; }
private:
    QQuick3DSGDirectRenderer *m_renderer;
};

void QQuick3DViewport::releaseResources()
{
    if (m_directRenderer) {
        window()->scheduleRenderJob(new CleanupJob(m_directRenderer), QQuickWindow::BeforeSynchronizingStage);
        m_directRenderer = nullptr;
    }

    m_node = nullptr;
}

void QQuick3DViewport::cleanupDirectRenderer()
{
    delete m_directRenderer;
    m_directRenderer = nullptr;
}

void QQuick3DViewport::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    if (newGeometry.size() != oldGeometry.size())
        update();
}

QSGNode *QQuick3DViewport::updatePaintNode(QSGNode *node, QQuickItem::UpdatePaintNodeData *)
{
    // ### Maybe don't check this every time, only when the window changes
    QSGRendererInterface *rif = window()->rendererInterface();
    const bool isRhi = QSGRendererInterface::isApiRhiBased(rif->graphicsApi());
    if (!isRhi) {
        // We only support using the RHI API, so bail out gracefully
        // ### maybe return a node tree that displays a warning that this isn't supported
        return nullptr;
    }

    // When changing render modes
    if (m_renderModeDirty) {
        if (node) {
            delete node;
            node = nullptr;
            m_node = nullptr;
            m_renderNode = nullptr;
        }
        if (m_directRenderer) {
            delete m_directRenderer;
            m_directRenderer = nullptr;
        }
    }

    m_renderModeDirty = false;

    if (m_renderMode == Offscreen) {
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

        n->setFiltering(smooth() ? QSGTexture::Linear : QSGTexture::Nearest);
        n->setRect(0, 0, width(), height());
        if (checkIsVisible() && isComponentComplete()) {
            n->renderer->synchronize(this, desiredFboSize, n->devicePixelRatio);
            if (n->renderer->m_textureNeedsFlip)
                n->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
            updateDynamicTextures();
            n->scheduleRender();
        }

        return n;
    } else if (m_renderMode == Underlay) {
        setupDirectRenderer(Underlay);
        return node; // node should be nullptr
    } else if (m_renderMode == Overlay) {
        setupDirectRenderer(Overlay);
        return node; // node should be nullptr
    } else if (m_renderMode == Inline) {
        // QSGRenderNode-based rendering
        QQuick3DSGRenderNode *n = static_cast<QQuick3DSGRenderNode *>(node);
        if (!n) {
            if (!m_renderNode)
                m_renderNode = new QQuick3DSGRenderNode;
            n = m_renderNode;
        }

        if (!n->renderer) {
            n->window = window();
            n->renderer = createRenderer();
            n->renderer->data = n;
        }

        const QSize targetSize = window()->effectiveDevicePixelRatio() * QSize(width(), height());

        if (isVisible() && isComponentComplete()) {
            n->renderer->synchronize(this, targetSize, window()->effectiveDevicePixelRatio(), false);
            updateDynamicTextures();
            n->markDirty(QSGNode::DirtyMaterial);
        }

        return n;
    } else {
        qWarning("Invalid renderMode %d", int(m_renderMode));
        return nullptr;
    }
}

void QQuick3DViewport::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    if (change == ItemSceneChange) {
        if (value.window) {
            // TODO: if we want to support multiple windows, there has to be a scene manager for
            // every window.
            QQuick3DObjectPrivate::get(m_sceneRoot)->sceneManager->setWindow(value.window);
            if (m_importScene)
                QQuick3DObjectPrivate::get(m_importScene)->sceneManager->setWindow(value.window);
        }
    }
}

void QQuick3DViewport::setCamera(QQuick3DCamera *camera)
{
    if (m_camera == camera)
        return;

    m_camera = camera;
    if (m_camera)
        m_camera->updateGlobalVariables(QRect(0, 0, width(), height()));
    emit cameraChanged();
    update();
}

void QQuick3DViewport::setEnvironment(QQuick3DSceneEnvironment *environment)
{
    if (m_environment == environment)
        return;

    m_environment = environment;
    if (m_environment && !m_environment->parentItem())
        m_environment->setParentItem(m_sceneRoot);
    emit environmentChanged();
    update();
}

void QQuick3DViewport::setImportScene(QQuick3DNode *inScene)
{
    // ### We may need consider the case where there is
    // already a scene tree here
    // FIXME : Only the first importScene is an effective one
    if (m_importScene)
        return;

    // FIXME : Check self-import or cross-import
    // Currently it does not work since importScene qml parsed in a reverse order.
    QQuick3DNode *scene = inScene;
    while (scene) {
        if (m_sceneRoot == scene) {
            qmlWarning(this) << "Cannot allow self-import or cross-import!";
            return;
        }

        QQuick3DSceneRootNode *rn = qobject_cast<QQuick3DSceneRootNode *>(scene);
        scene = rn ? rn->view3D()->importScene() : nullptr;
    }

    m_importScene = inScene;
    if (m_importScene) {
        // If the referenced scene doesn't have a manager use the one from the sceen root (scenes defined outside of an view3d)
        auto privateObject = QQuick3DObjectPrivate::get(m_importScene);
        if (!privateObject->sceneManager) {
            if (QQuick3DSceneManager *manager = QQuick3DObjectPrivate::get(m_sceneRoot)->sceneManager) {
                manager->setWindow(window());
                privateObject->refSceneManager(*manager);
            }
            Q_ASSERT(privateObject->sceneManager);
        }

        connect(privateObject->sceneManager, &QQuick3DSceneManager::needsUpdate,
                this, &QQuickItem::update);

        QQuick3DNode *scene = inScene;
        while (scene) {
            QQuick3DSceneRootNode *rn = qobject_cast<QQuick3DSceneRootNode *>(scene);
            scene = rn ? rn->view3D()->importScene() : nullptr;

            if (scene) {
                connect(QQuick3DObjectPrivate::get(scene)->sceneManager,
                        &QQuick3DSceneManager::needsUpdate,
                        this, &QQuickItem::update);
            }
        }
    }

    emit importSceneChanged();
    update();
}

void QQuick3DViewport::setRenderMode(QQuick3DViewport::RenderMode renderMode)
{
    if (m_renderMode == renderMode)
        return;

    m_renderMode = renderMode;
    m_renderModeDirty = true;
    emit renderModeChanged();
    update();
}


/*!
    \qmlmethod vector3d View3D::mapFrom3DScene(vector3d scenePos)

    Transforms \a scenePos from scene space (3D) into view space (2D).

    The returned x- and y-values will be be in view coordinates, with the top-left
    corner at [0, 0] and the bottom-right corner at [width, height]. The returned
    z-value contains the distance from the near clip plane of the frustum (clipNear) to
    \a scenePos in scene coordinates. If the distance is negative, that means the \a scenePos
    is behind the active camera. If \a scenePos cannot be mapped to a position in the scene,
    a position of [0, 0, 0] is returned.

    This function requires that \l camera is assigned to the view.

    \sa mapTo3DScene(), {Camera::mapToViewport()}{Camera.mapToViewport()}
*/
QVector3D QQuick3DViewport::mapFrom3DScene(const QVector3D &scenePos) const
{
    if (!m_camera) {
        qmlWarning(this) << "Cannot resolve view position without a camera assigned!";
        return QVector3D(0, 0, 0);
    }

    qreal _width = width();
    qreal _height = height();
    if (_width == 0 || _height == 0)
        return QVector3D(0, 0, 0);

    const QVector3D normalizedPos = m_camera->mapToViewport(scenePos, _width, _height);
    return normalizedPos * QVector3D(float(_width), float(_height), 1);
}

/*!
    \qmlmethod vector3d View3D::mapTo3DScene(vector3d viewPos)

    Transforms \a viewPos from view space (2D) into scene space (3D).

    The x- and y-values of \a viewPos should be in view coordinates, with the top-left
    corner at [0, 0] and the bottom-right corner at [width, height]. The z-value is
    interpreted as the distance from the near clip plane of the frustum (clipNear) in
    scene coordinates.

    If \a viewPos cannot successfully be mapped to a position in the scene, a position of
    [0, 0, 0] is returned.

    This function requires that a \l camera is assigned to the view.

    \sa mapFrom3DScene(), {Camera::mapFromViewport}{Camera.mapFromViewport()}
*/
QVector3D QQuick3DViewport::mapTo3DScene(const QVector3D &viewPos) const
{
    if (!m_camera) {
        qmlWarning(this) << "Cannot resolve scene position without a camera assigned!";
        return QVector3D(0, 0, 0);
    }

    qreal _width = width();
    qreal _height = height();
    if (_width == 0 || _height == 0)
        return QVector3D(0, 0, 0);

    const QVector3D normalizedPos = viewPos / QVector3D(float(_width), float(_height), 1);
    return m_camera->mapFromViewport(normalizedPos, _width, _height);
}

/*!
    \qmlmethod PickResult View3D::pick(float x, float y)

    This method will "shoot" a ray into the scene from view coordinates \a x and \a y
    and return information about the nearest intersection with an object in the scene.

    This can, for instance, be called with mouse coordinates to find the object under the mouse cursor.

    \note For an object with a custom \l Geometry, intersections will be approximated based on its bounding
    box.
*/
QQuick3DPickResult QQuick3DViewport::pick(float x, float y) const
{
    const QPointF position(qreal(x) * window()->effectiveDevicePixelRatio(), qreal(y) * window()->effectiveDevicePixelRatio());
    // Some non-thread safe stuff to do input
    // First need to get a handle to the renderer

    QQuick3DSceneRenderer *renderer = getRenderer();
    if (!renderer)
        return QQuick3DPickResult();

    auto pickResult = renderer->syncPick(position);
    if (!pickResult.m_hitObject)
        return QQuick3DPickResult();

    auto backendObject = pickResult.m_hitObject;
    const auto sceneManager = QQuick3DObjectPrivate::get(m_sceneRoot)->sceneManager;
    QQuick3DObject *frontendObject = sceneManager->lookUpNode(backendObject);

    // FIXME : for the case of consecutive importScenes
    if (!frontendObject && m_importScene) {
        const auto importSceneManager = QQuick3DObjectPrivate::get(m_importScene)->sceneManager;
        frontendObject = importSceneManager->lookUpNode(backendObject);
    }

    QQuick3DModel *model = qobject_cast<QQuick3DModel*>(frontendObject);
    if (!model)
        return QQuick3DPickResult();

    return QQuick3DPickResult(model,
                              ::sqrtf(pickResult.m_cameraDistanceSq),
                              pickResult.m_localUVCoords,
                              pickResult.m_scenePosition);
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

void QQuick3DViewport::updateDynamicTextures()
{
    // Update QSGDynamicTextures that are used for source textures and Quick items
    // Must be called on the render thread.

    const auto &sceneManager = QQuick3DObjectPrivate::get(m_sceneRoot)->sceneManager;
    for (auto *texture : qAsConst(sceneManager->qsgDynamicTextures))
        texture->updateTexture();

    QQuick3DNode *scene = m_importScene;
    while (scene) {
        const auto &importSm = QQuick3DObjectPrivate::get(scene)->sceneManager;
        if (importSm != sceneManager) {
            for (auto *texture : qAsConst(importSm->qsgDynamicTextures))
                texture->updateTexture();
        }

        // if importScene has another import
        QQuick3DSceneRootNode *rn = qobject_cast<QQuick3DSceneRootNode *>(scene);
        scene = rn ? rn->view3D()->importScene() : nullptr;
    }
}

void QQuick3DViewport::setupDirectRenderer(RenderMode mode)
{
    auto renderMode = (mode == Underlay) ? QQuick3DSGDirectRenderer::Underlay
                                         : QQuick3DSGDirectRenderer::Overlay;
    if (!m_directRenderer) {
        m_directRenderer = new QQuick3DSGDirectRenderer(createRenderer(), window(), renderMode);
        connect(window(), &QQuickWindow::sceneGraphInvalidated, this, &QQuick3DViewport::cleanupDirectRenderer, Qt::DirectConnection);
    }

    const QSizeF targetSize = window()->effectiveDevicePixelRatio() * QSizeF(width(), height());
    m_directRenderer->setViewport(QRectF(window()->effectiveDevicePixelRatio() * mapToScene(QPointF(0, 0)), targetSize));
    m_directRenderer->setVisibility(isVisible());
    if (isVisible()) {
        m_directRenderer->renderer()->synchronize(this, targetSize.toSize(), window()->effectiveDevicePixelRatio(), false);
        updateDynamicTextures();
        m_directRenderer->requestRender();
    }
}

// This is used for offscreen mode since we need to check if
// this item is used by an effect but hidden
bool QQuick3DViewport::checkIsVisible() const
{
    auto childPrivate = QQuickItemPrivate::get(this);
    return (childPrivate->explicitVisible ||
            (childPrivate->extra.isAllocated() && childPrivate->extra->effectRefCount));

}

QT_END_NAMESPACE
