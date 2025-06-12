// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgrenderdata_p.h"

#if QT_CONFIG(thread)
#include <QtCore/qthreadpool.h>
#endif // QT_CONFIG(thread)

#include "graphobjects/qssgrenderroot_p.h"
#include "graphobjects/qssgrenderlayer_p.h"
#include "qssgrendercontextcore.h"
#include "qssgrenderer_p.h"
#include "resourcemanager/qssgrenderbuffermanager_p.h"

QT_BEGIN_NAMESPACE

static void reindexChildNodes(QSSGRenderNode &node, const quint32 version, quint32 &dfsIdx, size_t &count)
{
    Q_ASSERT(node.type != QSSGRenderNode::Type::Layer);
    if (node.type != QSSGRenderNode::Type::Layer) {
        // Note: In the case of import scenes the version and index might already
        // have been set. We therefore assume nodes with same version is already
        // indexed.
        if (node.h.version() != version)
            node.h = QSSGRenderNodeHandle(0, version, dfsIdx);
        for (QSSGRenderNode &chld : node.children)
            reindexChildNodes(chld, version, ++dfsIdx, ++count);
    }
}

static void reindexLayerChildNodes(QSSGRenderLayer &layer, const quint32 version, quint32 &dfsIdx, size_t &count)
{
    Q_ASSERT(layer.type == QSSGRenderNode::Type::Layer);
    if (layer.type == QSSGRenderNode::Type::Layer) {
        layer.h = QSSGRenderNodeHandle(0, version, 0); // Layer nodes are always indexed at 0;
        for (QSSGRenderNode &chld : layer.children)
            reindexChildNodes(chld, version, ++dfsIdx, ++count);
    }
}

static void reindex(QSSGRenderRoot *rootNode, const quint32 version, quint32 &dfsIdx, size_t &count)
{
    if (rootNode) {
        Q_ASSERT(rootNode->type == QSSGRenderNode::Type::Root);
        Q_ASSERT(dfsIdx == 0);
        rootNode->h = QSSGRenderNodeHandle(0, version, 0);
        for (QSSGRenderNode &chld : rootNode->children) // These should be layer nodes
            reindexLayerChildNodes(static_cast<QSSGRenderLayer &>(chld), version, dfsIdx, count);
    }
}

enum class Insert { Back, Indexed };
template <Insert insert = Insert::Back>
static void collectChildNodesFirst(QSSGRenderNode &node, QSSGGlobalRenderNodeData::NodeStore &outList, [[maybe_unused]] size_t &idx)
{
    Q_ASSERT_X(node.type != QSSGRenderNode::Type::Layer, Q_FUNC_INFO, "Unexpected Layer node in child list!");
    if constexpr (insert == Insert::Indexed)
        outList[idx++] = &node;
    else
        outList.push_back(&node);
    for (QSSGRenderNode &chld : node.children)
        collectChildNodesFirst<insert>(chld, outList, idx);
}

template <Insert insert = Insert::Back>
static void collectChildNodesSecond(QSSGRenderNode &node, QSSGGlobalRenderNodeData::NodeStore &outList, [[maybe_unused]] size_t &idx)
{
    if (node.getGlobalState(QSSGRenderNode::GlobalState::Active)) {
        if constexpr (insert == Insert::Indexed)
            outList[idx++] = &node;
        else
            outList.push_back(&node);
        for (QSSGRenderNode &chld : node.children)
            collectChildNodesSecond<insert>(chld, outList, idx);
    }
}

enum class Discard { None, Inactive };
template <Discard discard = Discard::None, Insert insert = Insert::Back>
static void collectLayerChildNodes(QSSGRenderLayer *layer, QSSGGlobalRenderNodeData::NodeStore &outList, [[maybe_unused]] size_t &idx)
{
    Q_ASSERT(layer->type == QSSGRenderNode::Type::Layer);
    if (layer) {
        for (QSSGRenderNode &chld : layer->children) {
            if constexpr (discard == Discard::Inactive)
                collectChildNodesSecond<insert>(chld, outList, idx);
            else
                collectChildNodesFirst<insert>(chld, outList, idx);
        }
    }

    if  constexpr (insert == Insert::Back)
        idx = outList.size();
}

