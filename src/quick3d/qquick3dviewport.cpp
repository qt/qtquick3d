// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dviewport_p.h"
#include "qquick3dsceneenvironment_p.h"
#include "qquick3dscenemanager_p.h"
#include "qquick3dtexture_p.h"
#include "qquick3dscenerenderer_p.h"
#include "qquick3dscenerootnode_p.h"
#include "qquick3dcamera_p.h"
#include "qquick3dmodel_p.h"
#include "qquick3drenderstats_p.h"
#include "qquick3ditem2d_p.h"
#include "qquick3ddefaultmaterial_p.h"
#include "qquick3dprincipledmaterial_p.h"
#include "qquick3dcustommaterial_p.h"
#include "qquick3dspecularglossymaterial_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssglayerrenderdata_p.h>

#include <QtQuick3DUtils/private/qssgassert_p.h>

#include <qsgtextureprovider.h>
#include <QSGSimpleTextureNode>
#include <QSGRendererInterface>
#include <QQuickWindow>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickpointerhandler_p.h>

#include <QtQml>

#include <QtGui/private/qeventpoint_p.h>

#include <QtCore/private/qnumeric_p.h>

#include <optional>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcEv, "qt.quick3d.event")
Q_LOGGING_CATEGORY(lcPick, "qt.quick3d.pick")
Q_LOGGING_CATEGORY(lcHover, "qt.quick3d.hover")

static bool isforceInputHandlingSet()
{
    static const bool v = (qEnvironmentVariableIntValue("QT_QUICK3D_FORCE_INPUT_HANDLING") > 0);
    return v;
}

struct ViewportTransformHelper : public QQuickDeliveryAgent::Transform
{
    static void removeAll() {
        for (auto o : owners) {
            if (!o.isNull())
                o->setSceneTransform(nullptr);
        }
        owners.clear();
    }

    void setOnDeliveryAgent(QQuickDeliveryAgent *da) {
        da->setSceneTransform(this);
        owners.append(da);
    }

    /*
        Transforms viewport coordinates to 2D scene coordinates.
        Returns the point in targetItem corresponding to \a viewportPoint,
        assuming that targetItem is mapped onto sceneParentNode.
        If it's no longer a "hit" on sceneParentNode, returns the last-good point.
    */
    QPointF map(const QPointF &viewportPoint) override {
        std::optional<QSSGRenderRay> rayResult = renderer->getRayFromViewportPos(viewportPoint * dpr);
        if (rayResult.has_value()) {
            auto pickResult = renderer->syncPickOne(rayResult.value(), sceneParentNode);
            auto ret = pickResult.m_localUVCoords.toPointF();
            if (!uvCoordsArePixels) {
                ret = QPointF(targetItem->x() + ret.x() * targetItem->width(),
                              targetItem->y() - ret.y() * targetItem->height() + targetItem->height());
            }
            const bool outOfModel = pickResult.m_localUVCoords.isNull();
            qCDebug(lcEv) << viewportPoint << "->" << (outOfModel ? "OOM" : "") << ret << "@" << pickResult.m_scenePosition
                          << "UV" << pickResult.m_localUVCoords << "dist" << qSqrt(pickResult.m_distanceSq);
            if (outOfModel) {
                return lastGoodMapping;
            } else {
                lastGoodMapping = ret;
                return ret;
            }
        }
        return QPointF();
    }

    QQuick3DSceneRenderer *renderer = nullptr;
    QSSGRenderNode *sceneParentNode = nullptr;
    QQuickItem* targetItem = nullptr;
    qreal dpr = 1;
    bool uvCoordsArePixels = false; // if false, they are in the range 0..1
    QPointF lastGoodMapping;

    static QList<QPointer<QQuickDeliveryAgent>> owners;
};

QList<QPointer<QQuickDeliveryAgent>> ViewportTransformHelper::owners;

