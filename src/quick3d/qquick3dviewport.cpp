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
    \inqmlmodule QtQuick3D
    \brief Provides a viewport on which to render a 3D scene.

    View3D provides a 2D surface for 3D content to be rendered to. Before 3D
    content can be displayed in a Qt Quick scene, it must first be flattend.

    There are two ways to define a 3D scene for View3D to view.  The first
    and easiest is to just define a higharchy of \l Node based items as
    children of the View3D. This becomes the implicit scene of the viewport.

    It is also possible to reference an existing scene by using the
    \l importScene property of the scene you want to render.
    This scene does not have to exist as a child of the current View3D.

    There is also a combination approach where you define both a scene with
    children nodes, as well as define a scene that is being referenced. In this
    case you can treat the referenced scene as a sibling of the child scene.

    This is demonstrated in \l {Qt Quick 3D - View3D example}{View3D example}

    To control how a scene is rendered, it is necessary to define a
    \l SceneEnvironment to the \l environment property.

    To project a 3D scene to a 2D viewport, it is necessary to view the scene
    from a camera. If a scene has more than one camera it is possible to set
    which camera is used to render the scene to this viewport by setting the
    \l camera property.

    It is also possible to define where the 3D scene is rendered to using
    the \l renderMode property. This can be necessary for performance
    reasons on certain platforms where it is expensive to render to
    intermediate offscreen surfaces. There are certain tradeoffs to rendering
    directly to the window though, so this is not the default.
*/

QQuick3DViewport::QQuick3DViewport(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents);
    m_camera = nullptr;
    m_sceneRoot = new QQuick3DSceneRootNode(this);
    m_environment = new QQuick3DSceneEnvironment(m_sceneRoot);
    m_renderStats = new QQuick3DRenderStats(m_sceneRoot);
    QSharedPointer<QQuick3DSceneManager> sceneManager(new QQuick3DSceneManager(m_sceneRoot));
    QQuick3DObjectPrivate::get(m_sceneRoot)->refSceneManager(sceneManager);
    connect(QQuick3DObjectPrivate::get(m_sceneRoot)->sceneManager.data(), &QQuick3DSceneManager::needsUpdate,
            this, &QQuickItem::update);
}

