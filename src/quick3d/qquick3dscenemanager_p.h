// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGSCENEMANAGER_P_H
#define QSSGSCENEMANAGER_P_H

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

#include <QtCore/QObject>
#include <QtCore/QSet>

#include <QtQuick3D/private/qtquick3dglobal_p.h>

#include "qquick3dobject_p.h"
#include "qquick3dnode_p.h"

QT_BEGIN_NAMESPACE

class QSGDynamicTexture;
class QQuickWindow;
class QSSGBufferManager;
class QSSGRenderContextInterface;

class Q_QUICK3D_PRIVATE_EXPORT QQuick3DSceneManager : public QObject
{
    Q_OBJECT
public:
    explicit QQuick3DSceneManager(QObject *parent = nullptr);
    ~QQuick3DSceneManager() override;

    void setWindow(QQuickWindow *window);
    QQuickWindow *window();

    void dirtyItem(QQuick3DObject *item);
    void cleanup(QSSGRenderGraphObject *item);

    void polishItems();
    void forcePolish();
    void sync();

    void cleanupNodes();
    void updateDirtyResourceNodes();
    void updateDirtySpatialNodes();

    void updateDirtyNode(QQuick3DObject *object);
    void updateDirtyResource(QQuick3DObject *resourceObject);
    void updateDirtySpatialNode(QQuick3DNode *spatialNode);
    void updateBoundingBoxes(const QSSGRef<QSSGBufferManager> &mgr);

    QQuick3DObject *lookUpNode(const QSSGRenderGraphObject *node) const;

    QQuick3DObject *dirtySpatialNodeList;
    QQuick3DObject *dirtyResourceList;
    QQuick3DObject *dirtyImageList;
    QQuick3DObject *dirtyTextureDataList;
    QList<QQuick3DObject *> dirtyLightList;
    QList<QQuick3DObject *> dirtyBoundingBoxList;
    QList<QSSGRenderGraphObject *> cleanupNodeList;
    QList<QSSGRenderGraphObject *> resourceCleanupQueue;
    QSet<QQuick3DObject *> parentlessItems;
    QVector<QSGDynamicTexture *> qsgDynamicTextures;
    QHash<const QSSGRenderGraphObject *, QQuick3DObject *> m_nodeMap;
    QSet<QSSGRenderGraphObject *> resourceLoaders;
    QQuickWindow *m_window = nullptr;
    QSSGRenderContextInterface *rci = nullptr;
    friend QQuick3DObject;

Q_SIGNALS:
    void needsUpdate();
    void windowChanged();

private Q_SLOTS:
    void updateNodes(QQuick3DObject **listHead);
    void preSync();
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuick3DSceneManager)

#endif // QSSGSCENEMANAGER_P_H
