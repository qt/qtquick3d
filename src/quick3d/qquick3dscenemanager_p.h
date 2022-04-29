/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
******************************************************************************/

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

    void updateDirtyNodes();
    void updateDirtyNode(QQuick3DObject *object);
    void updateDirtyResource(QQuick3DObject *resourceObject);
    void updateDirtySpatialNode(QQuick3DNode *spatialNode);
    void updateBoundingBoxes(const QSSGRef<QSSGBufferManager> &mgr);

    QQuick3DObject *lookUpNode(const QSSGRenderGraphObject *node) const;

    void cleanupNodes();

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
    QQuickWindow *m_window = nullptr;
    QSSGRenderContextInterface *rci = nullptr;
    friend QQuick3DObject;

Q_SIGNALS:
    void needsUpdate();
    void windowChanged();

private Q_SLOTS:
    void preSync();
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuick3DSceneManager)

#endif // QSSGSCENEMANAGER_P_H