template <QSSGRenderDataHelpers::Strategy Strategy>
static bool calcGlobalNodeDataIndexedImpl(QSSGRenderNode *node,
                                          const quint32 version,
                                          QSSGGlobalRenderNodeData::GlobalTransformStore &globalTransforms,
                                          QSSGGlobalRenderNodeData::GlobalOpacityStore &globalOpacities)
{
    using DirtyFlag = QSSGRenderNode::DirtyFlag;
    using FlagT = QSSGRenderNode::FlagT;
    constexpr DirtyFlag TransformAndOpacityDirty = DirtyFlag(FlagT(DirtyFlag::TransformDirty) | FlagT(DirtyFlag::OpacityDirty));

    if (Q_UNLIKELY(!node || (node->h.version() != version)))
        return false;

    constexpr bool forcedRebuilf = (Strategy == QSSGRenderDataHelpers::Strategy::Initial);
    bool retval = forcedRebuilf || node->isDirty(TransformAndOpacityDirty);

    if (retval) {
        const auto idx = node->h.index();

        auto &globalTransform = globalTransforms[idx];
        auto &globalOpacity = globalOpacities[idx];
        globalOpacity = node->localOpacity;
        globalTransform = node->localTransform;

        if (QSSGRenderNode *parent = node->parent) {
            const auto pidx = parent->h.index();
            const auto pOpacity = globalOpacities[pidx];
            globalOpacity *= pOpacity;

            if (parent->type != QSSGRenderGraphObject::Type::Layer) {
                const auto pTransform = globalTransforms[pidx];
                globalTransform = pTransform * node->localTransform;
            }
        }
        // Clear dirty flags (Transform, Opacity, Active, Pickable)
        node->clearDirty(TransformAndOpacityDirty);
    }

    // We always clear dirty in a reasonable manner but if we aren't active
    // there is no reason to tell the universe if we are dirty or not.
    return retval;
}

QSSGRenderDataHelpers::GlobalStateResult QSSGRenderDataHelpers::updateGlobalNodeState(QSSGRenderNode *node, const quint32 version)
{
    using LocalState = QSSGRenderNode::LocalState;
    using GlobalState = QSSGRenderNode::GlobalState;
    using DirtyFlag = QSSGRenderNode::DirtyFlag;
    using FlagT = QSSGRenderNode::FlagT;

    constexpr DirtyFlag ActiveOrPickableDirty = DirtyFlag(FlagT(DirtyFlag::ActiveDirty) | FlagT(DirtyFlag::PickableDirty));

    if (Q_UNLIKELY(!node || (node->h.version() != version)))
        return GlobalStateResult::None;

    const bool activeDirty = node->isDirty(DirtyFlag::ActiveDirty);
    const bool pickableDirty = node->isDirty(DirtyFlag::PickableDirty);

    const bool updateState = activeDirty || pickableDirty;

    if (updateState) {
        const QSSGRenderNode *parent = node->parent;
        const bool hasParent = (parent != nullptr);
        const bool globallyActive = node->getLocalState(LocalState::Active) && (!hasParent || parent->getGlobalState(GlobalState::Active));
        node->flags = globallyActive ? (node->flags | FlagT(GlobalState::Active)) : (node->flags & ~FlagT(GlobalState::Active));
        const bool globallyPickable = node->getLocalState(LocalState::Pickable) || (hasParent && parent->getGlobalState(GlobalState::Pickable));
        node->flags = globallyPickable ? (node->flags | FlagT(GlobalState::Pickable)) : (node->flags & ~FlagT(GlobalState::Pickable));

        // Clear dirty flags (Transform, Opacity, Active, Pickable)
        node->clearDirty(ActiveOrPickableDirty);
    }

    return GlobalStateResult((activeDirty << 1) | (pickableDirty << 2));
}

