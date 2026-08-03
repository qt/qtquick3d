// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QQuickView>

#include <algorithm>

#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dobject_p.h>
#include <QtQuick3D/private/qquick3dcontentlayer_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendernode_p.h>

#include "../shared/util.h"

// Regression test for particles ignoring the content-layer (Node::layers)
// camera filter. The layers property is not inherited, and the render nodes
// that represent particles are produced by internal update nodes parented to
// the system; the system value has to be forwarded to them explicitly. User
// content instantiated from a delegate is not touched by the system and
// keeps its own layers assignment.
class tst_ParticleLayers : public QQuick3DDataTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void systemLayersReachParticleRenderNodes();
    void layersChangePropagatesDynamically();

private:
    using Layer = QQuick3DContentLayer;

    QQuickView *load();
    static QQuick3DNode *system(QObject *root);
    static void collectTags(const QSSGRenderNode *node, QSSGRenderGraphObject::Type type, QList<quint32> &out);
    static QList<quint32> tagsForType(QQuick3DNode *system, QSSGRenderGraphObject::Type type);
    void verifyLayerTags(QQuick3DNode *system, quint32 expected);
};

void tst_ParticleLayers::initTestCase()
{
    QQuick3DDataTest::initTestCase();
    if (!initialized())
        return;
}

QQuickView *tst_ParticleLayers::load()
{
    QQuickView *view = createView(QStringLiteral("particlelayers.qml"), QSize(200, 200));
    if (!view)
        return nullptr;
    if (!QTest::qWaitForWindowExposed(view)) {
        delete view;
        return nullptr;
    }
    // Let the system emit and the render nodes sync over a few frames.
    waitForFrames(view, 3);
    return view;
}

QQuick3DNode *tst_ParticleLayers::system(QObject *root)
{
    return root->findChild<QQuick3DNode *>(QStringLiteral("psystem"));
}

void tst_ParticleLayers::collectTags(const QSSGRenderNode *node, QSSGRenderGraphObject::Type type, QList<quint32> &out)
{
    if (!node)
        return;
    if (node->type == type)
        out.append(node->tag.value());
    for (const auto &child : node->children)
        collectTags(&child, type, out);
}

QList<quint32> tst_ParticleLayers::tagsForType(QQuick3DNode *system, QSSGRenderGraphObject::Type type)
{
    QList<quint32> tags;
    if (!system)
        return tags;
    auto *sysNode = static_cast<QSSGRenderNode *>(QQuick3DObjectPrivate::get(system)->spatialNode);
    collectTags(sysNode, type, tags);
    return tags;
}

// Checks both kinds of renderables the systems' particles produce: the
// Particles node of the sprite particle follows the system's layers, while
// the Model nodes of the compound ModelParticle3D delegate are user content
// and keep their own assignment (one explicit, one the default), no matter
// what the system's layers value is.
void tst_ParticleLayers::verifyLayerTags(QQuick3DNode *system, quint32 expected)
{
    QList<quint32> particleTags;
    QList<quint32> modelTags;
    // The nodes appear once the emitter has produced particles; give it a
    // couple of extra frames if needed.
    QTRY_VERIFY(!(particleTags = tagsForType(system, QSSGRenderGraphObject::Type::Particles)).isEmpty());
    QTRY_VERIFY((modelTags = tagsForType(system, QSSGRenderGraphObject::Type::Model)).size() >= 2);
    for (quint32 tag : std::as_const(particleTags))
        QCOMPARE(tag, expected);
    std::sort(modelTags.begin(), modelTags.end());
    const QList<quint32> expectedModelTags { quint32(Layer::Layer0), quint32(Layer::Layer5) };
    QCOMPARE(modelTags, expectedModelTags);
}

// The particle render nodes carry the system's layer mask, not the default
// Layer0, so the camera layer filter can include or exclude them.
void tst_ParticleLayers::systemLayersReachParticleRenderNodes()
{
    QScopedPointer<QQuickView> view(load());
    QVERIFY(view);
    QQuick3DNode *psystem = system(view->rootObject());
    QVERIFY(psystem);
    QCOMPARE(psystem->layers(), int(Layer::Layer2));

    verifyLayerTags(psystem, quint32(Layer::Layer2));
}

// Changing the system's layers at runtime re-stamps the particle render nodes.
void tst_ParticleLayers::layersChangePropagatesDynamically()
{
    QScopedPointer<QQuickView> view(load());
    QVERIFY(view);
    QQuick3DNode *psystem = system(view->rootObject());
    QVERIFY(psystem);

    verifyLayerTags(psystem, quint32(Layer::Layer2));
    if (QTest::currentTestFailed())
        return;

    psystem->setLayers(int(Layer::Layer3));
    QCoreApplication::processEvents();
    QVERIFY(waitForFrames(view.data(), 2));

    verifyLayerTags(psystem, quint32(Layer::Layer3));
}

QTEST_MAIN(tst_ParticleLayers)
#include "tst_particlelayers.moc"
