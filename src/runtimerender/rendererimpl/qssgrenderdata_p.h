// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGRENDERDATA_P_H
#define QSSGRENDERDATA_P_H

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

#include <QtGui/qmatrix4x4.h>

#include "qssgrenderableobjects_p.h"

#include <vector>
#include <memory>

QT_BEGIN_NAMESPACE

struct QSSGRenderNode;
class QSSGRenderRoot;

class QThreadPool;

// Per window node data
class QSSGGlobalRenderNodeData
{
    Q_DISABLE_COPY_MOVE(QSSGGlobalRenderNodeData)
public:
    struct InstanceTransforms
    {
        QMatrix4x4 local;
        QMatrix4x4 global;
    };

    struct LayerNodeSection
    {
        size_t offset = 0;
        size_t size = 0;
    };

    using LayerNodeView = QSSGDataView<QSSGRenderNode *>;

    using GlobalTransformStore = std::vector<QMatrix4x4>;
    using GlobalOpacityStore = std::vector<float>;
    using InstanceTransformStore = std::vector<InstanceTransforms>;
    using NodeStore = std::vector<QSSGRenderNode *>;
    using LayerNodeViewStore = std::vector<LayerNodeSection>;

    QSSGGlobalRenderNodeData();
    ~QSSGGlobalRenderNodeData();

    void reindex(QSSGRenderRoot *rootNode);

    [[nodiscard]] quint32 version() const { return m_version; }

    // NOTE: The node count is the number of nodes in the "world" tree.
    //       This is not the the same as the storage size. as some nodes
    //       may not be stored in the data or use the same storage location!
    [[nodiscard]] size_t nodeCount() const { return m_nodeCount; }
    [[nodiscard]] size_t storageSize() const { return m_size; }

    [[nodiscard]] QMatrix4x4 getGlobalTransform(QSSGRenderNodeHandle h, QMatrix4x4 defaultValue) const;
    [[nodiscard]] QMatrix4x4 getGlobalTransform(QSSGRenderNodeHandle h) const;
    [[nodiscard]] QMatrix4x4 getGlobalTransform(const QSSGRenderNode &node) const;
    [[nodiscard]] float getGlobalOpacity(QSSGRenderNodeHandle h, float defaultValue = 1.0f) const;
    [[nodiscard]] float getGlobalOpacity(const QSSGRenderNode &node) const;
    [[nodiscard]] InstanceTransforms getInstanceTransforms(QSSGRenderNodeHandle h) const;
    [[nodiscard]] InstanceTransforms getInstanceTransforms(const QSSGRenderNode &node) const;

    [[nodiscard]] LayerNodeView getLayerNodeView(QSSGRenderLayerHandle h) const;
    [[nodiscard]] LayerNodeView getLayerNodeView(const QSSGRenderLayer &layer) const;

    GlobalTransformStore globalTransforms { };
    GlobalOpacityStore globalOpacities { };
    InstanceTransformStore instanceTransforms { };
    NodeStore nodes { };
    LayerNodeViewStore layerNodes { };

#if QT_CONFIG(thread)
    // NOTE: Thread pool for parallel processing of render data.
    //       This thread pool is not intended for async calls.
    //       For long running tasks use the global thread pool instead.
    //       (Threads executed here are expected to be done before the sync has ended).
    const std::unique_ptr<QThreadPool> &threadPool() const;
#endif // QT_CONFIG(thread)

private:
    void collectNodes(QSSGRenderRoot *rootNode);
    void updateGlobalState();

#if QT_CONFIG(thread)
    std::unique_ptr<QThreadPool> m_threadPool;
#endif // QT_CONFIG(thread)

    size_t m_size = 0;
    size_t m_nodeCount = 0;
    quint32 m_version = 0;
};

using QSSGGlobalRenderNodeDataPtr = std::shared_ptr<QSSGGlobalRenderNodeData>;

class QSSGRenderDataHelpers
{
    QSSGRenderDataHelpers() = delete;
public:
    enum class Strategy
    {
        Initial, // Initial calculation of ALL global variables
        Update, // Update calculation of ONLY changed global variables
    };

    enum GlobalStateResult : quint8
    {
        None,
        ActiveChanged = 0x1,
        PickableChanged = 0x2,
    };

    using GlobalStateResultT = std::underlying_type_t<GlobalStateResult>;

