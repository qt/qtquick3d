// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderroot_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdata_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>


class tst_NodeIndexing : public QObject
{
    Q_OBJECT

public:
    tst_NodeIndexing() = default;
    ~tst_NodeIndexing() = default;

private slots:
    void initTestCase();
    void testIndexingWithChildren();
    void testIndexingWithMultipleViews();
    void testIndexingWithImportScene();
    void testVersionWrapAround();
    void testActiveStateSuppressesReindex();
    void testRemoveFromGraphClearsRootNodeRefOnChildren();
    void testImportSceneStateAfterAddRemove();
    void testImportSceneRemovePreservesSharedState();

private:
    static void removeFromLayer(QSSGRenderLayer &layer, std::vector<QSSGRenderNode *> &nodes);

    static void buildBasicNodeHierarchy(QSSGRenderNode *parent, std::vector<QSSGRenderNode *> &outList);

    QSSGRenderRoot rootNode;

    // 10 nodes with 2 children each means we have a total of 30 nodes
    static constexpr int initialNodeCount = 10 + (10 * 2);
};

void tst_NodeIndexing::initTestCase()
{

}

void tst_NodeIndexing::testIndexingWithChildren()
{
    QSSGRenderLayer layer;

    layer.ref(&rootNode);
    rootNode.addChild(layer);

    std::vector<QSSGRenderNode *> nodes;

    QVERIFY(layer.children.isEmpty());
    QVERIFY(layer.rootNode == &rootNode);

    buildBasicNodeHierarchy(&layer, nodes);

    QVERIFY(!nodes.empty());

    rootNode.reindex();

    std::unordered_set<quint32> nodeIndices;
    for (auto *node : nodes) {
        QVERIFY(node->h.hasId());
        nodeIndices.insert(node->h.index());
    }

    QCOMPARE(nodeIndices.size(), initialNodeCount);

    removeFromLayer(layer, nodes);
    rootNode.removeChild(layer);
    qDeleteAll(nodes);
}

void tst_NodeIndexing::testIndexingWithMultipleViews()
{
    // Multiple views means we have more then one layer
    constexpr size_t LayerCount = 3;
    std::array<QSSGRenderLayer, LayerCount> layers;

    for (auto &layer : layers) {
        QVERIFY(layer.children.isEmpty());
        layer.ref(&rootNode);
    }

    std::array<std::vector<QSSGRenderNode *>, LayerCount> layerNodes;

    // Add nodes to the extra layers
    for (size_t i = 0; i < LayerCount; ++i) {
        buildBasicNodeHierarchy(&layers[i], layerNodes[i]);
        QVERIFY(!layerNodes[i].empty());
    }

    // Add the layers to the root node
    for (auto &layer : layers) {
        rootNode.addChild(layer);
        QVERIFY(layer.rootNode == &rootNode);
    }

    // Reindex the root node, which should index all nodes in all layers
    rootNode.reindex();

    std::set<quint32> nodeIndices;
    // Check that all nodes in the testNodes and layerNodes have unique indices
    for (const auto &nodes : layerNodes) {
        for (auto *node : nodes) {
            QVERIFY(node->h.hasId());
            nodeIndices.insert(node->h.index());
        }
    }

    // Check that we have unique indices for all nodes
    QCOMPARE(nodeIndices.size(), initialNodeCount * LayerCount);

    quint32 expectedIndex = 1; // Start from 1 because the first index is reserved for the root/layer node(s)
    for (auto begin = nodeIndices.begin(), end = nodeIndices.end(); begin != end; ++begin) {
        // Check that the indices are without gaps
        QCOMPARE(expectedIndex, *begin);
        ++expectedIndex;
    }

    // Now remove all nodes from their layers
    for (size_t i = 0; i < LayerCount; ++i) {
        removeFromLayer(layers[i], layerNodes[i]);
        QVERIFY(layers[i].children.isEmpty());
        rootNode.removeChild(layers[i]);
    }

    for (auto &nodes : layerNodes)
        qDeleteAll(nodes);
}