QQuick3DViewport::~QQuick3DViewport()
{
    for (const auto &connection : qAsConst(m_connections))
        disconnect(connection);
    // Do not delete scenemanager along with sceneroot
    auto &sceneManager = QQuick3DObjectPrivate::get(m_sceneRoot)->sceneManager;
    if (sceneManager)
        sceneManager->setParent(nullptr);
    delete m_sceneRoot;
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

    Holds the root scene of the View3D.

    \sa importScene
*/
QQuick3DNode *QQuick3DViewport::scene() const
{
    return m_sceneRoot;
}

/*!
    \qmlproperty QtQuick3D::Node QtQuick3D::View3D::importScene

    This property defines the reference node of the scene to render to the
    viewport. The node does not have to be a child of the View3D.
    This referenced node becomes sibling with possible child nodes of View3D.
    \note This property can only be set once, not removed or changed later.

    \sa Node
*/
QQuick3DNode *QQuick3DViewport::importScene() const
{
    return m_importScene;
}

/*!
    \qmlproperty enumeration QtQuick3D::View3D::renderMode

    This property determines how the scene is rendered to the viewport.

    \value View3D.Offscreen Scene is rendered to a texture. Comes with no limitations.
    \value View3D.Underlay Scene is rendered directly to the window before Qt Quick is rendered.
    \value View3D.Overlay Scene is rendered directly to the window after Qt Quick is rendered.
    \value View3D.Inline Scene is rendered to the current render target using QSGRenderNode.

    The default mode is \c View3D.Offscreen as this is the offers the best compatibility.
*/
QQuick3DViewport::RenderMode QQuick3DViewport::renderMode() const
{
    return m_renderMode;
}

/*!
    \qmlproperty QtQuick3D::RenderStats QtQuick3D::View3D::renderStats

    Accessor to \l {RenderStats}, which can be used to gain information of
    \l {RenderStats::fps}{fps}, \l {RenderStats::frameTime}{frameTime},
    \l {RenderStats::renderTime}{renderTime}, \l {RenderStats::syncTime}{syncTime},
    and \l {RenderStats::maxFrameTime}{maxFrameTime}.
*/
QQuick3DRenderStats *QQuick3DViewport::renderStats() const
{
    return m_renderStats;
}

void QQuick3DViewport::setShaderCacheFile(const QUrl &shaderCacheFile)
{
    m_shaderCacheFile = shaderCacheFile;
}

QUrl QQuick3DViewport::shaderCacheFile()
{
    return m_shaderCacheFile;
}

void QQuick3DViewport::readShaderCache()
{
    QByteArray error;
    if (!shaderCacheFile().isEmpty()) {
        QFile file(QQmlFile::urlToLocalFileOrQrc(shaderCacheFile()));
        if (file.open(QIODevice::ReadOnly))
            m_shaderCacheData = qUncompress(file.readAll());

        if (m_shaderCacheData.isEmpty()) {
            error = QByteArrayLiteral("Failed to read or uncompress shader cache: ");
            error.append(shaderCacheFile().toString().toUtf8());
            error.append(" ");
            error.append(file.errorString().toUtf8());
        }
    } else if (!m_shaderCacheImport.isEmpty()) {
        m_shaderCacheData = qUncompress(m_shaderCacheImport);
        if (m_shaderCacheData.isEmpty())
            error = QByteArrayLiteral("Failed uncompress shader cache.");
    }

    if (!error.isEmpty())
        Q_EMIT shaderCacheLoadErrors(error);
}

void QQuick3DViewport::writeShaderCache(const QUrl &shaderCacheFile)
{
    if (m_shaderCacheData.isEmpty()) {
        Q_EMIT shaderCacheExported(false);
        return; // Warning is already printed by export function
    }
    const QString filePath = shaderCacheFile.toLocalFile();
    if (filePath.isEmpty()) {
        qWarning() << __FUNCTION__ << "Warning: Invalid filename: " << shaderCacheFile;
        Q_EMIT shaderCacheExported(false);
        return;
    }
    QSaveFile file(filePath);
    QFileInfo(filePath).dir().mkpath(QStringLiteral("."));
    bool success = false;
    if (file.open(QIODevice::WriteOnly) && file.write(m_shaderCacheData) != -1) {
        file.commit();
        success = true;
    } else {
        qWarning() << __FUNCTION__ << "Warning: Failed to write shader cache:"
                   << shaderCacheFile << file.errorString();
    }
    Q_EMIT shaderCacheExported(success);
}

void QQuick3DViewport::doExportShaderCache()
{
    if (m_exportShaderCacheRequested) {
        if (!QOpenGLContext::currentContext()) {
            qWarning () << "Unable to export shader cache. No current context.";
            m_exportShaderCacheRequested = false;
            Q_EMIT shaderCacheExported(false);
            return;
        }
        auto rci = QSSGRenderContextInterface::getRenderContextInterface(quintptr(window()));
        if (rci) {
            m_shaderCacheData = rci->shaderCache()->exportShaderCache(m_binaryShaders);
            if (m_shaderCacheData.size()) {
                m_shaderCacheData = qCompress(m_shaderCacheData, m_compressionLevel);
                writeShaderCache(m_exportShaderCacheFile);
            } else {
                Q_EMIT shaderCacheExported(false);
            }
        }
        m_exportShaderCacheRequested = false;
    }
}

void QQuick3DViewport::doImportShaderCache()
{
    readShaderCache();
    if (!m_shaderCacheData.isNull()) {
        QByteArray error;
        auto rci = QSSGRenderContextInterface::getRenderContextInterface(quintptr(window()));
        rci->shaderCache()->importShaderCache(m_shaderCacheData, error);
        if (!error.isEmpty())
            Q_EMIT shaderCacheLoadErrors(error);
    }
}

void QQuick3DViewport::exportShaderCache(const QUrl &shaderCacheFile, bool binaryShaders,
                                         int compressionLevel)
{
    if (m_exportShaderCacheRequested) {
        qWarning () << "Export shader cache already requested";
        return;
    }
    m_exportShaderCacheFile = shaderCacheFile;
    m_binaryShaders = binaryShaders;
    m_compressionLevel = compressionLevel;
    m_exportShaderCacheRequested = true;
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
    if (!w || !w->openglContext() || QThread::currentThread() != w->openglContext()->thread()) {
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
            m_node = nullptr;
            m_renderNode = nullptr;
        }
        if (m_directRenderer) {
            delete m_directRenderer;
            m_directRenderer = nullptr;
        }
        updateClearBeforeRendering();
    }

    m_renderModeDirty = false;

    doExportShaderCache();

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
            doImportShaderCache();
        }
        QSize minFboSize = QQuickItemPrivate::get(this)->sceneGraphContext()->minimumFBOSize();
        QSize desiredFboSize(qMax<int>(minFboSize.width(), width()),
                             qMax<int>(minFboSize.height(), height()));

        n->devicePixelRatio = window()->effectiveDevicePixelRatio();
        desiredFboSize *= n->devicePixelRatio;

        n->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
        n->setFiltering(smooth() ? QSGTexture::Linear : QSGTexture::Nearest);
        n->setRect(0, 0, width(), height());
        if (checkIsVisible() && isComponentComplete()) {
            n->renderer->synchronize(this, desiredFboSize);
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
            doImportShaderCache();
        }

        const QSize targetSize = window()->effectiveDevicePixelRatio() * QSize(width(), height());

        if (isVisible() && isComponentComplete()) {
            n->renderer->synchronize(this, targetSize, false);
            updateDynamicTextures();
            n->markDirty(QSGNode::DirtyMaterial);
        }

        return n;
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

        QQuick3DSceneRootNode *rn = dynamic_cast<QQuick3DSceneRootNode *>(scene);
        scene = rn ? rn->view3D()->importScene() : nullptr;
    }

    m_importScene = inScene;
    if (m_importScene) {
        // If the referenced scene doesn't have a manager, add one (scenes defined outside of an view3d)
        auto privateObject = QQuick3DObjectPrivate::get(m_importScene);
        // ### BUG: This will probably leak, need to think harder about this
        if (!privateObject->sceneManager) {
            QSharedPointer<QQuick3DSceneManager> manager(new QQuick3DSceneManager(m_importScene));
            manager->setWindow(window());
            privateObject->refSceneManager(manager);
        }

        connect(privateObject->sceneManager.data(), &QQuick3DSceneManager::needsUpdate,
                this, &QQuickItem::update);

        QQuick3DNode *scene = inScene;
        while (scene) {
            QQuick3DSceneRootNode *rn = dynamic_cast<QQuick3DSceneRootNode *>(scene);
            scene = rn ? rn->view3D()->importScene() : nullptr;

            if (scene) {
                connect(QQuick3DObjectPrivate::get(scene)->sceneManager.data(),
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

    Transforms \a scenePos from scene space (3D) into view space (2D). The
    returned x- and y-values will be be in view coordinates. The returned z-value
    will contain the distance from the near side of the frustum (clipNear) to
    \a scenePos in scene coordinates. If the distance is negative, the point is behind the camera.
    If \a scenePos cannot be mapped to a position in the scene, a position of [0, 0, 0] is returned.
    This function requires that a camera is assigned to the view.

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

    Transforms \a viewPos from view space (2D) into scene space (3D). The x- and
    y-values of \a viewPos should be in view coordinates. The z-value should be
    the distance from the near side of the frustum (clipNear) into the scene in scene
    coordinates. If \a viewPos cannot be mapped to a position in the scene, a
    position of [0, 0, 0] is returned.

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

    Transforms the screen space coordinates \a x and \a y to a ray cast towards that position
    in scene space. Returns information about the ray hit.
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
    // TODO: could be optimized to not update textures that aren't used or are on culled
    // geometry.
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
        QQuick3DSceneRootNode *rn = dynamic_cast<QQuick3DSceneRootNode *>(scene);
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
        doImportShaderCache();
    }
    const QSizeF targetSize = window()->effectiveDevicePixelRatio() * QSizeF(width(), height());
    m_directRenderer->setViewport(QRectF(window()->effectiveDevicePixelRatio() * mapToScene(QPointF(0, 0)), targetSize));
    m_directRenderer->setVisibility(isVisible());
    if (isVisible()) {
        m_directRenderer->renderer()->synchronize(this, targetSize.toSize(), false);
        updateDynamicTextures();
        m_directRenderer->requestRender();
    }
    updateClearBeforeRendering();
}

void QQuick3DViewport::updateClearBeforeRendering()
{
    // Don't clear window when rendering visible underlay
    window()->setClearBeforeRendering(m_renderMode != Underlay || !isVisible());
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
