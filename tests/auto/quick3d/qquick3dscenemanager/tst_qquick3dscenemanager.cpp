// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dobject_p.h>
#include <QtQuick3D/private/qquick3dscenemanager_p.h>

class tst_QQuick3DSceneManager : public QObject
{
    Q_OBJECT

private slots:
    void refSceneManagerSurvivesManagerDeletion();
    void dirtyObjectSurvivesManagerDeletion();
    void detachLeavesObjectReQueueable();
    void reentrantDirtyDuringUpdateSpatialNodeIsNotDropped();
};

// A QML binding evaluated as a side effect of updateSpatialNode() (e.g. Model::instanceRoot
// bound to Model::bounds) can call dirty() on the very object that is currently being synced,
// from inside its own updateSpatialNode() call. This stand-in reproduces that reentrancy without
// needing a Model or any particular property.
//
// Plain QQuick3DObject (an "Unknown"-type, i.e. resource) is used rather than QQuick3DNode:
// the node sync path (updateDirtySpatialNode()) additionally wires the object into a scene
// graph rooted at a QQuick3DWindowAttachment, which this lightweight test has no need to set
// up. The resource sync path (updateDirtyResource()) exercises the exact same reentrancy-guard
// code and has none of that scaffolding.
class ReentrantDirtyObject : public QQuick3DObject
{
public:
    int updateCount = 0;

    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override
    {
        ++updateCount;
        QSSGRenderGraphObject *result = QQuick3DObject::updateSpatialNode(node);
        if (updateCount == 1) {
            // Simulate the reentrant re-dirty happening exactly once, on the first sync,
            // the way a one-shot property-watcher registration or a binding tied to a
            // property this very update just changed would.
            QQuick3DObjectPrivate::get(this)->dirty(QQuick3DObjectPrivate::OpacityValue);
        }
        return result;
    }
};

// An object can be referenced by more than one view. When the view owning the tracked scene
// manager is destroyed, the manager is deleted and the object's QPointer auto-clears, but the
// reference count is not decremented. A subsequent refSceneManager() with a new manager used to
// dereference the dead manager (sceneManager->window()) and crash when the count was already > 1.
// This verifies that the new manager is re-adopted instead, and that the object can still be
// torn down cleanly afterwards. See QTBUG-144962 and QTBUG-88320.
void tst_QQuick3DSceneManager::refSceneManagerSurvivesManagerDeletion()
{
    auto *object = new QQuick3DNode;
    auto *priv = QQuick3DObjectPrivate::get(object);

    auto *managerA = new QQuick3DSceneManager;

    // Reference the object twice from the same manager, This leaves sceneRefCount at 2.
    QQuick3DObjectPrivate::refSceneManager(object, *managerA);
    QQuick3DObjectPrivate::refSceneManager(object, *managerA);
    QCOMPARE(priv->sceneManager.data(), managerA);
    QCOMPARE(priv->sceneRefCount, 2);

    // The owning view is destroyed: its scene manager is deleted, the QPointer auto-clears to
    // null, but sceneRefCount stays at 2 (Expected).
    delete managerA;
    QVERIFY(priv->sceneManager.isNull());
    QCOMPARE(priv->sceneRefCount, 2);

    // A new view references the same object. It must now re-adopt the new, valid manager.
    auto *managerB = new QQuick3DSceneManager;
    QQuick3DObjectPrivate::refSceneManager(object, *managerB); // must not crash
    QCOMPARE(priv->sceneManager.data(), managerB);

    // Destroy the object while the manager is still alive. This exercises the destructor's
    // sceneRefCount clamp and the final derefSceneManager() cleanup path; it must not crash.
    delete object;
    delete managerB;
}