void tst_NodeIndexing::testIndexingWithImportScene()
{
    // Multiple views means we have more then one layer
    constexpr size_t LayerCount = 3;
    std::array<QSSGRenderLayer, LayerCount> layers;

    for (auto &layer : layers) {
        QVERIFY(layer.children.isEmpty());
        layer.ref(&rootNode);
    }

    // In this case we'll have two sets of nodes, one for the main layer
    // and one for the import scene layers.
    std::array<std::vector<QSSGRenderNode *>, LayerCount> layerNodes;

    auto &mainLayerNodes = layerNodes[0];
    auto &mainLayer = layers[0];

    std::unique_ptr<QSSGRenderNode> sceneRoot(new QSSGRenderNode(QSSGRenderNode::Type::Node));
    mainLayer.addChild(*sceneRoot);

    // Build the main layer with the initial nodes
    buildBasicNodeHierarchy(sceneRoot.get(), mainLayerNodes);

    for (size_t i = 1; i < LayerCount; ++i) {
        // For the other layers, we'll just copy the main layer nodes
        layerNodes[i] = mainLayerNodes;
        // Create a new layer for the import scene
        auto &currentLayer = layers[i];
        QVERIFY(currentLayer.children.isEmpty());
        // Replicate how we do import scenes
        currentLayer.children.m_head = currentLayer.children.m_tail = sceneRoot.get();

    }

    // Add the layers to the root node
    for (auto &layer : layers) {
        rootNode.addChild(layer);
        QVERIFY(layer.rootNode == &rootNode);
    }

    // Reindex the root node, which should index all nodes in all layers
    rootNode.reindex();

    std::set<quint32> nodeIndices;
    size_t insertedNodeCount = 0;
    // Check that all nodes in the testNodes and layerNodes have unique indices
    for (const auto &nodes : layerNodes) {
        for (auto *node : nodes) {
            QVERIFY(node->h.hasId());
            ++insertedNodeCount;
            nodeIndices.insert(node->h.index());
        }
    }

    // Need to add the scene root as well
    QVERIFY(sceneRoot && sceneRoot->h.hasId());
    ++insertedNodeCount;
    nodeIndices.insert(sceneRoot->h.index());

    // Check that we have unique indices for all nodes
    // The total number of nodes should be initialNodeCount * LayerCount
    // (It would be better to get the numbers from the reindex function,
    // but this is good enough for now).
    QCOMPARE(insertedNodeCount, (initialNodeCount * LayerCount) + 1 /* for the scene root node */);
    // But the indices should only be as many as the main layer nodes
    QCOMPARE(nodeIndices.size(), mainLayerNodes.size() + 1 /* for the scene root node */);
    QCOMPARE(nodeIndices.size(), initialNodeCount + 1 /* for the scene root node */);

    // Check that the indices are in the expected range
    quint32 expectedIndex = 1; // Start from 1 because the first index is reserved for the root/layer node(s)
    for (auto begin = nodeIndices.begin(), end = nodeIndices.end(); begin != end; ++begin) {
        // Check that the indices are without gaps
        QCOMPARE(expectedIndex, *begin);
        ++expectedIndex;
    }

    // Now remove all nodes from their layers
    removeFromLayer(mainLayer, mainLayerNodes);
    mainLayer.removeChild(*sceneRoot);
    QVERIFY(mainLayer.children.isEmpty());

    // Ensure that the scene root isn't referenced by any layer
    for (size_t i = 0; i < LayerCount; ++i) {
        auto &layer = layers[i];
        layer.children.clear(); // Clear the list (or the sceneRoot will be used after it's deleted)
        rootNode.removeChild(layer);
    }

    qDeleteAll(mainLayerNodes);
}

void tst_NodeIndexing::testVersionWrapAround()
{
    using VersionType = QSSGRenderNodeVersionType;

    QSSGRenderLayer layer;
    layer.ref(&rootNode);
    rootNode.addChild(layer);

    auto *node = new QSSGRenderNode(QSSGRenderNode::Type::Node);
    layer.addChild(*node);

    rootNode.reindex();
    QVERIFY(node->h.hasId());

    // Bypass the counter directly to just below the maximum, avoiding the need
    // to iterate through all 65535 values in the test.
    auto &gnd = *rootNode.globalNodeData();
    gnd.m_version = std::numeric_limits<VersionType>::max() - 1;

    // Reindex to reach the maximum version value.
    rootNode.reindex();
    QVERIFY(node->h.hasId());
    QCOMPARE(node->h.version(), std::numeric_limits<VersionType>::max());
    QCOMPARE(layer.lh.version(), std::numeric_limits<VersionType>::max());

    // One more reindex wraps max + 1 to 0; the guard skips 0 and bumps to 1.
    // Nodes and the layer handle must remain valid.
    rootNode.reindex();
    QVERIFY(node->h.hasId());
    QCOMPARE(node->h.version(), VersionType(1));
    QCOMPARE(layer.lh.version(), VersionType(1));

    layer.removeChild(*node);
    delete node;
    rootNode.removeChild(layer);
}