    /*!
        \brief calcGlobalNodeData
        \param node The node to calculate the global data for.
        \param version The version of the node current node tree.
        \param globalTransforms The global transforms store.
        \param globalOpacities The global opacities store.
        \return true if the data was updated, false otherwise.

        The function is used to calculate the global node data for the given node.
        It will use the given strategy to determine if it should calculate
        the initial values or update the existing values based on the node's dirty state.

        NOTE: This function assumes the output data is already allocated and can be directly
              indexed!!!
    */
    template <Strategy strategy>
    static bool calcGlobalNodeData(QSSGRenderNode *node,
                                   const quint32 version,
                                   QSSGGlobalRenderNodeData::GlobalTransformStore &globalTransforms,
                                   QSSGGlobalRenderNodeData::GlobalOpacityStore &globalOpacities)
    {
        if constexpr (strategy == Strategy::Initial)
            return calcGlobalVariablesIndexed(node, version, globalTransforms, globalOpacities);
        else
            return updateGlobalNodeDataIndexed(node, version, globalTransforms, globalOpacities);
    }

    /*!
        \brief calcInstanceTransforms
        \param node The node to calculate the instance transforms for.
        \param version The version of the node current node tree.
        \param globalTransforms The global transforms store.
        \param instanceTransforms The instance transforms store.
        \return true if the data was updated, false otherwise.

        The function is used to calculate the instance transforms for the given node.

        NOTE: This function assumes the output data is already allocated and can be directly
              indexed!!!
    */
    static bool calcInstanceTransforms(QSSGRenderNode *node,
                                       const quint32 version,
                                       QSSGGlobalRenderNodeData::GlobalTransformStore &globalTransforms,
                                       QSSGGlobalRenderNodeData::InstanceTransformStore &instanceTransforms);


    /*!
        \brief updateGlobalNodeState
        \param node The node to update the global state for.
        \param version The version of the node current node tree.
        \return The result of the update.

        The function is used to update the global state flagsfor the given node,
        meaning the global active and pickable state of the node.
    */
    static GlobalStateResult updateGlobalNodeState(QSSGRenderNode *node, const quint32 version);
private:
    static bool updateGlobalNodeDataIndexed(QSSGRenderNode *node,
                                            const quint32 version,
                                            QSSGGlobalRenderNodeData::GlobalTransformStore &globalTransforms,
                                            QSSGGlobalRenderNodeData::GlobalOpacityStore &globalOpacities);
    static bool calcGlobalVariablesIndexed(QSSGRenderNode *node,
                                           const quint32 version,
                                           QSSGGlobalRenderNodeData::GlobalTransformStore &globalTransforms,
                                           QSSGGlobalRenderNodeData::GlobalOpacityStore &globalOpacities);
};

// Per layer model data
class QSSGRenderModelData
{
    Q_DISABLE_COPY_MOVE(QSSGRenderModelData)
public:
    explicit QSSGRenderModelData(const QSSGGlobalRenderNodeDataPtr &globalNodeData);

    using ModelViewProjections = std::array<QMatrix4x4, 2>;
    using MaterialList = QVector<QSSGRenderGraphObject *>;

    using ModelViewProjectionStore = std::vector<ModelViewProjections>;
    using NormalMatrixStore = std::vector<QMatrix3x3>;
    using MeshStore = std::vector<QSSGRenderMesh *>;
    using MaterialStore = std::vector<MaterialList>;

    [[nodiscard]] ModelViewProjections getModelViewProjection(QSSGRenderModelHandle h) const;
    [[nodiscard]] ModelViewProjections getModelViewProjection(const QSSGRenderModel &model) const;

    [[nodiscard]] QMatrix3x3 getNormalMatrix(QSSGRenderModelHandle h, QMatrix3x3 defaultValue) const;
    [[nodiscard]] QMatrix3x3 getNormalMatrix(const QSSGRenderModel &model) const;

    [[nodiscard]] QSSGRenderMesh *getMesh(QSSGRenderModelHandle h) const;
    [[nodiscard]] QSSGRenderMesh *getMesh(const QSSGRenderModel &model) const;

    [[nodiscard]] MaterialList getMaterials(QSSGRenderModelHandle h) const;
    [[nodiscard]] MaterialList getMaterials(const QSSGRenderModel &model) const;

    [[nodiscard]] const QSSGGlobalRenderNodeDataPtr &globalNodeData() const { return m_gnd; }

    void updateModelData(QSSGModelsView &models, QSSGRenderer *renderer, const QSSGRenderCameraDataList &renderCameraData);

private:
    ModelViewProjectionStore modelViewProjections;
    NormalMatrixStore normalMatrices;
    MeshStore meshes;
    MaterialStore materials;

    void prepareMeshData(const QSSGModelsView &models, QSSGRenderer *renderer);
    void prepareMaterials(const QSSGModelsView &models);

    QSSGGlobalRenderNodeDataPtr m_gnd;

    quint32 m_version = 0;
};

QT_END_NAMESPACE

#endif // QSSGRENDERDATA_P_H