// An object on a scene manager's dirty list keeps a prevDirtyItem pointer into the manager's
// dirty-list arrays. Deleting the manager must detach such objects, otherwise that pointer
// dangles and the next removeFromDirtyList() (e.g. when the object is later destroyed) writes
// through freed memory.
void tst_QQuick3DSceneManager::dirtyObjectSurvivesManagerDeletion()
{
    auto *manager = new QQuick3DSceneManager;
    auto *object = new QQuick3DNode;
    auto *priv = QQuick3DObjectPrivate::get(object);

    // Referencing the manager puts the object into the manager's dirty list.
    QQuick3DObjectPrivate::refSceneManager(object, *manager);
    object->update();
    QVERIFY(priv->prevDirtyItem != nullptr);

    // Deleting the manager must clear the dirty-list links that point into it.
    delete manager;
    QVERIFY(priv->prevDirtyItem == nullptr);
    QVERIFY(priv->nextDirtyItem == nullptr);

    // With the links cleared call removeFromDirtyList() should be a safe no-op.
    priv->removeFromDirtyList();

    delete object;
}

// Verifies that objects can be re-scheduled when the scene manager is destroyed.
// addToDirtyList() only re-adds an object when prevDirtyItem is null.
// A stale (non-null) prevDirtyItem would block any later re-add!
//
// NOTE: this drives refSceneManager() directly, so it does NOT exercise the
// QQuick3DViewport::updateSceneManagerForImportScene() / destroyed() contract,
// that is covered by tst_importscenelifetime's sharedSceneReQueuedAfterHomeViewDestroyed().
void tst_QQuick3DSceneManager::detachLeavesObjectReQueueable()
{
    auto *managerA = new QQuick3DSceneManager;
    auto *object = new QQuick3DNode;
    auto *priv = QQuick3DObjectPrivate::get(object);

    QQuick3DObjectPrivate::refSceneManager(object, *managerA);
    object->update();
    QVERIFY(priv->prevDirtyItem != nullptr); // dirty, on A's list

    // Simulate the view, and its scene manager, being destroyed. Dirty objects needs
    // to be detached from the list.
    delete managerA;
    QVERIFY(priv->prevDirtyItem == nullptr);
    QVERIFY(priv->sceneManager.isNull());

    // Verify that the object can be re-queued with a new manager. A stale prevDirtyItem would block this.
    auto *managerB = new QQuick3DSceneManager;
    QQuick3DObjectPrivate::refSceneManager(object, *managerB);
    QCOMPARE(priv->sceneManager.data(), managerB);
    QVERIFY(priv->prevDirtyItem != nullptr); // re-queued on B!

    delete object;
    delete managerB;
}

// If dirty() is re-entered on an object while that object is still inside its own
// updateSpatialNode() call, the pending change it flags must not be silently dropped: the
// object needs to end up back on a dirty list so a later sync actually revisits it. See
// QTBUG-149055.
void tst_QQuick3DSceneManager::reentrantDirtyDuringUpdateSpatialNodeIsNotDropped()
{
    auto *manager = new QQuick3DSceneManager;
    auto *object = new ReentrantDirtyObject;
    auto *priv = QQuick3DObjectPrivate::get(object);

    QQuick3DObjectPrivate::refSceneManager(object, *manager);
    object->update();
    QVERIFY(priv->prevDirtyItem != nullptr);

    // First sync: updateSpatialNode() reentrantly re-dirties the object.
    manager->updateDirtyResourceNodes();
    QCOMPARE(object->updateCount, 1);

    // The reentrant dirty() call must leave the object relinked into a dirty list so a later
    // sync actually revisits it, instead of merely dirty-flagged with nowhere to go.
    QVERIFY(priv->dirtyAttributes != 0);
    QVERIFY(priv->prevDirtyItem != nullptr);

    // A subsequent sync must actually process the pending change that was set reentrantly.
    manager->updateDirtyResourceNodes();
    QCOMPARE(object->updateCount, 2);
    QCOMPARE(priv->dirtyAttributes, 0u);
    QVERIFY(priv->prevDirtyItem == nullptr);

    delete object;
    delete manager;
}

QTEST_MAIN(tst_QQuick3DSceneManager)
#include "tst_qquick3dscenemanager.moc"