void tst_NodeIndexing::testActiveStateSuppressesReindex()
{
    QSSGRenderLayer layer;
    layer.ref(&rootNode);
    rootNode.addChild(layer);

    std::vector<QSSGRenderNode *> nodes;
    buildBasicNodeHierarchy(&layer, nodes);

    rootNode.reindex();

    auto &gnd = *rootNode.globalNodeData();
    const auto versionBefore = gnd.m_version;

    // Capture the current handle state of every node.
    std::vector<std::pair<QSSGRenderNodeVersionType, quint32>> snapshot;
    snapshot.reserve(nodes.size());
    for (auto *node : nodes)
        snapshot.push_back({ node->h.version(), node->h.index() });

    // nodes[0] is added directly to the layer; nodes[1] and nodes[2] are its children.
    // nodes[3..5] form a sibling group with no relation to nodes[0..2].
    nodes.front()->setState(QSSGRenderNode::LocalState::Active, false);

    // The GND version must not have changed — no reindex should have occurred.
    QCOMPARE(gnd.m_version, versionBefore);

    // All handles must remain valid and unchanged.
    for (size_t i = 0; i < nodes.size(); ++i) {
        QCOMPARE(nodes[i]->h.version(), snapshot[i].first);
        QCOMPARE(nodes[i]->h.index(),   snapshot[i].second);
    }

    // The changed node and its children must have ActiveDirty set.
    // markDirty() propagates ActiveDirty to descendants via SubtreeUpdateMask.
    using DirtyFlag = QSSGRenderNode::DirtyFlag;
    QVERIFY( nodes[0]->isDirty(DirtyFlag::ActiveDirty));
    QVERIFY( nodes[1]->isDirty(DirtyFlag::ActiveDirty));
    QVERIFY( nodes[2]->isDirty(DirtyFlag::ActiveDirty));

    // The sibling sub-tree must not be affected.
    QVERIFY(!nodes[3]->isDirty(DirtyFlag::ActiveDirty));
    QVERIFY(!nodes[4]->isDirty(DirtyFlag::ActiveDirty));
    QVERIFY(!nodes[5]->isDirty(DirtyFlag::ActiveDirty));

    removeFromLayer(layer, nodes);
    rootNode.removeChild(layer);
    qDeleteAll(nodes);
}

void tst_NodeIndexing::removeFromLayer(QSSGRenderLayer &layer, std::vector<QSSGRenderNode *> &nodes)
{
    for (auto *node : nodes) {
        QSSGRenderNode *parent = node->parent;
        if (parent)
            parent->removeChild(*node);
        else
            layer.removeChild(*node);
    }

    // Check that all nodes no longer have parents and
    // their node index is reset.
    for (auto *node : nodes) {
        QCOMPARE(node->parent, nullptr);
        QVERIFY(!node->h.hasId());
    }
}

void tst_NodeIndexing::buildBasicNodeHierarchy(QSSGRenderNode *parent, std::vector<QSSGRenderNode *> &outList)
{
    Q_ASSERT(parent);
    Q_ASSERT(outList.size() == 0);

    for (int i = 0; i < initialNodeCount; ++i) {
        auto *node = new QSSGRenderNode(QSSGRenderNode::Type::Node);
        outList.push_back(node);
    }

    // Add nodes to the second layer
    for (int i = 0; i < initialNodeCount; i += 3) {
        auto *node = outList[i];
        QVERIFY(!node->h.hasId());
        parent->addChild(*node);

        // Add two children to each node
        for (int j = 0; j < 2; ++j) {
            auto *childNode = outList[i + j + 1];
            QVERIFY(!childNode->h.hasId());
            node->addChild(*childNode);
        }
    }
}

