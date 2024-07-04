// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSG_RENDER_IMPL_RENDERABLE_OBJECTS_H
#define QSSG_RENDER_IMPL_RENDERABLE_OBJECTS_H

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

#include <QtQuick3DRuntimeRender/private/qssgrendermodel_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderparticles_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermesh_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderkeys_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderableimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderreflectionprobe_p.h>

#include <QtQuick3DUtils/private/qssginvasivelinkedlist_p.h>

QT_BEGIN_NAMESPACE

enum class QSSGRenderableObjectFlag : quint32
{
    HasTransparency = 1 << 0,
    CompletelyTransparent = 1 << 1,
    Dirty = 1 << 2,
    CastsShadows = 1 << 3,
    ReceivesShadows = 1 << 4,
    HasAttributePosition = 1 << 5,
    HasAttributeNormal = 1 << 6,
    HasAttributeTexCoord0 = 1 << 7,
    HasAttributeTexCoord1 = 1 << 8,
    HasAttributeTangent = 1 << 9,
    HasAttributeBinormal = 1 << 10,
    HasAttributeColor = 1 << 11,
    HasAttributeJointAndWeight = 1 << 12,
    IsPointsTopology = 1 << 13,
    // The number of target models' attributes are too many
    // to store in a renderable flag.
    // They will be recorded in shaderKey.
    HasAttributeMorphTarget = 1 << 14,
    RequiresScreenTexture = 1 << 15,
    ReceivesReflections = 1 << 16,
    UsedInBakedLighting = 1 << 17,
    RendersWithLightmap = 1 << 18,
    HasAttributeTexCoordLightmap = 1 << 19,
    CastsReflections = 1 << 20
};

struct QSSGRenderableObjectFlags : public QFlags<QSSGRenderableObjectFlag>
{
    void setHasTransparency(bool inHasTransparency)
    {
        setFlag(QSSGRenderableObjectFlag::HasTransparency, inHasTransparency);
    }
    bool hasTransparency() const { return this->operator&(QSSGRenderableObjectFlag::HasTransparency); }
    void setCompletelyTransparent(bool inTransparent)
    {
        setFlag(QSSGRenderableObjectFlag::CompletelyTransparent, inTransparent);
    }
    bool isCompletelyTransparent() const
    {
        return this->operator&(QSSGRenderableObjectFlag::CompletelyTransparent);
    }
    void setDirty(bool inDirty) { setFlag(QSSGRenderableObjectFlag::Dirty, inDirty); }
    bool isDirty() const { return this->operator&(QSSGRenderableObjectFlag::Dirty); }

    void setCastsShadows(bool inCastsShadows) { setFlag(QSSGRenderableObjectFlag::CastsShadows, inCastsShadows); }
    bool castsShadows() const { return this->operator&(QSSGRenderableObjectFlag::CastsShadows); }

    void setReceivesShadows(bool inReceivesShadows) { setFlag(QSSGRenderableObjectFlag::ReceivesShadows, inReceivesShadows); }
    bool receivesShadows() const { return this->operator&(QSSGRenderableObjectFlag::ReceivesShadows); }

    void setReceivesReflections(bool inReceivesReflections) { setFlag(QSSGRenderableObjectFlag::ReceivesReflections, inReceivesReflections); }
    bool receivesReflections() const { return this->operator&(QSSGRenderableObjectFlag::ReceivesReflections); }

    void setCastsReflections(bool inCastsReflections) { setFlag(QSSGRenderableObjectFlag::CastsReflections, inCastsReflections); }
    bool castsReflections() const { return this->operator&(QSSGRenderableObjectFlag::CastsReflections); }

    void setUsedInBakedLighting(bool inUsedInBakedLighting) { setFlag(QSSGRenderableObjectFlag::UsedInBakedLighting, inUsedInBakedLighting); }
    bool usedInBakedLighting() const { return this->operator&(QSSGRenderableObjectFlag::UsedInBakedLighting); }

    void setRendersWithLightmap(bool inRendersWithLightmap) { setFlag(QSSGRenderableObjectFlag::RendersWithLightmap, inRendersWithLightmap); }
    bool rendersWithLightmap() const { return this->operator&(QSSGRenderableObjectFlag::RendersWithLightmap); }