class QQuick3DExtensionListHelper
{
    Q_DISABLE_COPY_MOVE(QQuick3DExtensionListHelper);
public:
    static void extensionAppend(QQmlListProperty<QQuick3DObject> *list, QQuick3DObject *extension)
    {
        QSSG_ASSERT(list && extension, return);

        if (QQuick3DViewport *that = qobject_cast<QQuick3DViewport *>(list->object)) {
            if (const auto idx = that->m_extensions.indexOf(extension); idx == -1) {
                if (!extension->parentItem())
                    extension->setParentItem(that->m_sceneRoot);
                that->m_extensions.push_back(extension);
                that->m_extensionListDirty = true;
            }
        }
    }
    static QQuick3DObject *extensionAt(QQmlListProperty<QQuick3DObject> *list, qsizetype index)
    {
        QQuick3DObject *ret = nullptr;
        QSSG_ASSERT(list, return ret);

        if (QQuick3DViewport *that = qobject_cast<QQuick3DViewport *>(list->object)) {
            if (that->m_extensions.size() > index)
                ret = that->m_extensions.at(index);
        }

        return ret;
    }
    static qsizetype extensionCount(QQmlListProperty<QQuick3DObject> *list)
    {
        qsizetype ret = -1;
        QSSG_ASSERT(list, return ret);

        if (QQuick3DViewport *that = qobject_cast<QQuick3DViewport *>(list->object))
            ret = that->m_extensions.size();

        return ret;
    }
    static void extensionClear(QQmlListProperty<QQuick3DObject> *list)
    {
        QSSG_ASSERT(list, return);

        if (QQuick3DViewport *that = qobject_cast<QQuick3DViewport *>(list->object)) {
            that->m_extensions.clear();
            that->m_extensionListDirty = true;
        }
    }
    static void extensionReplace(QQmlListProperty<QQuick3DObject> *list, qsizetype idx, QQuick3DObject *o)
    {
        QSSG_ASSERT(list, return);

        if (QQuick3DViewport *that = qobject_cast<QQuick3DViewport *>(list->object)) {
            if (that->m_extensions.size() > idx && idx > -1) {
                that->m_extensions.replace(idx, o);
                that->m_extensionListDirty = true;
            }
        }
    }
        static void extensionRemoveLast(QQmlListProperty<QQuick3DObject> *list)
    {
        QSSG_ASSERT(list, return);

        if (QQuick3DViewport *that = qobject_cast<QQuick3DViewport *>(list->object)) {
            that->m_extensions.removeLast();
            that->m_extensionListDirty = true;
        }
    }
};

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

    To control how a scene is rendered, you can set the \l environment
    property. The type \l SceneEnvironment has a number of visual properties
    that can be adjusted, such as background color, tone mapping, anti-aliasing
    and more. \l ExtendedSceneEnvironment in the \c{QtQuick3D.Helpers} module
    extends \l SceneEnvironment with even more features, adding common
    post-processing effects.

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

    A View3D with the default Offscreen \l renderMode is implicitly a
    \l{QSGTextureProvider}{texture provider} as well. This means that \l
    ShaderEffect or \l{QtQuick3D::Texture::sourceItem}{Texture.sourceItem} can reference
    the View3D directly as long as all items live within the same
    \l{QQuickWindow}{window}. Like with any other \l Item, it is also possible
    to switch the View3D, or one of its ancestors, into a texture-based
    \l{QtQuick::Item::layer.enabled}{item layer}.

    \sa {Qt Quick 3D - View3D example}
*/

QQuick3DViewport::QQuick3DViewport(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents);
    m_camera = nullptr;
    m_sceneRoot = new QQuick3DSceneRootNode(this);
    m_environment = new QQuick3DSceneEnvironment(m_sceneRoot);
    m_renderStats = new QQuick3DRenderStats();
    QQuick3DSceneManager *sceneManager = new QQuick3DSceneManager();
    QQuick3DObjectPrivate::get(m_sceneRoot)->refSceneManager(*sceneManager);
    Q_ASSERT(sceneManager == QQuick3DObjectPrivate::get(m_sceneRoot)->sceneManager);
    connect(sceneManager, &QQuick3DSceneManager::needsUpdate,
            this, &QQuickItem::update);

    // Overrides the internal input handling to always be true
    // instead of potentially updated after a sync (see updatePaintNode)
    if (isforceInputHandlingSet()) {
        m_enableInputProcessing = true;
        updateInputProcessing();
        forceActiveFocus();
    }
}