void tst_NodeIndexing::testRemoveFromGraphClearsRootNodeRefOnChildren()
{
    // When removeFromGraph() orphans its direct children it must also clear
    // their rootNodeRef, just as removeChild() does for the removed node
    // itself. Without the fix, an orphaned node P retains a live
    // rootNodeRef. If the root is then destroyed and a grandchild C calls
    // removeFromGraph() it will execute P->removeChild(C), which accesses
    // P->rootNodeRef via QSSGRenderRoot::get() — a use-after-free crash.
    //
    // Hierarchy under test:
    //   Root -> Layer -> sceneRoot -> GP -> P -> C
    //
    // sceneRoot is a plain node added directly to the layer. GP, P, and C
    // form a chain beneath it. We remove GP from the graph and assert that
    // P's rootNodeRef is null afterwards.  We also exercise
    // C->removeFromGraph() to verify it is safe when P->rootNodeRef is null
    // (get() returns nullptr so markDirty is not called).
    //
    // Note: layers do not set the parent pointer on their direct children
    // (see the Layer / ImportScene special case in addChild), so sceneRoot
    // is needed to give GP a non-null parent that removeFromGraph() can
    // walk up through.

    QSSGRenderLayer layer;
    layer.ref(&rootNode);
    rootNode.addChild(layer);

    auto *sceneRoot = new QSSGRenderNode;
    auto *gp = new QSSGRenderNode;
    auto *p = new QSSGRenderNode;
    auto *c = new QSSGRenderNode;

    layer.addChild(*sceneRoot);
    sceneRoot->addChild(*gp);
    gp->addChild(*p);
    p->addChild(*c);

    rootNode.reindex();

    // Pre-condition: all nodes must have a rootNodeRef pointing to the root.
    QVERIFY(sceneRoot->rootNodeRef != nullptr);
    QVERIFY(gp->rootNodeRef != nullptr);
    QVERIFY(p->rootNodeRef != nullptr);
    QVERIFY(c->rootNodeRef != nullptr);

    // Remove GP — this calls sceneRoot->removeChild(GP) which clears
    // GP->rootNodeRef, then orphans P: P->parent = nullptr.
    // The fix requires that P->rootNodeRef is also cleared here.
    gp->removeFromGraph();

    // GP itself must be fully detached by sceneRoot->removeChild(GP).
    QCOMPARE(gp->parent, nullptr);
    QCOMPARE(gp->rootNodeRef, nullptr);

    // P was orphaned by the loop inside GP->removeFromGraph().
    // Its rootNodeRef must be null — this is what the fix ensures.
    QCOMPARE(p->parent, nullptr);
    QCOMPARE(p->rootNodeRef, nullptr);

    // C is still a child of P at this point (P->removeFromGraph not called).
    // Calling C->removeFromGraph() will invoke P->removeChild(C).
    // With P->rootNodeRef == nullptr, get() returns null and markDirty is
    // skipped — this must not crash.
    c->removeFromGraph();

    QCOMPARE(c->parent, nullptr);
    QCOMPARE(c->rootNodeRef, nullptr);

    delete c;
    delete p;
    delete gp;
    layer.removeChild(*sceneRoot);
    delete sceneRoot;
    rootNode.removeChild(layer);
}