bool QSSGRenderDataHelpers::calcInstanceTransforms(QSSGRenderNode *node,
                                                   const quint32 version,
                                                   QSSGGlobalRenderNodeData::GlobalTransformStore &globalTransforms,
                                                   QSSGGlobalRenderNodeData::InstanceTransformStore &instanceTransforms)
{
    if (Q_UNLIKELY(!node || (node->h.version() != version)))
        return false;

    constexpr bool retval = true;

    // NOTE: We've already calculated the global states and transforms at this point
    // so if the node isn't active we don't need to do anything.
    // We're also assuming the node list is stored depth first order.
    const auto idx = node->h.index();
    QSSGRenderNode *parent = node->parent;
    if (parent && parent->type != QSSGRenderGraphObject::Type::Layer && node->getLocalState(QSSGRenderNode::LocalState::Active)) {
        const auto pidx = parent->h.index();
        const auto &pGlobalTransform = globalTransforms[pidx];
        QSSGRenderNode *instanceRoot = node->instanceRoot;
        if (instanceRoot == node) {
            instanceTransforms[idx] = { node->localTransform, pGlobalTransform };
        } else if (instanceRoot) {
            auto &[nodeInstanceLocalTransform, nodeInstanceGlobalTransform] = instanceTransforms[idx];
            auto &[instanceRootLocalTransform, instanceRootGlobalTransform] = instanceTransforms[instanceRoot->h.index()];
            nodeInstanceGlobalTransform = instanceRootGlobalTransform;
            //### technically O(n^2) -- we could cache localInstanceTransform if every node in the
            // tree is guaranteed to have the same instance root. That would require an API change.
            nodeInstanceLocalTransform = node->localTransform;
            auto *p = parent;
            while (p) {
                if (p == instanceRoot) {
                    nodeInstanceLocalTransform = instanceRootLocalTransform * nodeInstanceLocalTransform;
                    break;
                }
                nodeInstanceLocalTransform = p->localTransform * nodeInstanceLocalTransform;
                p = p->parent;
            }
        } else {
            // By default, we do magic: translation is applied to the global instance transform,
            // while scale/rotation is local
            QMatrix4x4 globalInstanceTransform = globalTransforms[pidx];
            QMatrix4x4 localInstanceTransform = node->localTransform;
            auto &localInstanceMatrix = *reinterpret_cast<float (*)[4][4]>(localInstanceTransform.data());
            QVector3D localPos{localInstanceMatrix[3][0], localInstanceMatrix[3][1], localInstanceMatrix[3][2]};
            localInstanceMatrix[3][0] = 0;
            localInstanceMatrix[3][1] = 0;
            localInstanceMatrix[3][2] = 0;
            globalInstanceTransform = pGlobalTransform;
            globalInstanceTransform.translate(localPos);
            instanceTransforms[idx] = { localInstanceTransform, globalInstanceTransform };
        }
    } else {
        instanceTransforms[idx] = { node->localTransform, {} };
    }

    return retval;
}

QSSGGlobalRenderNodeData::QSSGGlobalRenderNodeData()
#if QT_CONFIG(thread)
    : m_threadPool(new QThreadPool)
#endif // QT_CONFIG(thread)
{

}

QSSGGlobalRenderNodeData::~QSSGGlobalRenderNodeData()
{

}

void QSSGGlobalRenderNodeData::reindex(QSSGRenderRoot *rootNode)
{
    if (rootNode) {
        quint32 dfsIdx = 0;
        m_nodeCount = 0;

        // If the window changes the window root node changes as well,
        // this check ensures we accidentally don't reindex with the
        // same version, as that can cause problems.
        // The start version should be set to the last layer's last version.
        if (m_version == rootNode->startVersion())
            m_version = rootNode->startVersion() + 1;
        else
            ++m_version;

        ::reindex(rootNode, m_version, dfsIdx, m_nodeCount);

        // Actual storage size (Some nodes, like the layers, will all use index 0).
        // NOTE: This can differ from the node count, as nodes are collected for each
        // layer. Since layers can reference nodes outside their view the node list
        // will contain duplicate nodes when import scene is used!
        m_size = dfsIdx + 1;

        globalTransforms.resize(m_size, QMatrix4x4{ Qt::Uninitialized });
        globalOpacities.resize(m_size, 1.0f);
        instanceTransforms.resize(m_size, { QMatrix4x4{ Qt::Uninitialized }, QMatrix4x4{ Qt::Uninitialized } });

        collectNodes(rootNode);
        // NOTE: If the tree was dirty we force a full rebuild of the global transforms etc. since
        //       the stored data is invalid for the new index order.
        updateGlobalState();
    }
}

QMatrix4x4 QSSGGlobalRenderNodeData::getGlobalTransform(QSSGRenderNodeHandle h, QMatrix4x4 defaultValue) const
{
    // Ensure we have an valid index.
    const bool hasId = h.hasId();
    const bool validVersion = hasId && (h.version() == m_version);
    const auto index = h.index();

    // NOTE: In effect we are returning the local transform here in some cases, which
    // is why we don't assert or have hints about the likelyhood of branching here.
    if (!validVersion || !(globalTransforms.size() > index))
        return defaultValue;

    return globalTransforms[index];
}

QMatrix4x4 QSSGGlobalRenderNodeData::getGlobalTransform(QSSGRenderNodeHandle h) const
{
    return getGlobalTransform(h, QMatrix4x4{ Qt::Uninitialized });
}

