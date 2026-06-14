// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QQuickView>
#include <QQuickItem>

#include <private/qquick3dscenemanager_p.h>
#include <private/qquick3dobject_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderroot_p.h>

#include "../shared/util.h"

class tst_ImportSceneLifetime : public QQuick3DDataTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void sharedSceneSurvivesViewRecreation();
    void sharedSceneReQueuedAfterHomeViewDestroyed();
};

void tst_ImportSceneLifetime::initTestCase()
{
    QQuick3DDataTest::initTestCase();
    if (!initialized())
        return;
}

// A scene imported into a View3D is owned by the QML tree, not the view, so it can
// outlive the view that imports it. The view's render root lives on the per-window
// QQuick3DWindowAttachment, and imported nodes hold a back-reference to that root
// through rootNodeRef. If the attachment (and its root) is destroyed when the last
// view in a window goes away, that back-reference dangles, and recreating a view that
// imports the same scene reads the freed root in setImportScene() -> a use-after-free.
//
// Destroy the importing view while the window (and the shared scene) live on, and
// verify the attachment and its render root are kept alive and reused. This is checked
// by pointer identity only, the freed root is never dereferenced, so it is a
// deterministic check rather than relying on hitting the use-after-free read.
void tst_ImportSceneLifetime::sharedSceneSurvivesViewRecreation()
{
    QScopedPointer<QQuickView> view(createView(QStringLiteral("importscenelifetime.qml"), QSize(200, 200)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));
    QObject *root = view->rootObject();
    QVERIFY(root);
    QVERIFY(waitForFrames(view.data(), 2));

    // The view is present and importing the shared scene; capture the window attachment
    // and the render root the shared scene's nodes point at.
    auto *attachment = QQuick3DSceneManager::getOrSetWindowAttachment(*view);
    QVERIFY(attachment);
    const QSSGRenderRoot *rootBefore = attachment->rootNode();
    QVERIFY(rootBefore);

    // Destroy the importing view. The shared scene lives on, so the attachment and its
    // root must not be torn down while the window is alive.
    QVERIFY(root->setProperty("loadView", false));
    QVERIFY(waitForFrames(view.data(), 3));
    QCoreApplication::processEvents();
    QVERIFY(waitForFrames(view.data(), 1));

    // Same window -> same attachment and same render root. getOrSetWindowAttachment()
    // would hand back a fresh attachment (with a new root) had the old one been deleted.
    // Pointer comparison only; the old root is never dereferenced.
    auto *attachmentAfter = QQuick3DSceneManager::getOrSetWindowAttachment(*view);
    // Compare as raw pointers: a freed attachment must not be dereferenced (which QCOMPARE's
    // QObject formatting would do) when reporting a mismatch.
    QCOMPARE(static_cast<const void *>(attachmentAfter), static_cast<const void *>(attachment));
    QCOMPARE(attachmentAfter->rootNode(), rootBefore);

    // Recreate the view importing the same shared scene; setImportScene() runs again and
    // must read a still-valid root. Reaching here without crashing (ASan) is part of the
    // assertion.
    QVERIFY(root->setProperty("loadView", true));
    QVERIFY(waitForFrames(view.data(), 2));
    QCOMPARE(QQuick3DSceneManager::getOrSetWindowAttachment(*view)->rootNode(), rootBefore);
}


// Test and verify the re-scheduling contract that keeps a shared imported scene alive when the transient view is destroyed.
// If that contract breaks, the shared scene is orphaned (sceneManager == null) and this will fail.
// See comments in the test body for details.
void tst_ImportSceneLifetime::sharedSceneReQueuedAfterHomeViewDestroyed()
{
    QScopedPointer<QQuickView> view(createView(QStringLiteral("sharedimportreadopt.qml"), QSize(400, 200)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));
    QObject *root = view->rootObject();
    QVERIFY(root);
    QVERIFY(waitForFrames(view.data(), 2));

    auto *sharedModel = root->findChild<QQuick3DObject *>(QStringLiteral("sharedModel"));
    QVERIFY(sharedModel);
    auto *transientViewCamera = root->findChild<QQuick3DObject *>(QStringLiteral("transientViewCamera"));
    QVERIFY(transientViewCamera);
    auto *staticViewCamera = root->findChild<QQuick3DObject *>(QStringLiteral("staticViewCamera"));
    QVERIFY(staticViewCamera);

    auto *modelPriv = QQuick3DObjectPrivate::get(sharedModel);
    // A camera's scene manager is its View3D's scene manager.
    QQuick3DSceneManager *transientSM = QQuick3DObjectPrivate::get(transientViewCamera)->sceneManager;
    QQuick3DSceneManager *staticSM = QQuick3DObjectPrivate::get(staticViewCamera)->sceneManager;
    QVERIFY(transientSM);
    QVERIFY(staticSM);
    QVERIFY(transientSM != staticSM);

    // Only the transient view imports the shared scene so far, so its objects are adopted by it.
    QVERIFY(modelPriv->sceneManager);
    QCOMPARE(static_cast<const void *>(modelPriv->sceneManager.data()),
             static_cast<const void *>(transientSM));

    // The static view now imports it too. The transient stays as the main caretaker.
    QVERIFY(root->setProperty("staticViewImports", true));
    QVERIFY(waitForFrames(view.data(), 2));
    QCOMPARE(static_cast<const void *>(modelPriv->sceneManager.data()),
             static_cast<const void *>(transientSM));

    // Destroy the transient view. Its scene manager's destroyed() must drive
    // updateSceneManagerForImportScene() on the surviving view and re-adopt the shared scene.
    QVERIFY(root->setProperty("loadTransientView", false));
    QVERIFY(waitForFrames(view.data(), 3));
    QCoreApplication::processEvents();
    QVERIFY(waitForFrames(view.data(), 1));

    // The shared scene must be re-homed on the surviving view rather than left orphaned. If the
    // destroyed()->updateSceneManagerForImportScene contract breaks, sceneManager stays null here.
    // (void* comparison so a failure does not dereference a possibly-freed manager when formatting).
    QVERIFY(modelPriv->sceneManager);
    QCOMPARE(static_cast<const void *>(modelPriv->sceneManager.data()),
             static_cast<const void *>(staticSM));
}

QTEST_MAIN(tst_ImportSceneLifetime)
#include "tst_importscenelifetime.moc"
