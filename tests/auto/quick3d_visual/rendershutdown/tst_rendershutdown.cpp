// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QSignalSpy>
#include <QQuickView>
#include <QQuickItem>
#include <QThread>

#include <private/qquickwindow_p.h>

#include "../shared/util.h"

class tst_rendershutdown : public QQuick3DDataTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void teardownDoesNotUseFreedInlineRenderer();
    void sceneGraphInvalidationDoesNotUseFreedInlineRenderer();
};

void tst_rendershutdown::initTestCase()
{
    QQuick3DDataTest::initTestCase();
    if (!initialized())
        return;
}

void tst_rendershutdown::teardownDoesNotUseFreedInlineRenderer()
{
    QScopedPointer<QQuickView> view(createView(QStringLiteral("inlinerendershutdown.qml"), QSize(200, 200)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));
    QVERIFY(view->rootObject());
    // Two frames so the renderer, its layer and the reflection/shadow map managers exist.
    QVERIFY(waitForFrames(view.data(), 2));

    // Destroying the view tears down the window and its scene graph on the render
    // thread. Without the fix, onInvalidated() reads the freed renderer here.
    view.reset();

    // Let the render thread finish releasing the scene graph.
    QCoreApplication::processEvents();
}

// Force the scene graph to be released while the view object stays alive (as happens
// with a non-persistent scene graph when the window is hidden). This drives the same
// QSGRenderContext::invalidated -> onInvalidated -> getRenderer() path, and the
// subsequent re-show must allocate a fresh render node instead of reusing a dangling
// m_renderNode.
void tst_rendershutdown::sceneGraphInvalidationDoesNotUseFreedInlineRenderer()
{
    QScopedPointer<QQuickView> view(createView(QStringLiteral("inlinerendershutdown.qml"), QSize(200, 200)));
    QVERIFY(view);
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));
    QVERIFY(view->rootObject());
    QVERIFY(waitForFrames(view.data(), 2));

    // Only the threaded render loop emits sceneGraphInvalidated on hide; the basic
    // (non-threaded) loop used on some CI configurations releases synchronously without
    // the signal, so gate the wait on the render loop kind (mirrors tst_qquickwindow's
    // headless() test).
    const bool threaded = QQuickWindowPrivate::get(view.data())->context->thread() != QThread::currentThread();
    QSignalSpy invalidated(view.data(), &QQuickWindow::sceneGraphInvalidated);
    view->setPersistentGraphics(false);
    view->setPersistentSceneGraph(false);
    view->hide();
    // Force the scene graph (and the Inline render node) to be released now instead of
    // waiting for a window-system occlusion event, which is not reliably delivered in CI.
    view->releaseResources();
    if (threaded)
        QTRY_VERIFY(invalidated.size() >= 1);

    // Bring the view back and render again. setupInlineRenderer() must not reuse a
    // dangling m_renderNode.
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));
    QVERIFY(waitForFrames(view.data(), 2));
}

QTEST_MAIN(tst_rendershutdown)

#include "tst_rendershutdown.moc"