QQuick3DViewport::~QQuick3DViewport()
{
    // If the quick window still exists, make sure to disconnect any of the direct
    // connections to this View3D
    if (auto qw = window())
        disconnect(qw, nullptr, this, nullptr);

    auto sceneManager = QQuick3DObjectPrivate::get(m_sceneRoot)->sceneManager;
    if (sceneManager) {
        sceneManager->setParent(nullptr);
        if (auto wa = sceneManager->wattached)
            wa->queueForCleanup(sceneManager);
    }

    delete m_sceneRoot;
    m_sceneRoot = nullptr;

    // m_renderStats is tightly coupled with the render thread, so can't delete while we
    // might still be rendering.
    m_renderStats->deleteLater();

    if (!window() && sceneManager && sceneManager->wattached) {
        if (sceneManager->wattached->rci().use_count() <= 1)
            delete sceneManager->wattached;
    }

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
    } else {
        QQuickItemPrivate::data_append(property, obj);
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

    \note If this property contains a camera that's not \l {Node::visible}{visible} then
    no further attempts to find a camera will be done.

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
    \qmlproperty enumeration QtQuick3D::View3D::renderFormat
    \since 6.4

    This property determines the backing texture's format. Applicable only when
    the View3D is rendering to a texture, for example because the \l renderMode
    is \c{View3D.Offscreen}.

    The default is \c{ShaderEffectSource.RGBA8}.

    If the format is not supported by the underlying graphics driver at run
    time, RGBA8 is used.

    \list
    \li ShaderEffectSource.RGBA8
    \li ShaderEffectSource.RGBA16F
    \li ShaderEffectSource.RGBA32F
    \endlist

    \sa QtQuick::ShaderEffectSource::format, QtQuick::Item::layer.format
 */
QQuickShaderEffectSource::Format QQuick3DViewport::renderFormat() const
{
    return m_renderFormat;
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
    QQuick3DSceneRenderer *renderer = nullptr;

    if (QQuickWindow *qw = window()) {
        auto wa = QQuick3DSceneManager::getOrSetWindowAttachment(*qw);
        auto rci = wa->rci();
        if (!rci) {
            QSGRendererInterface *rif = qw->rendererInterface();
            if (QSSG_GUARD(QSGRendererInterface::isApiRhiBased(rif->graphicsApi()))) {
                QRhi *rhi = static_cast<QRhi *>(rif->getResource(qw, QSGRendererInterface::RhiResource));
                QSSG_CHECK_X(rhi != nullptr, "No QRhi from QQuickWindow, this cannot happen");
                // The RenderContextInterface, and the objects owned by it (such
                // as, the BufferManager) are always per-QQuickWindow, and so per
                // scenegraph render thread. Hence the association with window.
                // Multiple View3Ds in the same window can use the same rendering
                // infrastructure (so e.g. the same QSSGBufferManager), but two
                // View3D objects in different windows must not, except for certain
                // components that do not work with and own native graphics
                // resources (most notably, QSSGShaderLibraryManager - but this
                // distinction is handled internally by QSSGRenderContextInterface).
                rci = std::make_shared<QSSGRenderContextInterface>(rhi);
                wa->setRci(rci);

                // Use DirectConnection to stay on the render thread, if there is one.
                connect(wa, &QQuick3DWindowAttachment::releaseCachedResources, this,
                        &QQuick3DViewport::onReleaseCachedResources, Qt::DirectConnection);

            } else {
                qWarning("The Qt Quick scene is using a rendering method that is not based on QRhi and a 3D graphics API. "
                         "Qt Quick 3D is not functional in such an environment. The View3D item is not going to display anything.");
            }
        }

        if (rci)
            renderer = new QQuick3DSceneRenderer(rci);
    }

    return renderer;
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

    switch (m_renderMode) {
    // Direct rendering
    case Underlay:
        Q_FALLTHROUGH();
    case Overlay:
        setupDirectRenderer(m_renderMode);
        node = nullptr;
        break;
    case Offscreen:
        node = setupOffscreenRenderer(node);
        break;
    case Inline:
        // QSGRenderNode-based rendering
        node = setupInlineRenderer(node);
        break;
    }

    if (!isforceInputHandlingSet()) {
        // Implicitly enable internal input processing if any item2ds are present.
        const auto inputHandlingEnabled =
                QQuick3DObjectPrivate::get(m_sceneRoot)->sceneManager->inputHandlingEnabled;
        const auto enable = inputHandlingEnabled > 0;
        if (m_enableInputProcessing != enable) {
            m_enableInputProcessing = enable;
            QMetaObject::invokeMethod(this, "updateInputProcessing", Qt::QueuedConnection);
        }
    }

    return node;
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
            m_renderStats->setWindow(value.window);
        }
    } else if (change == ItemVisibleHasChanged && isVisible()) {
        update();
    }
}

bool QQuick3DViewport::event(QEvent *event)
{
    if (m_enableInputProcessing && event->isPointerEvent())
        return internalPick(static_cast<QPointerEvent *>(event));
    else
        return QQuickItem::event(event);
}

void QQuick3DViewport::componentComplete()
{
    QQuickItem::componentComplete();
    Q_QUICK3D_PROFILE_REGISTER(this);
}