QMatrix4x4 QSSGGlobalRenderNodeData::getGlobalTransform(const QSSGRenderNode &node) const
{
    return getGlobalTransform(node.h, node.localTransform);
}

float QSSGGlobalRenderNodeData::getGlobalOpacity(QSSGRenderNodeHandle h, float defaultValue) const
{
    const bool hasId = h.hasId();
    const bool validVersion = hasId && (h.version() == m_version);
    const auto index = h.index();

    if (!validVersion || !(globalOpacities.size() > index))
        return defaultValue;

    return globalOpacities[index];
}

float QSSGGlobalRenderNodeData::getGlobalOpacity(const QSSGRenderNode &node) const
{
    return getGlobalOpacity(node.h, node.localOpacity);
}

#if QT_CONFIG(thread)
const std::unique_ptr<QThreadPool> &QSSGGlobalRenderNodeData::threadPool() const { return m_threadPool; }
#endif // QT_CONFIG(thread)

QSSGGlobalRenderNodeData::LayerNodeView QSSGGlobalRenderNodeData::getLayerNodeView(QSSGRenderLayerHandle h) const
{
    const bool hasId = h.hasId();
    const bool validVersion = hasId && (h.version() == m_version);
    const auto index = h.index();

    if (!validVersion || !(layerNodes.size() > index))
        return { };

    auto &seciont = layerNodes[index];

    return { nodes.data() + seciont.offset, qsizetype(seciont.size) };
}

QSSGGlobalRenderNodeData::LayerNodeView QSSGGlobalRenderNodeData::getLayerNodeView(const QSSGRenderLayer &layer) const
{
    return getLayerNodeView(layer.lh);
}

void QSSGGlobalRenderNodeData::collectNodes(QSSGRenderRoot *rootNode)
{
    // 1. Collect all the nodes and create views into the node storage for each layer
    // 2. Update the global state
    // 3. Update the global data
    Q_ASSERT(rootNode != nullptr);

    nodes.clear();
    nodes.resize(m_nodeCount, nullptr);
    layerNodes.clear();

    size_t idx = 0;
    quint32 layerIdx = 0;
    for (QSSGRenderNode &chld : rootNode->children) {
        Q_ASSERT(chld.type == QSSGRenderNode::Type::Layer);
        QSSGRenderLayer *layer = static_cast<QSSGRenderLayer *>(&chld);
        const size_t offset = idx;
        collectLayerChildNodes<Discard::None, Insert::Indexed>(layer, nodes, idx);
        layer->lh = QSSGRenderLayerHandle(layer->h.context(), m_version, layerIdx++);
        layerNodes.emplace_back(LayerNodeSection{offset , idx - offset});
    }

    nodes.resize(idx /* idx == next_idx == size */);
}

void QSSGGlobalRenderNodeData::updateGlobalState()
{
    // Update the active and pickable state
    for (QSSGRenderNode *node : nodes)
        QSSGRenderDataHelpers::updateGlobalNodeState(node, m_version);

    // Recalculate ALL the global transforms and opacities.
    for (QSSGRenderNode *node : nodes)
        calcGlobalNodeDataIndexedImpl<QSSGRenderDataHelpers::Strategy::Initial>(node, m_version, globalTransforms, globalOpacities);

    // FIXME: We shouldn't need to re-create all the instance transforms even when instancing isn't used...
    for (QSSGRenderNode *node : nodes)
        QSSGRenderDataHelpers::calcInstanceTransforms(node, m_version, globalTransforms, instanceTransforms);
}

QSSGGlobalRenderNodeData::InstanceTransforms QSSGGlobalRenderNodeData::getInstanceTransforms(const QSSGRenderNode &node) const
{
    return getInstanceTransforms(node.h);
}

QSSGGlobalRenderNodeData::InstanceTransforms QSSGGlobalRenderNodeData::getInstanceTransforms(QSSGRenderNodeHandle h) const
{
    const bool hasId = h.hasId();
    const bool validVersion = hasId && (h.version() == m_version);
    const auto index = h.index();

    if (!validVersion || !(instanceTransforms.size() > index))
        return { };

    return instanceTransforms[index];
}

QSSGRenderModelData::QSSGRenderModelData(const QSSGGlobalRenderNodeDataPtr &globalNodeData)
    : m_gnd(globalNodeData)
    , m_version(globalNodeData->version())
{

}