    void setHasAttributePosition(bool b) { setFlag(QSSGRenderableObjectFlag::HasAttributePosition, b); }
    bool hasAttributePosition() const { return this->operator&(QSSGRenderableObjectFlag::HasAttributePosition); }

    void setHasAttributeNormal(bool b) { setFlag(QSSGRenderableObjectFlag::HasAttributeNormal, b); }
    bool hasAttributeNormal() const { return this->operator&(QSSGRenderableObjectFlag::HasAttributeNormal); }

    void setHasAttributeTexCoord0(bool b) { setFlag(QSSGRenderableObjectFlag::HasAttributeTexCoord0, b); }
    bool hasAttributeTexCoord0() const { return this->operator&(QSSGRenderableObjectFlag::HasAttributeTexCoord0); }

    void setHasAttributeTexCoord1(bool b) { setFlag(QSSGRenderableObjectFlag::HasAttributeTexCoord1, b); }
    bool hasAttributeTexCoord1() const { return this->operator&(QSSGRenderableObjectFlag::HasAttributeTexCoord1); }

    void setHasAttributeTexCoordLightmap(bool b) { setFlag(QSSGRenderableObjectFlag::HasAttributeTexCoordLightmap, b); }
    bool hasAttributeTexCoordLightmap() const { return this->operator&(QSSGRenderableObjectFlag::HasAttributeTexCoordLightmap); }

    void setHasAttributeTangent(bool b) { setFlag(QSSGRenderableObjectFlag::HasAttributeTangent, b); }
    bool hasAttributeTangent() const { return this->operator&(QSSGRenderableObjectFlag::HasAttributeTangent); }

    void setHasAttributeBinormal(bool b) { setFlag(QSSGRenderableObjectFlag::HasAttributeBinormal, b); }
    bool hasAttributeBinormal() const { return this->operator&(QSSGRenderableObjectFlag::HasAttributeBinormal); }

    void setHasAttributeColor(bool b) { setFlag(QSSGRenderableObjectFlag::HasAttributeColor, b); }
    bool hasAttributeColor() const { return this->operator&(QSSGRenderableObjectFlag::HasAttributeColor); }

    void setHasAttributeJointAndWeight(bool b) { setFlag(QSSGRenderableObjectFlag::HasAttributeJointAndWeight, b);
    }
    bool hasAttributeJointAndWeight() const { return this->operator&(QSSGRenderableObjectFlag::HasAttributeJointAndWeight); }

    void setHasAttributeMorphTarget(bool b) { setFlag(QSSGRenderableObjectFlag::HasAttributeMorphTarget, b);
    }
    bool hasAttributeMorphTarget() const { return this->operator&(QSSGRenderableObjectFlag::HasAttributeMorphTarget); }

    void setPointsTopology(bool v)
    {
        setFlag(QSSGRenderableObjectFlag::IsPointsTopology, v);
    }
    bool isPointsTopology() const
    {
        return this->operator&(QSSGRenderableObjectFlag::IsPointsTopology);
    }
    void setRequiresScreenTexture(bool v)
    {
        setFlag(QSSGRenderableObjectFlag::RequiresScreenTexture, v);
    }
    bool requiresScreenTexture() const {
        return this->operator&(QSSGRenderableObjectFlag::RequiresScreenTexture);
    }
};

struct QSSGShaderLight
{
    QSSGRenderLight *light = nullptr;
    bool shadows = false;
    QVector3D direction;

    inline bool operator < (const QSSGShaderLight &o) const
    {
        // sort by light type
        if (light->type < o.light->type)
            return true;
        // then shadow lights first
        if (shadows > o.shadows)
            return true;
        return false;
    }
};

struct QSSGShaderReflectionProbe
{
    QVector3D probeCubeMapCenter;
    QVector3D probeBoxMax;
    QVector3D probeBoxMin;
    bool enabled = false;
    int parallaxCorrection = 0;
};

// Having this as a QVLA is beneficial mainly because QVector would need to
// detach somewhere in QSSGLayerRenderPreparationData::prepareForRender so the
// implicit sharing's benefits do not outweigh the cost of allocations in this case.
typedef QVarLengthArray<QSSGShaderLight, 16> QSSGShaderLightList;
using QSSGShaderLightListView = QSSGDataView<QSSGShaderLight>;

struct QSSGRenderableObject;