void QQuick3DViewport::setCamera(QQuick3DCamera *camera)
{
    if (m_camera == camera)
        return;

    if (camera && !camera->parentItem())
        camera->setParentItem(m_sceneRoot);
    if (camera)
        camera->updateGlobalVariables(QRect(0, 0, width(), height()));

    QQuick3DObjectPrivate::attachWatcherPriv(m_sceneRoot, this, &QQuick3DViewport::setCamera, camera, m_camera);

    m_camera = camera;
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
        auto privateObject = QQuick3DObjectPrivate::get(m_importScene);
        if (!privateObject->sceneManager) {
            // If object doesn't already have scene manager, check from its children
            QQuick3DSceneManager *manager = findChildSceneManager(m_importScene);
            // If still not found, use the one from the scene root (scenes defined outside of an view3d)
            if (!manager)
                manager = QQuick3DObjectPrivate::get(m_sceneRoot)->sceneManager;
            if (manager) {
                manager->setWindow(window());
                privateObject->refSceneManager(*manager);
            }
            // At this point some manager will exist
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

void QQuick3DViewport::setRenderFormat(QQuickShaderEffectSource::Format format)
{
    if (m_renderFormat == format)
        return;

    m_renderFormat = format;
    m_renderModeDirty = true;
    emit renderFormatChanged();
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
*/
QQuick3DPickResult QQuick3DViewport::pick(float x, float y) const
{
    QQuick3DSceneRenderer *renderer = getRenderer();
    if (!renderer)
        return QQuick3DPickResult();

    const QPointF position(qreal(x) * window()->effectiveDevicePixelRatio(),
                           qreal(y) * window()->effectiveDevicePixelRatio());
    std::optional<QSSGRenderRay> rayResult = renderer->getRayFromViewportPos(position);
    if (!rayResult.has_value())
        return QQuick3DPickResult();

    return processPickResult(renderer->syncPick(rayResult.value()));

}

/*!
    \qmlmethod List<PickResult> View3D::pickAll(float x, float y)

    This method will "shoot" a ray into the scene from view coordinates \a x and \a y
    and return a list of information about intersections with objects in the scene.
    The returned list is sorted by distance from the camera with the nearest
    intersections appearing first and the furthest appearing last.

    This can, for instance, be called with mouse coordinates to find the object under the mouse cursor.

    \since 6.2
*/
QList<QQuick3DPickResult> QQuick3DViewport::pickAll(float x, float y) const
{
    QQuick3DSceneRenderer *renderer = getRenderer();
    if (!renderer)
        return QList<QQuick3DPickResult>();

    const QPointF position(qreal(x) * window()->effectiveDevicePixelRatio(),
                           qreal(y) * window()->effectiveDevicePixelRatio());
    std::optional<QSSGRenderRay> rayResult = renderer->getRayFromViewportPos(position);
    if (!rayResult.has_value())
        return QList<QQuick3DPickResult>();

    const auto resultList = renderer->syncPickAll(rayResult.value());
    QList<QQuick3DPickResult> processedResultList;
    processedResultList.reserve(resultList.size());
    for (const auto &result : resultList)
        processedResultList.append(processPickResult(result));

    return processedResultList;
}

/*!
    \qmlmethod PickResult View3D::rayPick(vector3d origin, vector3d direction)

    This method will "shoot" a ray into the scene starting at \a origin and in
    \a direction and return information about the nearest intersection with an
    object in the scene.

    This can, for instance, be called with the position and forward vector of
    any object in a scene to see what object is in front of an item. This
    makes it possible to do picking from any point in the scene.

    \since 6.2
*/
QQuick3DPickResult QQuick3DViewport::rayPick(const QVector3D &origin, const QVector3D &direction) const
{
    QQuick3DSceneRenderer *renderer = getRenderer();
    if (!renderer)
        return QQuick3DPickResult();

    const QSSGRenderRay ray(origin, direction);

    return processPickResult(renderer->syncPick(ray));
}

/*!
    \qmlmethod List<PickResult> View3D::rayPickAll(vector3d origin, vector3d direction)

    This method will "shoot" a ray into the scene starting at \a origin and in
    \a direction and return a list of information about the nearest intersections with
    objects in the scene.
    The list is presorted by distance from the origin along the direction
    vector with the nearest intersections appearing first and the furthest
    appearing last.

    This can, for instance, be called with the position and forward vector of
    any object in a scene to see what objects are in front of an item. This
    makes it possible to do picking from any point in the scene.

    \since 6.2
*/
QList<QQuick3DPickResult> QQuick3DViewport::rayPickAll(const QVector3D &origin, const QVector3D &direction) const
{
    QQuick3DSceneRenderer *renderer = getRenderer();
    if (!renderer)
        return QList<QQuick3DPickResult>();

    const QSSGRenderRay ray(origin, direction);

    const auto resultList = renderer->syncPickAll(ray);
    QList<QQuick3DPickResult> processedResultList;
    processedResultList.reserve(resultList.size());
    for (const auto &result : resultList)
        processedResultList.append(processPickResult(result));

    return processedResultList;
}

void QQuick3DViewport::processPointerEventFromRay(const QVector3D &origin, const QVector3D &direction, QPointerEvent *event)
{
    internalPick(event, origin, direction);
}

QQuick3DLightmapBaker *QQuick3DViewport::maybeLightmapBaker()
{
    return m_lightmapBaker;
}

QQuick3DLightmapBaker *QQuick3DViewport::lightmapBaker()
{
    if (!m_lightmapBaker)
        m_lightmapBaker= new QQuick3DLightmapBaker(this);

    return m_lightmapBaker;
}

/*!
    \internal
*/
void QQuick3DViewport::bakeLightmap()
{
    lightmapBaker()->bake();
}

void QQuick3DViewport::setGlobalPickingEnabled(bool isEnabled)
{
    QQuick3DSceneRenderer *renderer = getRenderer();
    if (!renderer)
        return;

    renderer->setGlobalPickingEnabled(isEnabled);
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
    for (auto *texture : std::as_const(sceneManager->qsgDynamicTextures))
        texture->updateTexture();

    QQuick3DNode *scene = m_importScene;
    while (scene) {
        const auto &importSm = QQuick3DObjectPrivate::get(scene)->sceneManager;
        if (importSm != sceneManager) {
            for (auto *texture : std::as_const(importSm->qsgDynamicTextures))
                texture->updateTexture();
        }

        // if importScene has another import
        QQuick3DSceneRootNode *rn = qobject_cast<QQuick3DSceneRootNode *>(scene);
        scene = rn ? rn->view3D()->importScene() : nullptr;
    }
}

QSGNode *QQuick3DViewport::setupOffscreenRenderer(QSGNode *node)
{
    SGFramebufferObjectNode *n = static_cast<SGFramebufferObjectNode *>(node);

    if (!n) {
        if (!m_node)
            m_node = new SGFramebufferObjectNode;
        n = m_node;
    }

    if (!n->renderer) {
        n->window = window();
        n->renderer = createRenderer();
        if (!n->renderer)
            return nullptr;
        n->renderer->fboNode = n;
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
}

QSGNode *QQuick3DViewport::setupInlineRenderer(QSGNode *node)
{
    QQuick3DSGRenderNode *n = static_cast<QQuick3DSGRenderNode *>(node);
    if (!n) {
        if (!m_renderNode)
            m_renderNode = new QQuick3DSGRenderNode;
        n = m_renderNode;
    }

    if (!n->renderer) {
        n->window = window();
        n->renderer = createRenderer();
        if (!n->renderer)
            return nullptr;
    }

    const QSize targetSize = window()->effectiveDevicePixelRatio() * QSize(width(), height());

    // checkIsVisible, not isVisible, because, for example, a
    // { visible: false; layer.enabled: true } item still needs
    // to function normally.
    if (checkIsVisible() && isComponentComplete()) {
        n->renderer->synchronize(this, targetSize, window()->effectiveDevicePixelRatio());
        updateDynamicTextures();
        n->markDirty(QSGNode::DirtyMaterial);
    }

    return n;
}


void QQuick3DViewport::setupDirectRenderer(RenderMode mode)
{
    auto renderMode = (mode == Underlay) ? QQuick3DSGDirectRenderer::Underlay
                                         : QQuick3DSGDirectRenderer::Overlay;
    if (!m_directRenderer) {
        QQuick3DSceneRenderer *sceneRenderer = createRenderer();
        if (!sceneRenderer)
            return;
        m_directRenderer = new QQuick3DSGDirectRenderer(sceneRenderer, window(), renderMode);
        connect(window(), &QQuickWindow::sceneGraphInvalidated, this, &QQuick3DViewport::cleanupDirectRenderer, Qt::DirectConnection);
    }

    const QSizeF targetSize = window()->effectiveDevicePixelRatio() * QSizeF(width(), height());
    m_directRenderer->setViewport(QRectF(window()->effectiveDevicePixelRatio() * mapToScene(QPointF(0, 0)), targetSize));
    m_directRenderer->setVisibility(isVisible());
    if (isVisible()) {
        m_directRenderer->renderer()->synchronize(this, targetSize.toSize(), window()->effectiveDevicePixelRatio());
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

bool QQuick3DViewport::internalPick(QPointerEvent *event, const QVector3D &origin, const QVector3D &direction) const
{
    // Some non-thread-safe stuff to do input
    // First need to get a handle to the renderer
    QQuick3DSceneRenderer *renderer = getRenderer();
    if (!renderer || !event)
        return false;

    const bool isHover = QQuickDeliveryAgentPrivate::isHoverEvent(event);
    if (!isHover)
        qCDebug(lcEv) << event;
    struct SubsceneInfo {
        QQuick3DObject* obj = nullptr;
        QVarLengthArray<QPointF, 16> eventPointScenePositions;
    };
    QFlatMap<QQuickItem*, SubsceneInfo> visitedSubscenes;

    const bool useRayPicking = !direction.isNull();

    for (int pointIndex = 0; pointIndex < event->pointCount(); ++pointIndex) {
        QQuick3DSceneRenderer::PickResultList pickResults;
        auto &eventPoint = event->point(pointIndex);
        if (useRayPicking) {
            const QSSGRenderRay ray(origin, direction);
            pickResults = renderer->syncPickAll(ray);
        } else {
            const QPointF realPosition = eventPoint.position() * window()->effectiveDevicePixelRatio();
            std::optional<QSSGRenderRay> rayResult = renderer->getRayFromViewportPos(realPosition);
            if (rayResult.has_value())
                pickResults = renderer->syncPickAll(rayResult.value());
        }
        if (!isHover)
            qCDebug(lcPick) << pickResults.size() << "pick results for" << event->point(pointIndex);
        if (pickResults.isEmpty()) {
            eventPoint.setAccepted(false); // let it fall through the viewport to Items underneath
            continue; // next eventPoint
        }

        const auto sceneManager = QQuick3DObjectPrivate::get(m_sceneRoot)->sceneManager;

        for (const auto &pickResult : pickResults) {
            auto backendObject = pickResult.m_hitObject;
            if (!backendObject)
                continue;

            QQuick3DObject *frontendObject = sceneManager->lookUpNode(backendObject);
            if (!frontendObject && m_importScene) {
                const auto importSceneManager = QQuick3DObjectPrivate::get(m_importScene)->sceneManager;
                frontendObject = importSceneManager->lookUpNode(backendObject);
            }

            if (!frontendObject)
                continue;

            QQuickItem *subsceneRootItem = nullptr;
            QPointF subscenePosition;
            auto frontendObjectPrivate = QQuick3DObjectPrivate::get(frontendObject);
            if (frontendObjectPrivate->type == QQuick3DObjectPrivate::Type::Item2D) {
                // Item2D
                auto item2D = qobject_cast<QQuick3DItem2D *>(frontendObject);
                if (item2D)
                    subsceneRootItem = item2D->contentItem();
                if (!subsceneRootItem || subsceneRootItem->childItems().isEmpty())
                    continue; // ignore empty 2D subscenes
                // In this case the "UV" coordinates are in pixels in the subscene root item, so we can just use them.
                subscenePosition = pickResult.m_localUVCoords.toPointF();
                // Even though an Item2D is an "infinite plane" for rendering purposes,
                // avoid delivering events outside the rectangular area that is occupied by child items,
                // so that events can "fall through" to other interactive content in the scene or behind it.
                if (!subsceneRootItem->childrenRect().contains(subscenePosition))
                    continue;
            } else if (frontendObjectPrivate->type == QQuick3DObjectPrivate::Type::Model) {
                // Model
                int materialSubset = pickResult.m_subset;
                const auto backendModel = static_cast<const QSSGRenderModel *>(backendObject);
                // Get material
                if (backendModel->materials.size() < (pickResult.m_subset + 1))
                    materialSubset = backendModel->materials.size() - 1;
                if (materialSubset < 0)
                    continue;
                const auto backendMaterial = backendModel->materials.at(materialSubset);
                const auto frontendMaterial = sceneManager->lookUpNode(backendMaterial);
                if (!frontendMaterial)
                    continue;
                const auto frontendMaterialPrivate = QQuick3DObjectPrivate::get(frontendMaterial);

                if (frontendMaterialPrivate->type == QQuick3DObjectPrivate::Type::DefaultMaterial) {
                    // Default Material
                    const auto defaultMaterial = qobject_cast<QQuick3DDefaultMaterial *>(frontendMaterial);
                    if (defaultMaterial) {
                        // Just check for a diffuseMap for now
                        if (defaultMaterial->diffuseMap() && defaultMaterial->diffuseMap()->sourceItem())
                            subsceneRootItem = defaultMaterial->diffuseMap()->sourceItem();
                    }

                } else if (frontendMaterialPrivate->type == QQuick3DObjectPrivate::Type::PrincipledMaterial) {
                    // Principled Material
                    const auto principledMaterial = qobject_cast<QQuick3DPrincipledMaterial *>(frontendMaterial);
                    if (principledMaterial) {
                        // Just check for a baseColorMap for now
                        if (principledMaterial->baseColorMap() && principledMaterial->baseColorMap()->sourceItem())
                            subsceneRootItem = principledMaterial->baseColorMap()->sourceItem();
                    }
                } else if (frontendMaterialPrivate->type == QQuick3DObjectPrivate::Type::SpecularGlossyMaterial) {
                    // SpecularGlossy Material
                    const auto specularGlossyMaterial = qobject_cast<QQuick3DSpecularGlossyMaterial *>(frontendMaterial);
                    if (specularGlossyMaterial) {
                        // Just check for a albedoMap for now
                        if (specularGlossyMaterial->albedoMap() && specularGlossyMaterial->albedoMap()->sourceItem())
                            subsceneRootItem = specularGlossyMaterial->albedoMap()->sourceItem();
                    }
                } else if (frontendMaterialPrivate->type == QQuick3DObjectPrivate::Type::CustomMaterial) {
                    // Custom Material
                    const auto customMaterial = qobject_cast<QQuick3DCustomMaterial *>(frontendMaterial);
                    if (customMaterial) {
                        // This case is a bit harder because we can not know how the textures will be used
                        const auto &texturesInputs = customMaterial->m_dynamicTextureMaps;
                        for (const auto &textureInput : texturesInputs) {
                            if (auto texture = textureInput->texture()) {
                                if (texture->sourceItem()) {
                                    subsceneRootItem = texture->sourceItem();
                                    break;
                                }
                            }
                        }
                    }
                }
                if (subsceneRootItem) {
                    // In this case the pick result really is using UV coordinates.
                    subscenePosition = QPointF(subsceneRootItem->x() +
                                               pickResult.m_localUVCoords.x() * subsceneRootItem->width(),
                                               subsceneRootItem->y() -
                                               pickResult.m_localUVCoords.y() * subsceneRootItem->height() +
                                               subsceneRootItem->height());
                }
            }

            if (subsceneRootItem) {
                SubsceneInfo &subscene = visitedSubscenes[subsceneRootItem]; // create if not found
                subscene.obj = frontendObject;
                if (subscene.eventPointScenePositions.size() != event->pointCount()) {
                    // ensure capacity, and use an out-of-scene position rather than 0,0 by default
                    constexpr QPointF inf(-qt_inf(), -qt_inf());
                    subscene.eventPointScenePositions.resize(event->pointCount(), inf);
                }
                subscene.eventPointScenePositions[pointIndex] = subscenePosition;
            }
            if (isHover)
                qCDebug(lcHover) << "hover pick result:" << frontendObject << "@" << pickResult.m_scenePosition
                                << "UV" << pickResult.m_localUVCoords << "dist" << qSqrt(pickResult.m_distanceSq)
                                << "scene" << subscenePosition << subsceneRootItem;
            else
                qCDebug(lcPick) << "pick result:" << frontendObject << "@" << pickResult.m_scenePosition
                                << "UV" << pickResult.m_localUVCoords << "dist" << qSqrt(pickResult.m_distanceSq)
                                << "scene" << subscenePosition << subsceneRootItem;
        } // for pick results from each QEventPoint
    } // for each QEventPoint

    // Now deliver the entire event (all points) to each relevant subscene.
    // Maybe only some points fall inside, but QQuickDeliveryAgentPrivate::deliverMatchingPointsToItem()
    // makes reduced-subset touch events that contain only the relevant points, when necessary.
    bool ret = false;

//    ViewportTransformHelper::removeAll();
    QVarLengthArray<QPointF, 16> originalScenePositions;
    originalScenePositions.resize(event->pointCount());
    for (int pointIndex = 0; pointIndex < event->pointCount(); ++pointIndex)
        originalScenePositions[pointIndex] = event->point(pointIndex).scenePosition();
    for (auto subscene : visitedSubscenes) {
        QQuickItem *subsceneRoot = subscene.first;
        auto &subsceneInfo = subscene.second;
        Q_ASSERT(subsceneInfo.eventPointScenePositions.size() == event->pointCount());
        auto da = QQuickItemPrivate::get(subsceneRoot)->deliveryAgent();
        if (!isHover)
            qCDebug(lcPick) << "delivering to" << subsceneRoot << "via" << da << event;

        for (int pointIndex = 0; pointIndex < event->pointCount(); ++pointIndex) {
            const auto &pt = subsceneInfo.eventPointScenePositions.at(pointIndex);
            // By tradition, QGuiApplicationPrivate::processTouchEvent() has set the local position to the scene position,
            // and Qt Quick expects it to arrive that way: then QQuickDeliveryAgentPrivate::translateTouchEvent()
            // copies it into the scene position before localizing.
            // That might be silly, we might change it eventually, but gotta stay consistent for now.
            QEventPoint &ep = event->point(pointIndex);
            QMutableEventPoint::setPosition(ep, pt);
            QMutableEventPoint::setScenePosition(ep, pt);
        }

        if (event->isBeginEvent())
            da->setSceneTransform(nullptr);
        if (da->event(event)) {
            ret = true;
            if (QQuickDeliveryAgentPrivate::anyPointGrabbed(event) && !useRayPicking) {
                // In case any QEventPoint was grabbed, the relevant QQuickDeliveryAgent needs to know
                // how to repeat the picking/coordinate transformation for each update,
                // because delivery will bypass internalPick() due to the grab, and it's
                // more efficient to avoid whole-scene picking each time anyway.
                auto frontendObjectPrivate = QQuick3DObjectPrivate::get(subsceneInfo.obj);
                const bool item2Dcase = (frontendObjectPrivate->type == QQuick3DObjectPrivate::Type::Item2D);
                ViewportTransformHelper *transform = new ViewportTransformHelper;
                transform->renderer = renderer;
                transform->sceneParentNode = static_cast<QSSGRenderNode*>(frontendObjectPrivate->spatialNode);
                transform->targetItem = subsceneRoot;
                transform->dpr = window()->effectiveDevicePixelRatio();
                transform->uvCoordsArePixels = item2Dcase;
                transform->setOnDeliveryAgent(da);
                qCDebug(lcPick) << event->type() << "created ViewportTransformHelper on" << da;
            }
        } else if (event->type() != QEvent::HoverMove) {
            qCDebug(lcPick) << subsceneRoot << "didn't want" << event;
        }
        event->setAccepted(false); // reject implicit grab and let it keep propagating
    }
    if (visitedSubscenes.isEmpty()) {
        event->setAccepted(false);
    } else {
        for (int pointIndex = 0; pointIndex < event->pointCount(); ++pointIndex)
            QMutableEventPoint::setScenePosition(event->point(pointIndex), originalScenePositions.at(pointIndex));
    }
    return ret;
}

QQuick3DPickResult QQuick3DViewport::processPickResult(const QSSGRenderPickResult &pickResult) const
{
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

    QQuick3DModel *model = qobject_cast<QQuick3DModel *>(frontendObject);
    if (!model)
        return QQuick3DPickResult();

    return QQuick3DPickResult(model,
                              ::sqrtf(pickResult.m_distanceSq),
                              pickResult.m_localUVCoords,
                              pickResult.m_scenePosition,
                              pickResult.m_localPosition,
                              pickResult.m_faceNormal,
                              pickResult.m_instanceIndex);
}

// Returns the first found scene manager of objects children
QQuick3DSceneManager *QQuick3DViewport::findChildSceneManager(QQuick3DObject *inObject, QQuick3DSceneManager *manager)
{
    if (manager)
        return manager;

    auto children = QQuick3DObjectPrivate::get(inObject)->childItems;
    for (auto child : children) {
        if (auto m = QQuick3DObjectPrivate::get(child)->sceneManager) {
            manager = m;
            break;
        }
        manager = findChildSceneManager(child, manager);
    }
    return manager;
}

void QQuick3DViewport::updateInputProcessing()
{
    // This should be called from the gui thread.
    setAcceptTouchEvents(m_enableInputProcessing);
    setAcceptHoverEvents(m_enableInputProcessing);
    setAcceptedMouseButtons(m_enableInputProcessing ? Qt::AllButtons : Qt::NoButton);
}

void QQuick3DViewport::onReleaseCachedResources()
{
    if (auto renderer = getRenderer())
        renderer->releaseCachedResources();
}

/*!
    \internal
*/
QQmlListProperty<QQuick3DObject> QQuick3DViewport::extensions()
{
    return QQmlListProperty<QQuick3DObject>{ this,
                                              &m_extensionListDirty,
                                              &QQuick3DExtensionListHelper::extensionAppend,
                                              &QQuick3DExtensionListHelper::extensionCount,
                                              &QQuick3DExtensionListHelper::extensionAt,
                                              &QQuick3DExtensionListHelper::extensionClear,
                                              &QQuick3DExtensionListHelper::extensionReplace,
                                              &QQuick3DExtensionListHelper::extensionRemoveLast};
}

QT_END_NAMESPACE