QMatrix3x3 QSSGRenderModelData::getNormalMatrix(QSSGRenderModelHandle h, QMatrix3x3 defaultValue) const
{
    const bool hasId = h.hasId();
    const bool validVersion = hasId && (h.version() == m_version);
    const auto index = h.index();
    if (!validVersion || !(normalMatrices.size() > index))
        return defaultValue;

    return normalMatrices[index];
}

QMatrix3x3 QSSGRenderModelData::getNormalMatrix(const QSSGRenderModel &model) const
{
    return getNormalMatrix(model.mh, QMatrix3x3{ Qt::Uninitialized });
}

QSSGRenderMesh *QSSGRenderModelData::getMesh(QSSGRenderModelHandle h) const
{
    const bool hasId = h.hasId();
    const bool validVersion = hasId && (h.version() == m_version);
    const auto index = h.index();

    if (!validVersion || !(meshes.size() > index))
        return nullptr;

    return meshes[index];
}

QSSGRenderMesh *QSSGRenderModelData::getMesh(const QSSGRenderModel &model) const
{
    return getMesh(model.mh);
}

QSSGRenderModelData::MaterialList QSSGRenderModelData::getMaterials(QSSGRenderModelHandle h) const
{
    const bool hasId = h.hasId();
    const bool validVersion = hasId && (h.version() == m_version);
    const auto index = h.index();

    if (!validVersion || !(materials.size() > index))
        return {};

    return materials[index];
}

QSSGRenderModelData::MaterialList QSSGRenderModelData::getMaterials(const QSSGRenderModel &model) const
{
    return getMaterials(model.mh);
}

QSSGRenderModelData::ModelViewProjections QSSGRenderModelData::getModelViewProjection(const QSSGRenderModel &model) const
{
    return getModelViewProjection(model.mh);
}

QSSGRenderModelData::ModelViewProjections QSSGRenderModelData::getModelViewProjection(QSSGRenderModelHandle h) const
{
    const bool hasId = h.hasId();
    const bool validVersion = hasId && (h.version() == m_version);
    const auto index = h.index();

    if (!validVersion || !(modelViewProjections.size() > index))
        return {};

    return modelViewProjections[index];
}

void QSSGRenderModelData::prepareMeshData(const QSSGModelsView &models, QSSGRenderer *renderer)
{
    const auto &bufferManager = renderer->contextInterface()->bufferManager();

    const bool globalPickingEnabled = QSSGRendererPrivate::isGlobalPickingEnabled(*renderer);

    for (auto *model : models) {
        // It's up to the BufferManager to employ the appropriate caching mechanisms, so
        // loadMesh() is expected to be fast if already loaded. Note that preparing
        // the same QSSGRenderModel in different QQuickWindows (possible when a
        // scene is shared between View3Ds where the View3Ds belong to different
        // windows) leads to a different QSSGRenderMesh since the BufferManager is,
        // very correctly, per window, and so per scenegraph render thread.

        // Ensure we have a mesh
        if (auto *theMesh = bufferManager->loadMesh(*model)) {
            meshes[model->mh.index()] = theMesh;
            // Completely transparent models cannot be pickable.  But models with completely
            // transparent materials still are.  This allows the artist to control pickability
            // in a somewhat fine-grained style.
            const float modelGlobalOpacity = m_gnd->getGlobalOpacity(*model);
            const bool canModelBePickable = (modelGlobalOpacity > QSSGRendererPrivate::minimumRenderOpacity)
                    && (globalPickingEnabled
                        || model->getGlobalState(QSSGRenderModel::GlobalState::Pickable));
            if (canModelBePickable) {
                // Check if there is BVH data, if not generate it
                if (!theMesh->bvh) {
                    const QSSGMesh::Mesh mesh = bufferManager->loadLightmapMesh(*model);

                    if (mesh.isValid())
                        theMesh->bvh = bufferManager->loadMeshBVH(mesh);
                    else if (model->geometry)
                        theMesh->bvh = bufferManager->loadMeshBVH(model->geometry);
                    else if (!model->meshPath.isNull())
                        theMesh->bvh = bufferManager->loadMeshBVH(model->meshPath);

                    if (theMesh->bvh) {
                        const auto &roots = theMesh->bvh->roots();
                        for (qsizetype i = 0, end = qsizetype(roots.size()); i < end; ++i)
                            theMesh->subsets[i].bvhRoot = roots[i];
                    }
                }
            }
        } else {
            const size_t index = model->mh.index();
            if (QSSG_GUARD(meshes.size() > index))
                meshes[model->mh.index()] = nullptr;
        }
    }

    // Now is the time to kick off the vertex/index buffer updates for all the
    // new meshes (and their submeshes). This here is the last possible place
    // to kick this off because the rest of the rendering pipeline will only
    // see the individual sub-objects as "renderable objects".
    bufferManager->commitBufferResourceUpdates();
}