struct QSSGRenderableNodeEntry
{
    enum Overridden : quint16
    {
        Original = 0,
        Disabled = 0x1,
        GlobalTransform = 0x2,
        Materials = 0x4,
        GlobalOpacity = 0x5
    };

    QSSGRenderNode *node = nullptr;
    // TODO: We should have an index here for look-up and store the data in a table,
    // er already have the index from when we collect the nodes. We might cull some items at a later
    // stage but that should be fine. The sort data can be just a float and the index to this...
    mutable QMatrix4x4 globalTransform;
    mutable QSSGRenderMesh *mesh = nullptr;
    mutable QVector<QSSGRenderGraphObject *> materials;
    mutable QSSGShaderLightListView lights;
    mutable float globalOpacity { 1.0f };
    mutable quint16 overridden { Original };

    bool isNull() const { return (node == nullptr); }
    QSSGRenderableNodeEntry() = default;
    QSSGRenderableNodeEntry(QSSGRenderNode &inNode) : node(&inNode) {}
};

// Used for sorting
struct QSSGRenderableObjectHandle
{
    QSSGRenderableObjectHandle() = default;
    QSSGRenderableObjectHandle(QSSGRenderableObject *o, float camDistSq)
        : obj(o)
        , cameraDistanceSq(camDistSq)
    {}
    QSSGRenderableObject *obj = nullptr;
    float cameraDistanceSq = 0.0f;
};
Q_DECLARE_TYPEINFO(QSSGRenderableObjectHandle, Q_PRIMITIVE_TYPE);

using QSSGRenderableObjectList = QVector<QSSGRenderableObjectHandle>;

struct QSSGRenderableObject
{
    enum class Type : quint8
    {
        DefaultMaterialMeshSubset,
        CustomMaterialMeshSubset,
        Particles
    };

    // Variables used for picking
    const QMatrix4x4 &globalTransform;
    const QSSGBounds3 &bounds;
    QSSGBounds3 globalBounds;

    QSSGRenderableObjectFlags renderableFlags;
    // For rough sorting for transparency and for depth
    QVector3D worldCenterPoint;
    float depthBiasSq; // Squared as our sorting is based on the square distance!
    float camdistSq = 0.0f;
    QSSGDepthDrawMode depthWriteMode = QSSGDepthDrawMode::OpaqueOnly;
    const Type type;
    float instancingLodMin = -1;
    float instancingLodMax = -1;

    QSSGRenderableObject(Type ty,
                         QSSGRenderableObjectFlags inFlags,
                         const QVector3D &inWorldCenterPt,
                         const QMatrix4x4 &inGlobalTransform,
                         const QSSGBounds3 &inBounds,
                         float inDepthBias,
                         float inMinThreshold = -1,
                         float inMaxThreshold = -1)

        : globalTransform(inGlobalTransform)
        , bounds(inBounds)
        , globalBounds(inBounds)
        , renderableFlags(inFlags)
        , worldCenterPoint(inWorldCenterPt)
        , depthBiasSq(inDepthBias)
        , type(ty)
        , instancingLodMin(inMinThreshold)
        , instancingLodMax(inMaxThreshold)
    {
    }
};

Q_STATIC_ASSERT(std::is_trivially_destructible<QSSGRenderableObject>::value);

struct QSSGSubsetRenderable;

// Different subsets from the same model will get the same
// model context so we can generate the MVP and normal matrix once
// and only once per subset.
struct QSSGModelContext
{
    const QSSGRenderModel &model;
    QMatrix4x4 modelViewProjection;
    QMatrix3x3 normalMatrix;

    QSSGModelContext(const QSSGRenderModel &inModel, const QMatrix4x4 &globalTransform, const QMatrix4x4 &inViewProjection) : model(inModel)
    {
        // For skinning, node's global transformation will be ignored and
        // an identity matrix will be used for the normalMatrix
        if (model.usesBoneTexture()) {
            modelViewProjection = inViewProjection;
            normalMatrix = QMatrix3x3();
        } else {
            QSSGRenderNode::calculateMVPAndNormalMatrix(globalTransform, inViewProjection, modelViewProjection, normalMatrix);
        }
    }

    QSSGDataRef<QSSGSubsetRenderable> subsets;
};

