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

#ifndef QSSGVIEW3D_H
#define QSSGVIEW3D_H

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

#include <QtQuick/QQuickItem>
#include <QtCore/qurl.h>

#include <QtQuick3D/qtquick3dglobal.h>
#include <QtQuick3D/private/qquick3dpickresult_p.h>

#include "qquick3dsceneenvironment_p.h"
#include "qquick3drenderstats_p.h"

QT_BEGIN_NAMESPACE

class QSSGView3DPrivate;
class QQuick3DCamera;
class QQuick3DSceneEnvironment;
class QQuick3DNode;
class QQuick3DSceneRootNode;
class QQuick3DSceneRenderer;
class QQuick3DRenderStats;
class QQuick3DSceneManager;

class SGFramebufferObjectNode;
class QQuick3DSGRenderNode;
class QQuick3DSGDirectRenderer;

class Q_QUICK3D_EXPORT QQuick3DViewport : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QObject> data READ data DESIGNABLE false FINAL)
    Q_PROPERTY(QQuick3DCamera *camera READ camera WRITE setCamera NOTIFY cameraChanged FINAL)
    Q_PROPERTY(QQuick3DSceneEnvironment *environment READ environment WRITE setEnvironment NOTIFY environmentChanged FINAL)
    Q_PROPERTY(QQuick3DNode *scene READ scene NOTIFY sceneChanged)
    Q_PROPERTY(QQuick3DNode *importScene READ importScene WRITE setImportScene NOTIFY importSceneChanged FINAL)
    Q_PROPERTY(RenderMode renderMode READ renderMode WRITE setRenderMode NOTIFY renderModeChanged FINAL)
    Q_PROPERTY(QQuick3DRenderStats *renderStats READ renderStats CONSTANT)
    Q_CLASSINFO("DefaultProperty", "data")

    QML_NAMED_ELEMENT(View3D)

public:
    enum RenderMode {
        Offscreen,
        Underlay,
        Overlay,
        Inline
    };
    Q_ENUM(RenderMode)

    explicit QQuick3DViewport(QQuickItem *parent = nullptr);
    ~QQuick3DViewport() override;

    QQmlListProperty<QObject> data();

    QQuick3DCamera *camera() const;
    QQuick3DSceneEnvironment *environment() const;
    QQuick3DNode *scene() const;
    QQuick3DNode *importScene() const;
    RenderMode renderMode() const;
    QQuick3DRenderStats *renderStats() const;

    QQuick3DSceneRenderer *createRenderer() const;

    bool isTextureProvider() const override;
    QSGTextureProvider *textureProvider() const override;
    void releaseResources() override;

    Q_INVOKABLE QVector3D mapFrom3DScene(const QVector3D &scenePos) const;
    Q_INVOKABLE QVector3D mapTo3DScene(const QVector3D &viewPos) const;

    Q_INVOKABLE QQuick3DPickResult pick(float x, float y) const;
    Q_REVISION(6, 2) Q_INVOKABLE QList<QQuick3DPickResult> pickAll(float x, float y) const;
    Q_REVISION(6, 2) Q_INVOKABLE QQuick3DPickResult rayPick(const QVector3D &origin, const QVector3D &direction) const;
    Q_REVISION(6, 2) Q_INVOKABLE QList<QQuick3DPickResult> rayPickAll(const QVector3D &origin, const QVector3D &direction) const;

    void processPointerEventFromRay(const QVector3D &origin, const QVector3D &direction, QPointerEvent *event);

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value) override;

    bool event(QEvent *) override;

public Q_SLOTS:
    void setCamera(QQuick3DCamera *camera);
    void setEnvironment(QQuick3DSceneEnvironment * environment);
    void setImportScene(QQuick3DNode *inScene);
    void setRenderMode(QQuick3DViewport::RenderMode renderMode);
    void cleanupDirectRenderer();

    // Setting this true enables picking for all the models, regardless of
    // the models pickable property.
    void setGlobalPickingEnabled(bool isEnabled);

private Q_SLOTS:
    void invalidateSceneGraph();
    void cleanupResources();

Q_SIGNALS:
    void cameraChanged();
    void environmentChanged();
    void sceneChanged();
    void importSceneChanged();
    void renderModeChanged();

private:
    Q_DISABLE_COPY(QQuick3DViewport)
    QQuick3DSceneRenderer *getRenderer() const;
    void updateDynamicTextures();
    void setupDirectRenderer(RenderMode mode);
    bool checkIsVisible() const;
    bool internalPick(QPointerEvent *event, const QVector3D &origin = QVector3D(), const QVector3D &direction = QVector3D()) const;
    QQuick3DPickResult processPickResult(const QSSGRenderPickResult &pickResult) const;
    QQuick3DSceneManager *findChildSceneManager(QQuick3DObject *inObject, QQuick3DSceneManager *manager = nullptr);

    QQuick3DCamera *m_camera = nullptr;
    QQuick3DSceneEnvironment *m_environment = nullptr;
    QQuick3DSceneRootNode *m_sceneRoot = nullptr;
    QQuick3DNode *m_importScene = nullptr;
    mutable SGFramebufferObjectNode *m_node = nullptr;
    mutable QQuick3DSGRenderNode *m_renderNode = nullptr;
    mutable QQuick3DSGDirectRenderer *m_directRenderer = nullptr;
    bool m_renderModeDirty = false;
    RenderMode m_renderMode = Offscreen;
    QQuick3DRenderStats *m_renderStats = nullptr;
    QHash<QObject*, QMetaObject::Connection> m_connections;
    bool m_enableInputProcessing = true;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuick3DViewport)

#endif // QSSGVIEW3D_H