void QSSGRenderModelData::prepareMaterials(const QSSGModelsView &models)
{
    for (auto *model : models) {
        const size_t index = model->mh.index();
        if (QSSG_GUARD(materials.size() > index))
            materials[index] = model->materials;
    }
}

void QSSGRenderModelData::updateModelData(QSSGModelsView &models, QSSGRenderer *renderer, const QSSGRenderCameraDataList &renderCameraData)
{
    const auto modelCount = size_t(models.size());
    const bool versionChanged = m_version != m_gnd->version();
    const bool storageSizeChanged = (normalMatrices.size() < modelCount);

    const QMatrix3x3 defaultNormalMatrix;
    const QMatrix4x4 defaultModelViewProjection;

    // resize the storage if needed
    modelViewProjections.resize(modelCount, { defaultModelViewProjection, defaultModelViewProjection });
    normalMatrices.resize(modelCount, defaultNormalMatrix);
    meshes.resize(modelCount, nullptr);
    materials.resize(modelCount, {});

    // If the version or storage size changed we need to re-index the models.
    // NOTE: Node data's version is incremented when the node graph changes and starts at 1.
    if (versionChanged || storageSizeChanged) {
        m_version = m_gnd->version();

        for (quint32 i = 0; i < modelCount; ++i) {
            QSSGRenderModel *model = models[i];
            model->mh = QSSGRenderModelHandle(model->h.context(), model->h.version(), i);
        }
    }

    // - Normal matrices
    const auto doNormalMatrices = [&]() {
        for (const QSSGRenderModel *model : std::as_const(models)) {
            auto &normalMatrix = normalMatrices[model->mh.index()];
            const QMatrix4x4 globalTransform = m_gnd->getGlobalTransform(*model);
            QSSGRenderNode::calculateNormalMatrix(globalTransform, normalMatrix);
        }
    };

    // - MVPs
    const auto doMVPs = [&]() {
        for (const QSSGRenderModel *model : std::as_const(models)) {
            int mvpCount = 0;
            const QMatrix4x4 globalTransform = m_gnd->getGlobalTransform(*model);
            auto &mvp = modelViewProjections[model->mh.index()];
            for (const QSSGRenderCameraData &cameraData : renderCameraData)
                QSSGRenderNode::calculateMVP(globalTransform, cameraData.viewProjection, mvp[mvpCount++]);
        }
    };

#if QT_CONFIG(thread)
    const auto &threadPool = m_gnd->threadPool();
#define qssgTryThreadedStart(func) \
    if (!threadPool->tryStart(func)) { \
        qWarning("Unable to start thread for %s!", #func); \
        func(); \
    }
#define qssgTryWaitForDone() \
    threadPool->waitForDone();
#else
#define qssgTryThreadedStart(func) \
    func();
#define qssgTryWaitForDone() \
    /* no-op */
#endif // QT_CONFIG(thread)

    qssgTryThreadedStart(doNormalMatrices);
    qssgTryThreadedStart(doMVPs);

    // While we are waiting for the threads to finish, we can also prepare and update
    // the materials and meshes.
    prepareMaterials(models);
    prepareMeshData(models, renderer);

    // Wait for the threads to finish
    qssgTryWaitForDone();
}

bool QSSGRenderDataHelpers::updateGlobalNodeDataIndexed(QSSGRenderNode *node, const quint32 version, QSSGGlobalRenderNodeData::GlobalTransformStore &globalTransforms, QSSGGlobalRenderNodeData::GlobalOpacityStore &globalOpacities)
{
    return calcGlobalNodeDataIndexedImpl<Strategy::Update>(node, version, globalTransforms, globalOpacities);
}

bool QSSGRenderDataHelpers::calcGlobalVariablesIndexed(QSSGRenderNode *node, const quint32 version, QSSGGlobalRenderNodeData::GlobalTransformStore &globalTransforms, QSSGGlobalRenderNodeData::GlobalOpacityStore &globalOpacities)
{
    return calcGlobalNodeDataIndexedImpl<Strategy::Initial>(node, version, globalTransforms, globalOpacities);
}

QT_END_NAMESPACE