Q_STATIC_ASSERT(std::is_trivially_destructible<QSSGModelContext>::value);

class QSSGRenderer;
class QSSGLayerRenderData;
struct QSSGShadowMapEntry;

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGSubsetRenderable : public QSSGRenderableObject
{
    int reflectionProbeIndex = -1;
    float distanceFromReflectionProbe;
    quint32 subsetLevelOfDetail = 0;
    QSSGShaderReflectionProbe reflectionProbe;
    QSSGRenderer *renderer = nullptr;
    const QSSGModelContext &modelContext;
    const QSSGRenderSubset &subset;
    QRhiBuffer *instanceBuffer = nullptr;
    float opacity;
    const QSSGRenderGraphObject &material;
    QSSGRenderableImage *firstImage;
    QSSGShaderDefaultMaterialKey shaderDescription;
    const QSSGShaderLightListView &lights;

    struct {
        // Transient (due to the subsetRenderable being allocated using a
        // per-frame allocator on every frame), not owned refs from the
        // rhi-prepare step, used by the rhi-render step.
        struct {
            QRhiGraphicsPipeline *pipeline = nullptr;
            QRhiShaderResourceBindings *srb = nullptr;
        } mainPass;
        struct {
            QRhiGraphicsPipeline *pipeline = nullptr;
            QRhiShaderResourceBindings *srb = nullptr;
        } depthPrePass;
        struct {
            QRhiGraphicsPipeline *pipeline = nullptr;
            QRhiShaderResourceBindings *srb[6] = {};
        } shadowPass;
        struct {
            QRhiGraphicsPipeline *pipeline = nullptr;
            QRhiShaderResourceBindings *srb[6] = {};
        } reflectionPass;
    } rhiRenderData;

    QSSGSubsetRenderable(Type type,
                         QSSGRenderableObjectFlags inFlags,
                         const QVector3D &inWorldCenterPt,
                         QSSGRenderer *rendr,
                         const QSSGRenderSubset &inSubset,
                         const QSSGModelContext &inModelContext,
                         float inOpacity,
                         quint32 inSubsetLevelOfDetail,
                         const QSSGRenderGraphObject &mat,
                         QSSGRenderableImage *inFirstImage,
                         QSSGShaderDefaultMaterialKey inShaderKey,
                         const QSSGShaderLightListView &inLights);

    [[nodiscard]] const QSSGRenderGraphObject &getMaterial() const { return material; }
};

Q_STATIC_ASSERT(std::is_trivially_destructible<QSSGSubsetRenderable>::value);

/**
 * A renderable that corresponds to a particles.
 */
struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGParticlesRenderable : public QSSGRenderableObject
{
    QSSGRenderer *renderer = nullptr;
    const QSSGRenderParticles &particles;
    QSSGRenderableImage *firstImage;
    QSSGRenderableImage *colorTable;
    const QSSGShaderLightListView &lights;
    float opacity;

    struct {
        // Transient (due to the subsetRenderable being allocated using a
        // per-frame allocator on every frame), not owned refs from the
        // rhi-prepare step, used by the rhi-render step.
        struct {
            QRhiGraphicsPipeline *pipeline = nullptr;
            QRhiShaderResourceBindings *srb = nullptr;
        } mainPass;
        struct {
            QRhiGraphicsPipeline *pipeline = nullptr;
            QRhiShaderResourceBindings *srb = nullptr;
        } depthPrePass;
        struct {
            QRhiGraphicsPipeline *pipeline = nullptr;
            QRhiShaderResourceBindings *srb[6] = {};
        } shadowPass;
        struct {
            QRhiGraphicsPipeline *pipeline = nullptr;
            QRhiShaderResourceBindings *srb[6] = {};
        } reflectionPass;
    } rhiRenderData;

    QSSGParticlesRenderable(QSSGRenderableObjectFlags inFlags,
                            const QVector3D &inWorldCenterPt,
                            QSSGRenderer *rendr,
                            const QSSGRenderParticles &inParticles,
                            QSSGRenderableImage *inFirstImage,
                            QSSGRenderableImage *inColorTable,
                            const QSSGShaderLightListView &inLights,
                            float inOpacity);
};

Q_STATIC_ASSERT(std::is_trivially_destructible<QSSGParticlesRenderable>::value);

QT_END_NAMESPACE

#endif