void tst_NodeIndexing::testImportSceneStateAfterAddRemove()
{
    // setImportScene()/removeImportScene() inject and detach a shared scene
    // through addChild() and the dummy import-scene node. Verify that:
    //  - the dummy import-scene node is inserted at the *front* of the layer's
    //    child list, so the imported scene always gets the lowest indices,
    //  - the imported node keeps its existing parent (it belongs to its home
    //    tree) and gets the Imported state plus a rootNodeRef, and
    //  - removeImportScene() only resets the handle 'h', leaving the Imported
    //    state, rootNodeRef and parent untouched.

    QSSGRenderLayer mainLayer;
    QSSGRenderLayer importerLayer;
    mainLayer.ref(&rootNode);
    importerLayer.ref(&rootNode);

    // Home tree: mainLayer -> container -> sceneRoot -> { leafA, leafB }.
    // container is a SceneRoot so it gives sceneRoot a real, non-null parent
    // without tripping the "already part of another scene graph" warning.
    auto *container = new QSSGRenderNode(QSSGRenderNode::Type::SceneRoot);
    auto *sceneRoot = new QSSGRenderNode(QSSGRenderNode::Type::Node);
    auto *leafA = new QSSGRenderNode(QSSGRenderNode::Type::Node);
    auto *leafB = new QSSGRenderNode(QSSGRenderNode::Type::Node);
    mainLayer.addChild(*container);
    container->addChild(*sceneRoot);
    sceneRoot->addChild(*leafA);
    sceneRoot->addChild(*leafB);
    QCOMPARE(sceneRoot->parent, container);

    // A plain node added to the importer layer before the import: the dummy
    // import-scene node must end up *before* it in the child list.
    auto *regular = new QSSGRenderNode(QSSGRenderNode::Type::Node);
    importerLayer.addChild(*regular);

    rootNode.addChild(mainLayer);
    rootNode.addChild(importerLayer);

    importerLayer.setImportScene(*sceneRoot);

    // The dummy import-scene node exists and is at the front of the children.
    auto *dummy = importerLayer.importSceneNode;
    QVERIFY(dummy != nullptr);
    QVERIFY(!importerLayer.children.isEmpty());
    QCOMPARE(&importerLayer.children.front(), dummy);
    // The imported node is the (only) child of the dummy node.
    QVERIFY(!dummy->children.isEmpty());
    QCOMPARE(&dummy->children.back(), sceneRoot);

    // Import bookkeeping on the shared node.
    QVERIFY(sceneRoot->getLocalState(QSSGRenderNode::LocalState::Imported));
    QVERIFY(sceneRoot->rootNodeRef != nullptr);
    QCOMPARE(sceneRoot->parent, container); // parent must be untouched

    rootNode.reindex();

    // Everything got an index, and the import-scene node indexes before the
    // plain sibling node (front insertion).
    QVERIFY(dummy->h.hasId());
    QVERIFY(sceneRoot->h.hasId());
    QVERIFY(regular->h.hasId());
    QVERIFY(dummy->h.index() < regular->h.index());

    // Detach the import scene.
    importerLayer.removeImportScene(*sceneRoot);

    // The dummy's child list is cleared and the handle is reset...
    QVERIFY(dummy->children.isEmpty());
    QVERIFY(!sceneRoot->h.hasId());
    // ...but the Imported state, rootNodeRef and parent must be preserved.
    QVERIFY(sceneRoot->getLocalState(QSSGRenderNode::LocalState::Imported));
    QVERIFY(sceneRoot->rootNodeRef != nullptr);
    QCOMPARE(sceneRoot->parent, container);

    // Teardown.
    importerLayer.removeChild(*regular);
    sceneRoot->removeChild(*leafA);
    sceneRoot->removeChild(*leafB);
    container->removeChild(*sceneRoot);
    mainLayer.removeChild(*container);
    rootNode.removeChild(mainLayer);
    rootNode.removeChild(importerLayer);
    delete leafA;
    delete leafB;
    delete sceneRoot;
    delete container;
    delete regular;
}

void tst_NodeIndexing::testImportSceneRemovePreservesSharedState()
{
    // The same imported scene may be imported by more than one view. Removing
    // it from one importer must NOT clear the Imported state or rootNodeRef:
    // neither is restored by the update/reindex, and the other importing view
    // still relies on them.

    QSSGRenderLayer mainLayer;
    QSSGRenderLayer importerB;
    QSSGRenderLayer importerC;
    mainLayer.ref(&rootNode);
    importerB.ref(&rootNode);
    importerC.ref(&rootNode);

    auto *container = new QSSGRenderNode(QSSGRenderNode::Type::SceneRoot);
    auto *sceneRoot = new QSSGRenderNode(QSSGRenderNode::Type::Node);
    mainLayer.addChild(*container);
    container->addChild(*sceneRoot);

    rootNode.addChild(mainLayer);
    rootNode.addChild(importerB);
    rootNode.addChild(importerC);

    importerB.setImportScene(*sceneRoot);
    importerC.setImportScene(*sceneRoot);
    rootNode.reindex();

    auto *dummyB = importerB.importSceneNode;
    auto *dummyC = importerC.importSceneNode;
    QVERIFY(dummyB != nullptr);
    QVERIFY(dummyC != nullptr);
    QVERIFY(sceneRoot->getLocalState(QSSGRenderNode::LocalState::Imported));
    QVERIFY(sceneRoot->rootNodeRef != nullptr);
    QVERIFY(sceneRoot->h.hasId());

    // Remove from B only.
    importerB.removeImportScene(*sceneRoot);

    // B no longer references the scene...
    QVERIFY(dummyB->children.isEmpty());
    // ...but C still does, and the shared state survives.
    QVERIFY(!dummyC->children.isEmpty());
    QCOMPARE(&dummyC->children.back(), sceneRoot);
    QVERIFY(sceneRoot->getLocalState(QSSGRenderNode::LocalState::Imported));
    QVERIFY(sceneRoot->rootNodeRef != nullptr);

    // Teardown.
    importerC.removeImportScene(*sceneRoot);
    container->removeChild(*sceneRoot);
    mainLayer.removeChild(*container);
    rootNode.removeChild(mainLayer);
    rootNode.removeChild(importerB);
    rootNode.removeChild(importerC);
    delete sceneRoot;
    delete container;
}

QTEST_APPLESS_MAIN(tst_NodeIndexing)

#include "tst_nodeindexing.moc"
