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
    void dirtyObjectSurvivesManagerDeletion();
    void detachLeavesObjectReQueueable();
};

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

QTEST_MAIN(tst_QQuick3DSceneManager)
#include "tst_qquick3dscenemanager.moc"
