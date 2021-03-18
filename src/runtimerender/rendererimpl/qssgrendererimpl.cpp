/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererimpl_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendereffectsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderresourcemanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterialsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegeneratorv2_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgperframeallocator_p.h>

#include <QtQuick3DRender/private/qssgrenderframebuffer_p.h>
#include <QtQuick3DUtils/private/qssgdataref_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <cstdlib>
#include <algorithm>

#ifdef Q_CC_MSVC
#pragma warning(disable : 4355)
#endif

// Quick tests you can run to find performance problems

//#define QSSG_RENDER_DISABLE_HARDWARE_BLENDING 1
//#define QSSG_RENDER_DISABLE_LIGHTING 1
//#define QSSG_RENDER_DISABLE_TEXTURING 1
//#define QSSG_RENDER_DISABLE_TRANSPARENCY 1
//#define QSSG_RENDER_DISABLE_FRUSTUM_CULLING 1

// If you are fillrate bound then sorting opaque objects can help in some circumstances
//#define QSSG_RENDER_DISABLE_OPAQUE_SORT 1

QT_BEGIN_NAMESPACE

struct QSSGRenderableImage;
struct QSSGShaderGeneratorGeneratedShader;
struct QSSGSubsetRenderable;

void QSSGRendererImpl::releaseResources()
{
    m_shaders.clear();
    m_instanceRenderMap.clear();
    m_constantBuffers.clear();
}

QSSGRendererImpl::QSSGRendererImpl(QSSGRenderContextInterface *ctx)
    : m_contextInterface(ctx)
    , m_context(ctx->renderContext())
    , m_bufferManager(ctx->bufferManager())
    , m_currentLayer(nullptr)
    , m_pickRenderPlugins(true)
    , m_layerGPuProfilingEnabled(false)
    , m_progressiveAARenderRequest(false)
{
}

QSSGRendererImpl::~QSSGRendererImpl()
{
    m_contextInterface = nullptr;
    Q_ASSERT(m_shaders.size() == 0);
    Q_ASSERT(m_instanceRenderMap.size() == 0);
    Q_ASSERT(m_constantBuffers.size() == 0);
    releaseResources();
}

void QSSGRendererImpl::childrenUpdated(QSSGRenderNode &inParent)
{   
    if (inParent.type == QSSGRenderGraphObject::Type::Layer) {
        const QSSGRenderLayer *theLayer = layerForNode(inParent);
        auto theIter = m_instanceRenderMap.find(theLayer);
        if (theIter != m_instanceRenderMap.end()) {
            theIter.value()->cameras.clear();
            theIter.value()->lights.clear();
            theIter.value()->renderableNodes.clear();
        }
    } else if (inParent.parent) {
        childrenUpdated(*inParent.parent);
    }
}

static inline QSSGRenderLayer *getNextLayer(QSSGRenderLayer &inLayer)
{
    if (inLayer.nextSibling && inLayer.nextSibling->type == QSSGRenderGraphObject::Type::Layer)
        return static_cast<QSSGRenderLayer *>(inLayer.nextSibling);
    return nullptr;
}

// Found by fair roll of the dice (in practice we'll never have more then 1 layer!).
using QSSGRenderLayerList = QVarLengthArray<QSSGRenderLayer *, 4>;

static inline void maybePushLayer(QSSGRenderLayer &inLayer, QSSGRenderLayerList &outLayerList)
{
    inLayer.calculateGlobalVariables();
    if (inLayer.flags.testFlag(QSSGRenderLayer::Flag::GloballyActive) && inLayer.flags.testFlag(QSSGRenderLayer::Flag::LayerRenderToTarget))
        outLayerList.push_back(&inLayer);
}

bool QSSGRendererImpl::prepareLayerForRender(QSSGRenderLayer &inLayer,
                                               const QSize &surfaceSize)
{

    QSSGRenderLayerList renderableLayers;
    maybePushLayer(inLayer, renderableLayers);

    bool retval = false;

    auto iter = renderableLayers.crbegin();
    const auto end = renderableLayers.crend();
    for (; iter != end; ++iter) {
        // Store the previous state of if we were rendering a layer.
        QSSGRenderLayer *theLayer = *iter;
        QSSGRef<QSSGLayerRenderData> theRenderData = getOrCreateLayerRenderDataForNode(*theLayer);

        if (Q_LIKELY(theRenderData)) {
            theRenderData->prepareForRender(surfaceSize);
            retval = retval || theRenderData->layerPrepResult->flags.wasDirty();
        } else {
            Q_ASSERT(false);
        }
    }

    return retval;
}

void QSSGRendererImpl::renderLayer(QSSGRenderLayer &inLayer,
                                     const QSize &surfaceSize,
                                     bool clear,
                                     const QColor &clearColor)
{
    Q_UNUSED(surfaceSize)
    Q_UNUSED(clearColor)
    QSSGRenderLayerList renderableLayers;
    maybePushLayer(inLayer, renderableLayers);

    const QSSGRef<QSSGRenderContext> &theRenderContext(m_contextInterface->renderContext());
    // Do not use reference since it will just shadow the hardware context variable in the
    // render context breaking the caching.
    const QSSGRef<QSSGRenderFrameBuffer> theFB = theRenderContext->renderTarget();
    auto iter = renderableLayers.crbegin();
    const auto end = renderableLayers.crend();
    m_progressiveAARenderRequest = false;

    for (iter = renderableLayers.crbegin(); iter != end; ++iter) {
        // Store the previous state of if we were rendering a layer.
        QSSGRenderLayer *theLayer = *iter;
        const QSSGRef<QSSGLayerRenderData> &theRenderData = getOrCreateLayerRenderDataForNode(*theLayer);

        if (Q_LIKELY(theRenderData)) {
            // Make sure that we don't clear the window, when requested not to.
            theRenderData->layerPrepResult->flags.setRequiresTransparentClear(clear);
            if (theRenderData->layerPrepResult->isLayerVisible()) {
                theRenderData->runnableRenderToViewport(theFB);
                m_progressiveAARenderRequest |= theRenderData->progressiveAARenderRequest();
            }
        } else {
            Q_ASSERT(false);
        }
    }
}

QSSGRenderLayer *QSSGRendererImpl::layerForNode(const QSSGRenderNode &inNode) const
{
    if (inNode.type == QSSGRenderGraphObject::Type::Layer)
        return &const_cast<QSSGRenderLayer &>(static_cast<const QSSGRenderLayer &>(inNode));

    if (inNode.parent)
        return layerForNode(*inNode.parent);

    return nullptr;
}

QSSGRef<QSSGLayerRenderData> QSSGRendererImpl::getOrCreateLayerRenderDataForNode(const QSSGRenderNode &inNode)
{
    const QSSGRenderLayer *theLayer = layerForNode(inNode);
    if (theLayer) {
        auto it = m_instanceRenderMap.constFind(theLayer);
        if (it != m_instanceRenderMap.cend())
            return it.value();

        it = m_instanceRenderMap.insert(theLayer, new QSSGLayerRenderData(const_cast<QSSGRenderLayer &>(*theLayer), this));

        // create a profiler if enabled
        if (isLayerGpuProfilingEnabled() && it.value())
            it.value()->createGpuProfiler();

        return *it;
    }
    return nullptr;
}

QSSGRenderCamera *QSSGRendererImpl::cameraForNode(const QSSGRenderNode &inNode) const
{
    const QSSGRef<QSSGLayerRenderData> &theLayer = const_cast<QSSGRendererImpl &>(*this).getOrCreateLayerRenderDataForNode(inNode);
    if (theLayer)
        return theLayer->camera;
    return nullptr;
}

QSSGOption<QSSGCuboidRect> QSSGRendererImpl::cameraBounds(const QSSGRenderGraphObject &inObject)
{
    if (inObject.isNodeType()) {
        const QSSGRenderNode &theNode = static_cast<const QSSGRenderNode &>(inObject);
        const QSSGRef<QSSGLayerRenderData> &theLayer = getOrCreateLayerRenderDataForNode(theNode);

        QSSGRenderCamera *theCamera = theLayer->camera;
        if (theCamera)
            return theCamera->getCameraBounds(theLayer->layerPrepResult->viewport());

    }
    return QSSGOption<QSSGCuboidRect>();
}

void QSSGRendererImpl::drawScreenRect(QRectF inRect, const QVector3D &inColor)
{
    QSSGRenderCamera theScreenCamera;
    theScreenCamera.markDirty(QSSGRenderCamera::TransformDirtyFlag::TransformIsDirty);
    QRectF theViewport(m_context->viewport());
    theScreenCamera.flags.setFlag(QSSGRenderCamera::Flag::Orthographic);
    theScreenCamera.calculateGlobalVariables(theViewport);
    generateXYQuad();
    if (!m_screenRectShader) {
        QSSGRef<QSSGShaderProgramGeneratorInterface> theGenerator(getProgramGenerator());
        theGenerator->beginProgram();
        QSSGShaderStageGeneratorInterface &vertexGenerator(*theGenerator->getStage(QSSGShaderGeneratorStage::Vertex));
        QSSGShaderStageGeneratorInterface &fragmentGenerator(*theGenerator->getStage(QSSGShaderGeneratorStage::Fragment));
        // TODO: Move out and change type!
        vertexGenerator.addIncoming("attr_pos", "vec3");
        vertexGenerator.addUniform("modelViewProjection", "mat4");
        vertexGenerator.addUniform("rectangle_dims", "vec3");
        vertexGenerator.append("void main() {");
        vertexGenerator.append("\tgl_Position = modelViewProjection * vec4(attr_pos * rectangle_dims, 1.0);");
        vertexGenerator.append("}");
        fragmentGenerator.addUniform("output_color", "vec3");
        fragmentGenerator.append("void main() {");
        fragmentGenerator.append("\tgl_FragColor.rgb = output_color;");
        fragmentGenerator.append("\tgl_FragColor.a = 1.0;");
        fragmentGenerator.append("}");
        // No flags enabled
        m_screenRectShader = theGenerator->compileGeneratedShader("DrawScreenRect", QSSGShaderCacheProgramFlags(), ShaderFeatureSetList());
    }
    if (m_screenRectShader) {
        // Fudge the rect by one pixel to ensure we see all the corners.
        if (inRect.width() > 1)
            inRect.setWidth(inRect.width() - 1);
        if (inRect.height() > 1)
            inRect.setHeight(inRect.height() - 1);
        inRect.setX(inRect.x() + 1);
        inRect.setY(inRect.y() + 1);
        // Figure out the rect center.
        QSSGRenderNode theNode;

        const QPointF &center = inRect.center();
        QVector2D rectGlobalCenter = { float(center.x()), float(center.y()) };
        QVector2D rectCenter(toNormalizedRectRelative(theViewport, rectGlobalCenter));
        theNode.position.setX(rectCenter.x());
        theNode.position.setY(rectCenter.y());
        theNode.markDirty(QSSGRenderNode::TransformDirtyFlag::TransformIsDirty);
        theNode.calculateGlobalVariables();
        QMatrix4x4 theViewProjection;
        theScreenCamera.calculateViewProjectionMatrix(theViewProjection);
        QMatrix4x4 theMVP;
        QMatrix3x3 theNormal;
        theNode.calculateMVPAndNormalMatrix(theViewProjection, theMVP, theNormal);
        m_context->setBlendingEnabled(false);
        m_context->setDepthWriteEnabled(false);
        m_context->setDepthTestEnabled(false);
        m_context->setCullingEnabled(false);
        m_context->setActiveShader(m_screenRectShader);
        m_screenRectShader->setPropertyValue("modelViewProjection", theMVP);
        m_screenRectShader->setPropertyValue("output_color", inColor);
        m_screenRectShader->setPropertyValue("rectangle_dims", QVector3D(float(inRect.width()) / 2.0f, float(inRect.height()) / 2.0f, 0.0f));
    }
    if (!m_rectInputAssembler) {
        Q_ASSERT(m_quadVertexBuffer);
        const quint8 indexData[] = { 0, 1, 1, 2, 2, 3, 3, 0 };

        m_rectIndexBuffer = new QSSGRenderIndexBuffer(m_context, QSSGRenderBufferUsageType::Static,
                                                        QSSGRenderComponentType::UnsignedInteger8,
                                                        toDataView(indexData, sizeof(indexData)));

        QSSGRenderVertexBufferEntry theEntries[] = {
            QSSGRenderVertexBufferEntry("attr_pos", QSSGRenderComponentType::Float32, 3),
        };

        // create our attribute layout
        m_rectAttribLayout = m_context->createAttributeLayout(toDataView(theEntries, 1));

        quint32 strides = m_quadVertexBuffer->stride();
        quint32 offsets = 0;
        m_rectInputAssembler = m_context->createInputAssembler(m_rectAttribLayout,
                                                               toDataView(&m_quadVertexBuffer, 1),
                                                               m_rectIndexBuffer,
                                                               toDataView(&strides, 1),
                                                               toDataView(&offsets, 1));
    }

    m_context->setInputAssembler(m_rectInputAssembler);
    m_context->draw(QSSGRenderDrawMode::Lines, m_rectIndexBuffer->numIndices(), 0);
}

void QSSGRendererImpl::addMaterialDirtyClear(QSSGRenderGraphObject *material)
{
    m_materialClearDirty.insert(material);
}

void QSSGRendererImpl::beginFrame()
{
    for (int idx = 0, end = m_lastFrameLayers.size(); idx < end; ++idx)
        m_lastFrameLayers[idx]->resetForFrame();
    m_lastFrameLayers.clear();
    for (auto *matObj : qAsConst(m_materialClearDirty)) {
        if (matObj->type == QSSGRenderGraphObject::Type::CustomMaterial)
            static_cast<QSSGRenderCustomMaterial *>(matObj)->updateDirtyForFrame();
        else if (matObj->type == QSSGRenderGraphObject::Type::DefaultMaterial)
            static_cast<QSSGRenderDefaultMaterial *>(matObj)->dirty.updateDirtyForFrame();
    }
    m_materialClearDirty.clear();
}
void QSSGRendererImpl::endFrame()
{
}

inline bool pickResultLessThan(const QSSGRenderPickResult &lhs, const QSSGRenderPickResult &rhs)
{
    return lhs.m_cameraDistanceSq < rhs.m_cameraDistanceSq;
}

inline float clampUVCoord(float inUVCoord, QSSGRenderTextureCoordOp inCoordOp)
{
    if (inUVCoord > 1.0f || inUVCoord < 0.0f) {
        switch (inCoordOp) {
        default:
            Q_ASSERT(false);
            break;
        case QSSGRenderTextureCoordOp::ClampToEdge:
            inUVCoord = qMin(inUVCoord, 1.0f);
            inUVCoord = qMax(inUVCoord, 0.0f);
            break;
        case QSSGRenderTextureCoordOp::Repeat: {
            float multiplier = inUVCoord > 0.0f ? 1.0f : -1.0f;
            float clamp = std::fabs(inUVCoord);
            clamp = clamp - std::floor(clamp);
            if (multiplier < 0)
                inUVCoord = 1.0f - clamp;
            else
                inUVCoord = clamp;
        } break;
        case QSSGRenderTextureCoordOp::MirroredRepeat: {
            float multiplier = inUVCoord > 0.0f ? 1.0f : -1.0f;
            float clamp = std::fabs(inUVCoord);
            if (multiplier > 0.0f)
                clamp -= 1.0f;
            quint32 isMirrored = (quint32(clamp)) % 2 == 0;
            float remainder = clamp - std::floor(clamp);
            inUVCoord = remainder;
            if (isMirrored) {
                if (multiplier > 0.0f)
                    inUVCoord = 1.0f - inUVCoord;
            } else {
                if (multiplier < 0.0f)
                    inUVCoord = 1.0f - remainder;
            }
        } break;
        }
    }
    return inUVCoord;
}

QSSGPickResultProcessResult QSSGRendererImpl::processPickResultList(bool inPickEverything)
{
    Q_UNUSED(inPickEverything)
    if (m_lastPickResults.empty())
        return QSSGPickResultProcessResult();
    // Things are rendered in a particular order and we need to respect that ordering.
    std::stable_sort(m_lastPickResults.begin(), m_lastPickResults.end(), pickResultLessThan);

    // We need to pick against sub objects basically somewhat recursively
    // but if we don't hit any sub objects and the parent isn't pickable then
    // we need to move onto the next item in the list.
    // We need to keep in mind that theQuery->Pick will enter this method in a later
    // stack frame so *if* we get to sub objects we need to pick against them but if the pick
    // completely misses *and* the parent object locally pickable is false then we need to move
    // onto the next object.

    const int numToCopy = m_lastPickResults.size();
    Q_ASSERT(numToCopy >= 0);
    size_t numCopyBytes = size_t(numToCopy) * sizeof(QSSGRenderPickResult);
    QSSGRenderPickResult *thePickResults = reinterpret_cast<QSSGRenderPickResult *>(
            m_contextInterface->perFrameAllocator().allocate(numCopyBytes));
    ::memcpy(thePickResults, m_lastPickResults.data(), numCopyBytes);
    m_lastPickResults.clear();
    QSSGPickResultProcessResult thePickResult(thePickResults[0]);
    return thePickResult;
}

QSSGRenderPickResult QSSGRendererImpl::pick(QSSGRenderLayer &inLayer,
                                                const QVector2D &inViewportDimensions,
                                                const QVector2D &inMouseCoords,
                                                bool inPickSiblings,
                                                bool inPickEverything)
{
    m_lastPickResults.clear();

    QSSGRenderLayer *theLayer = &inLayer;
    // Stepping through how the original runtime did picking it picked layers in order
    // stopping at the first hit.  So objects on the top layer had first crack at the pick
    // vector itself.
    do {
        if (theLayer->flags.testFlag(QSSGRenderLayer::Flag::Active)) {
            const auto theIter = m_instanceRenderMap.constFind(theLayer);
            if (theIter != m_instanceRenderMap.cend()) {
                m_lastPickResults.clear();
                getLayerHitObjectList(*theIter.value(), inViewportDimensions, inMouseCoords, inPickEverything, m_lastPickResults);
                QSSGPickResultProcessResult retval(processPickResultList(inPickEverything));
                if (retval.m_wasPickConsumed)
                    return retval;
            } else {
                // Q_ASSERT( false );
            }
        }

        if (inPickSiblings)
            theLayer = getNextLayer(*theLayer);
        else
            theLayer = nullptr;
    } while (theLayer != nullptr);

    return QSSGRenderPickResult();
}

QSSGRenderPickResult QSSGRendererImpl::syncPick(const QSSGRenderLayer &layer,
                                                const QSSGRef<QSSGBufferManager> &bufferManager,
                                                const QVector2D &inViewportDimensions,
                                                const QVector2D &inMouseCoords)
{
    using PickResultList = QVarLengthArray<QSSGRenderPickResult, 20>; // Lets assume most items are filtered out already
    static const auto processResults = [](PickResultList &pickResults) {
        if (pickResults.empty())
            return QSSGPickResultProcessResult();
        // Things are rendered in a particular order and we need to respect that ordering.
        std::stable_sort(pickResults.begin(), pickResults.end(), [](const QSSGRenderPickResult &lhs, const QSSGRenderPickResult &rhs) {
            return lhs.m_cameraDistanceSq < rhs.m_cameraDistanceSq;
        });
        return QSSGPickResultProcessResult{ pickResults.at(0), true };
    };

    PickResultList pickResults;
    if (layer.flags.testFlag(QSSGRenderLayer::Flag::Active)) {
        getLayerHitObjectList(layer, bufferManager, inViewportDimensions, inMouseCoords, false, pickResults);
        QSSGPickResultProcessResult retval = processResults(pickResults);
        if (retval.m_wasPickConsumed)
            return retval;
    }

    return QSSGPickResultProcessResult();
}

QSSGOption<QVector2D> QSSGRendererImpl::facePosition(QSSGRenderNode &inNode,
                                                         QSSGBounds3 inBounds,
                                                         const QMatrix4x4 &inGlobalTransform,
                                                         const QVector2D &inViewportDimensions,
                                                         const QVector2D &inMouseCoords,
                                                         QSSGDataView<QSSGRenderGraphObject *> inMapperObjects,
                                                         QSSGRenderBasisPlanes inPlane)
{
    Q_UNUSED(inMapperObjects)
    const QSSGRef<QSSGLayerRenderData> &theLayerData = getOrCreateLayerRenderDataForNode(inNode);
    if (theLayerData == nullptr)
        return QSSGEmpty();
    // This function assumes the layer was rendered to the scene itself.  There is another
    // function
    // for completely offscreen layers that don't get rendered to the scene.
    bool wasRenderToTarget(theLayerData->layer.flags.testFlag(QSSGRenderLayer::Flag::LayerRenderToTarget));
    if (!wasRenderToTarget || theLayerData->camera == nullptr || !theLayerData->layerPrepResult.hasValue())
        return QSSGEmpty();

    QVector2D theMouseCoords(inMouseCoords);
    QVector2D theViewportDimensions(inViewportDimensions);

    const auto camera = theLayerData->layerPrepResult->camera();
    const auto viewport = theLayerData->layerPrepResult->viewport();
    QSSGOption<QSSGRenderRay> theHitRay = QSSGLayerRenderHelper::pickRay(*camera, viewport, theMouseCoords, theViewportDimensions, false);
    if (!theHitRay.hasValue())
        return QSSGEmpty();

    // Scale the mouse coords to change them into the camera's numerical space.
    QSSGRenderRay thePickRay = *theHitRay;
    QSSGOption<QVector2D> newValue = thePickRay.relative(inGlobalTransform, inBounds, inPlane);
    return newValue;
}

QVector3D QSSGRendererImpl::unprojectToPosition(QSSGRenderNode &inNode, QVector3D &inPosition, const QVector2D &inMouseVec) const
{
    // Translate mouse into layer's coordinates
    const QSSGRef<QSSGLayerRenderData> &theData = const_cast<QSSGRendererImpl &>(*this).getOrCreateLayerRenderDataForNode(inNode);
    if (theData == nullptr || theData->camera == nullptr) {
        return QVector3D(0, 0, 0);
    } // Q_ASSERT( false ); return QVector3D(0,0,0); }

    QSize theWindow = m_contextInterface->windowDimensions();
    QVector2D theDims(float(theWindow.width()), float(theWindow.height()));

    QSSGLayerRenderPreparationResult &thePrepResult(*theData->layerPrepResult);
    const auto camera = thePrepResult.camera();
    const auto viewport = thePrepResult.viewport();
    QSSGRenderRay theRay = QSSGLayerRenderHelper::pickRay(*camera, viewport, inMouseVec, theDims, true);

    return theData->camera->unprojectToPosition(inPosition, theRay);
}

QVector3D QSSGRendererImpl::unprojectWithDepth(QSSGRenderNode &inNode, QVector3D &, const QVector3D &inMouseVec) const
{
    // Translate mouse into layer's coordinates
    const QSSGRef<QSSGLayerRenderData> &theData = const_cast<QSSGRendererImpl &>(*this).getOrCreateLayerRenderDataForNode(inNode);
    if (theData == nullptr || theData->camera == nullptr) {
        return QVector3D(0, 0, 0);
    } // Q_ASSERT( false ); return QVector3D(0,0,0); }

    // Flip the y into gl coordinates from window coordinates.
    QVector2D theMouse(inMouseVec.x(), inMouseVec.y());
    float theDepth = inMouseVec.z();

    QSSGLayerRenderPreparationResult &thePrepResult(*theData->layerPrepResult);
    QSize theWindow = m_contextInterface->windowDimensions();
    const auto camera = thePrepResult.camera();
    const auto viewport = thePrepResult.viewport();
    QSSGRenderRay theRay = QSSGLayerRenderHelper::pickRay(*camera, viewport, theMouse, QVector2D(float(theWindow.width()), float(theWindow.height())), true);
    QVector3D theTargetPosition = theRay.origin + theRay.direction * theDepth;
    if (inNode.parent != nullptr && inNode.parent->type != QSSGRenderGraphObject::Type::Layer)
        theTargetPosition = mat44::transform(inNode.parent->globalTransform.inverted(), theTargetPosition);
    return theTargetPosition;
}

QVector3D QSSGRendererImpl::projectPosition(QSSGRenderNode &inNode, const QVector3D &inPosition) const
{
    // Translate mouse into layer's coordinates
    const QSSGRef<QSSGLayerRenderData> &theData = const_cast<QSSGRendererImpl &>(*this).getOrCreateLayerRenderDataForNode(inNode);
    if (theData == nullptr || theData->camera == nullptr) {
        return QVector3D(0, 0, 0);
    }

    QMatrix4x4 viewProj;
    theData->camera->calculateViewProjectionMatrix(viewProj);
    QVector4D projPos = mat44::transform(viewProj, QVector4D(inPosition, 1.0f));
    projPos.setX(projPos.x() / projPos.w());
    projPos.setY(projPos.y() / projPos.w());

    QRectF theViewport = theData->layerPrepResult->viewport();
    QVector2D theDims(float(theViewport.width()), float(theViewport.height()));
    projPos.setX(projPos.x() + 1.0f);
    projPos.setY(projPos.y() + 1.0f);
    projPos.setX(projPos.x() * 0.5f);
    projPos.setY(projPos.y() * 0.5f);
    QVector3D cameraToObject = theData->camera->getGlobalPos() - inPosition;
    projPos.setZ(sqrtf(QVector3D::dotProduct(cameraToObject, cameraToObject)));
    QVector3D mouseVec = QVector3D(projPos.x(), projPos.y(), projPos.z());
    mouseVec.setX(mouseVec.x() * theDims.x());
    mouseVec.setY(mouseVec.y() * theDims.y());

    mouseVec.setX(mouseVec.x() + float(theViewport.x()));
    mouseVec.setY(mouseVec.y() + float(theViewport.y()));

    // Flip the y into window coordinates so it matches the mouse.
    QSize theWindow = m_contextInterface->windowDimensions();
    mouseVec.setY(theWindow.height() - mouseVec.y());

    return mouseVec;
}

QSSGOption<QSSGLayerPickSetup> QSSGRendererImpl::getLayerPickSetup(QSSGRenderLayer &inLayer,
                                                                         const QVector2D &inMouseCoords,
                                                                         const QSize &inPickDims)
{
    const QSSGRef<QSSGLayerRenderData> &theData = getOrCreateLayerRenderDataForNode(inLayer);
    if (Q_UNLIKELY(theData == nullptr || theData->camera == nullptr)) {
        Q_ASSERT(false);
        return QSSGEmpty();
    }
    QSize theWindow = m_contextInterface->windowDimensions();
    QVector2D theDims(float(theWindow.width()), float(theWindow.height()));
    // The mouse is relative to the layer
    QSSGOption<QVector2D> theLocalMouse = getLayerMouseCoords(*theData, inMouseCoords, theDims, false);
    if (!theLocalMouse.hasValue())
        return QSSGEmpty();

    QSSGLayerRenderPreparationResult &thePrepResult(*theData->layerPrepResult);
    if (thePrepResult.camera() == nullptr) {
        return QSSGEmpty();
    }
    // Perform gluPickMatrix and pre-multiply it into the view projection
    QSSGRenderCamera &theCamera(*thePrepResult.camera());

    QRectF layerToPresentation = thePrepResult.viewport();
    // Offsetting is already taken care of in the camera's projection.
    // All we need to do is to scale and translate the image.
    layerToPresentation.setX(0);
    layerToPresentation.setY(0);
    QVector2D theMouse(*theLocalMouse);
    // The viewport will need to center at this location
    QVector2D viewportDims(float(inPickDims.width()), float(inPickDims.height()));
    QVector2D bottomLeft = QVector2D(theMouse.x() - viewportDims.x() / 2.0f, theMouse.y() - viewportDims.y() / 2.0f);
    // For some reason, and I haven't figured out why yet, the offsets need to be backwards for
    // this to work.
    // bottomLeft.x = layerToPresentation.m_Width - bottomLeft.x;
    // bottomLeft.y = layerToPresentation.m_Height - bottomLeft.y;
    // Virtual rect is relative to the layer.
    QRectF thePickRect(qreal(bottomLeft.x()), qreal(bottomLeft.y()), qreal(viewportDims.x()), qreal(viewportDims.y()));
    QMatrix4x4 projectionPremult;
    projectionPremult = QSSGRenderContext::applyVirtualViewportToProjectionMatrix(projectionPremult, layerToPresentation, thePickRect);
    projectionPremult = projectionPremult.inverted();

    QMatrix4x4 globalInverse = theCamera.globalTransform.inverted();
    QMatrix4x4 theVP = theCamera.projection * globalInverse;
    // For now we won't setup the scissor, so we may be off by inPickDims at most because
    // GetLayerMouseCoords will return
    // false if the mouse is too far off the layer.
    return QSSGLayerPickSetup(projectionPremult,
                                theVP,
                                QRect(0, 0, int(layerToPresentation.width()), int(layerToPresentation.height())));
}

QSSGOption<QRectF> QSSGRendererImpl::layerRect(QSSGRenderLayer &inLayer)
{
    QSSGRef<QSSGLayerRenderData> theData = getOrCreateLayerRenderDataForNode(inLayer);
    if (Q_UNLIKELY(theData == nullptr || theData->camera == nullptr)) {
        Q_ASSERT(false);
        return QSSGEmpty();
    }
    QSSGLayerRenderPreparationResult &thePrepResult(*theData->layerPrepResult);
    return thePrepResult.viewport();
}

// This doesn't have to be cheap.
void QSSGRendererImpl::runLayerRender(QSSGRenderLayer &inLayer, const QMatrix4x4 &inViewProjection)
{
    QSSGRef<QSSGLayerRenderData> theData = getOrCreateLayerRenderDataForNode(inLayer);
    if (Q_UNLIKELY(theData == nullptr || theData->camera == nullptr)) {
        Q_ASSERT(false);
        return;
    }
    theData->prepareAndRender(inViewProjection);
}

void QSSGRendererImpl::renderLayerRect(QSSGRenderLayer &inLayer, const QVector3D &inColor)
{
    QSSGRef<QSSGLayerRenderData> theData = getOrCreateLayerRenderDataForNode(inLayer);
    if (theData)
        theData->m_boundingRectColor = inColor;
}

void QSSGRendererImpl::releaseLayerRenderResources(QSSGRenderLayer &inLayer)
{
    auto theIter = m_instanceRenderMap.find(&inLayer);
    if (theIter != m_instanceRenderMap.end()) {
        auto theLastFrm = std::find(m_lastFrameLayers.begin(), m_lastFrameLayers.end(), theIter.value());
        if (theLastFrm != m_lastFrameLayers.end()) {
            theIter.value()->resetForFrame();
            m_lastFrameLayers.erase(theLastFrm);
        }
        m_instanceRenderMap.erase(theIter);
    }
}

void QSSGRendererImpl::renderQuad()
{
    m_context->setCullingEnabled(false);
    generateXYQuad();
    m_context->setInputAssembler(m_quadInputAssembler);
    m_context->draw(QSSGRenderDrawMode::Triangles, m_quadIndexBuffer->numIndices(), 0);
}

void QSSGRendererImpl::renderFlippedQuad(const QVector2D &inDimensions, const QMatrix4x4 &inMVP, QSSGRenderTexture2D &inQuadTexture, float opacity)
{
    m_context->setCullingEnabled(false);
    m_context->setBlendingEnabled(true);
    m_context->setBlendFunction(
                QSSGRenderBlendFunctionArgument(QSSGRenderSrcBlendFunc::One,
                                                QSSGRenderDstBlendFunc::OneMinusSrcAlpha,
                                                QSSGRenderSrcBlendFunc::One,
                                                QSSGRenderDstBlendFunc::OneMinusSrcAlpha));
    QSSGRef<QSSGFlippedQuadShader> theShader = getFlippedQuadShader();
    m_context->setActiveShader(theShader->shader);
    theShader->mvp.set(inMVP);
    theShader->dimensions.set(inDimensions);
    theShader->sampler.set(&inQuadTexture);
    theShader->opacity.set(opacity);

    generateXYQuad();
    m_context->setInputAssembler(m_quadInputAssembler);
    m_context->draw(QSSGRenderDrawMode::Triangles, m_quadIndexBuffer->numIndices(), 0);
}

void QSSGRendererImpl::layerNeedsFrameClear(QSSGLayerRenderData &inLayer)
{
    m_lastFrameLayers.push_back(&inLayer);
}

void QSSGRendererImpl::beginLayerDepthPassRender(QSSGLayerRenderData &inLayer)
{
    m_currentLayer = &inLayer;
}

void QSSGRendererImpl::endLayerDepthPassRender()
{
    m_currentLayer = nullptr;
}

void QSSGRendererImpl::beginLayerRender(QSSGLayerRenderData &inLayer)
{
    m_currentLayer = &inLayer;
}
void QSSGRendererImpl::endLayerRender()
{
    m_currentLayer = nullptr;
}

void QSSGRendererImpl::prepareImageForIbl(QSSGRenderImage &inImage)
{
    if (inImage.m_textureData.m_texture && inImage.m_textureData.m_texture->numMipmaps() < 1)
        inImage.m_textureData.m_texture->generateMipmaps();
}

bool nodeContainsBoneRoot(QSSGRenderNode &childNode, qint32 rootID)
{
    for (QSSGRenderNode *childChild = childNode.firstChild; childChild != nullptr; childChild = childChild->nextSibling) {
        if (childChild->skeletonId == rootID)
            return true;
    }

    return false;
}

void fillBoneIdNodeMap(QSSGRenderNode &childNode, QHash<long, QSSGRenderNode *> &ioMap)
{
    if (childNode.skeletonId >= 0)
        ioMap[childNode.skeletonId] = &childNode;
    for (QSSGRenderNode *childChild = childNode.firstChild; childChild != nullptr; childChild = childChild->nextSibling)
        fillBoneIdNodeMap(*childChild, ioMap);
}

QSSGOption<QVector2D> QSSGRendererImpl::getLayerMouseCoords(QSSGLayerRenderData &inLayerRenderData,
                                                                const QVector2D &inMouseCoords,
                                                                const QVector2D &inViewportDimensions,
                                                                bool forceImageIntersect) const
{
    if (inLayerRenderData.layerPrepResult.hasValue()) {
        const auto viewport = inLayerRenderData.layerPrepResult->viewport();
        return QSSGLayerRenderHelper::layerMouseCoords(viewport, inMouseCoords, inViewportDimensions, forceImageIntersect);
    }
    return QSSGEmpty();
}

bool QSSGRendererImpl::rendererRequestsFrames() const
{
    return m_progressiveAARenderRequest;
}

void QSSGRendererImpl::getLayerHitObjectList(QSSGLayerRenderData &inLayerRenderData,
                                               const QVector2D &inViewportDimensions,
                                               const QVector2D &inPresCoords,
                                               bool inPickEverything,
                                               TPickResultArray &outIntersectionResult)
{
    // This function assumes the layer was rendered to the scene itself. There is another
    // function for completely offscreen layers that don't get rendered to the scene.
    bool wasRenderToTarget(inLayerRenderData.layer.flags.testFlag(QSSGRenderLayer::Flag::LayerRenderToTarget));
    if (wasRenderToTarget && inLayerRenderData.camera != nullptr) {
        QSSGOption<QSSGRenderRay> theHitRay;
        if (inLayerRenderData.layerPrepResult.hasValue()) {
            const auto camera = inLayerRenderData.layerPrepResult->camera();
            const auto viewport = inLayerRenderData.layerPrepResult->viewport();
            theHitRay = QSSGLayerRenderHelper::pickRay(*camera, viewport, inPresCoords, inViewportDimensions, false);
        }

        if (theHitRay.hasValue()) {
            // Scale the mouse coords to change them into the camera's numerical space.
            QSSGRenderRay thePickRay = *theHitRay;
            for (int idx = inLayerRenderData.opaqueObjects.size(), end = 0; idx > end; --idx) {
                QSSGRenderableObject *theRenderableObject = inLayerRenderData.opaqueObjects.at(idx - 1).obj;
                if (inPickEverything || theRenderableObject->renderableFlags.isPickable())
                    intersectRayWithSubsetRenderable(thePickRay, *theRenderableObject, outIntersectionResult);
            }
            for (int idx = inLayerRenderData.transparentObjects.size(), end = 0; idx > end; --idx) {
                QSSGRenderableObject *theRenderableObject = inLayerRenderData.transparentObjects.at(idx - 1).obj;
                if (inPickEverything || theRenderableObject->renderableFlags.isPickable())
                    intersectRayWithSubsetRenderable(thePickRay, *theRenderableObject, outIntersectionResult);
            }
        }
    }
}

using RenderableList = QVarLengthArray<const QSSGRenderNode *>;
static void dfs(const QSSGRenderNode &node, RenderableList &renderables)
{
    if (node.isRenderableType())
        renderables.push_back(&node);

    for (QSSGRenderNode *child = node.firstChild; child != nullptr; child = child->nextSibling)
        dfs(*child, renderables);
}

void QSSGRendererImpl::getLayerHitObjectList(const QSSGRenderLayer &layer,
                                             const QSSGRef<QSSGBufferManager> &bufferManager,
                                             const QVector2D &inViewportDimensions,
                                             const QVector2D &inPresCoords,
                                             bool inPickEverything,
                                             PickResultList &outIntersectionResult)
{

    // This function assumes the layer was rendered to the scene itself. There is another
    // function for completely offscreen layers that don't get rendered to the scene.
    const bool wasRenderToTarget(layer.flags.testFlag(QSSGRenderLayer::Flag::LayerRenderToTarget));
    if (wasRenderToTarget && layer.renderedCamera != nullptr) {
        const auto camera = layer.renderedCamera;
        // TODO: Need to make sure we get the right Viewport rect here.
        const auto viewport = QRectF(QPointF(), QSizeF(qreal(inViewportDimensions.x()), qreal(inViewportDimensions.y())));
        const QSSGOption<QSSGRenderRay> hitRay = QSSGLayerRenderHelper::pickRay(*camera, viewport, inPresCoords, inViewportDimensions, false);
        if (hitRay.hasValue()) {
            // Scale the mouse coords to change them into the camera's numerical space.
            RenderableList renderables;
            for (QSSGRenderNode *childNode = layer.firstChild; childNode; childNode = childNode->nextSibling)
                dfs(*childNode, renderables);

            for (int idx = renderables.size(), end = 0; idx > end; --idx) {
                const auto &pickableObject = renderables.at(idx - 1);
                if (inPickEverything || pickableObject->flags.testFlag(QSSGRenderNode::Flag::LocallyPickable))
                    intersectRayWithSubsetRenderable(bufferManager, *hitRay, *pickableObject, outIntersectionResult);
            }
        }
    }
}

void QSSGRendererImpl::intersectRayWithSubsetRenderable(const QSSGRef<QSSGBufferManager> &bufferManager,
                                                        const QSSGRenderRay &inRay,
                                                        const QSSGRenderNode &node,
                                                        QSSGRendererImpl::PickResultList &outIntersectionResultList)
{
    if (node.type != QSSGRenderGraphObject::Type::Model)
        return;

    const QSSGRenderModel &model = static_cast<const QSSGRenderModel &>(node);

    // TODO: Technically we should have some guard here, as the meshes are usually loaded on a different thread,
    // so this isn't really nice (assumes all meshes are loaded before picking and none are removed, which currently should be the case).
    auto mesh = bufferManager->getMesh(model.meshPath);
    if (!mesh && !model.geometry)
        return;

    const auto &globalTransform = model.globalTransform;
    auto rayData = QSSGRenderRay::createRayData(globalTransform, inRay);

    // If this is a custom mesh, then only test against the bounding box
    if (model.geometry) {
        QSSGBounds3 modelBounds(model.geometry->boundsMin(), model.geometry->boundsMax());
        auto hit = QSSGRenderRay::intersectWithAABBv2(rayData, modelBounds);
        if (!hit.intersects())
            return;
        auto intersectionResult = QSSGRenderRay::createIntersectionResult(rayData, hit);
        outIntersectionResultList.push_back(QSSGRenderPickResult(model,
                                                                 intersectionResult.rayLengthSquared,
                                                                 intersectionResult.relXY,
                                                                 intersectionResult.scenePosition));
        return;
    }

    const auto &subMeshes = mesh->subsets;
    QSSGBounds3 modelBounds = QSSGBounds3::empty();
    for (const auto &subMesh : subMeshes)
        modelBounds.include(subMesh.bounds);

    if (modelBounds.isEmpty())
        return;

    auto hit = QSSGRenderRay::intersectWithAABBv2(rayData, modelBounds);

    // If we don't intersect with the model at all, then there's no need to go furher down!
    if (!hit.intersects())
        return;

    // Check each submesh to find the closest intersection point
    float minRayLength = std::numeric_limits<float>::max();
    QSSGRenderRay::IntersectionResult intersectionResult;
    QVector<QSSGRenderRay::IntersectionResult> results;

    for (const auto &subMesh : subMeshes) {
        QSSGRenderRay::IntersectionResult result;
        if (subMesh.bvhRoot) {
            hit = QSSGRenderRay::intersectWithAABBv2(rayData, subMesh.bvhRoot->boundingData);
            if (hit.intersects()) {
                results.clear();
                inRay.intersectWithBVH(rayData, subMesh.bvhRoot, mesh, results);
                float subMeshMinRayLength = std::numeric_limits<float>::max();
                for (const auto &subMeshResult : qAsConst(results)) {
                    if (subMeshResult.rayLengthSquared < subMeshMinRayLength) {
                        result = subMeshResult;
                        subMeshMinRayLength = result.rayLengthSquared;
                    }
                }
            }
        } else {
            hit = QSSGRenderRay::intersectWithAABBv2(rayData, subMesh.bounds);
            if (hit.intersects())
                result = QSSGRenderRay::createIntersectionResult(rayData, hit);
        }
        if (result.intersects && result.rayLengthSquared < minRayLength) {
            intersectionResult = result;
            minRayLength = intersectionResult.rayLengthSquared;
        }
    }

    if (!intersectionResult.intersects)
        return;

    outIntersectionResultList.push_back(
                                QSSGRenderPickResult(model,
                                                     intersectionResult.rayLengthSquared,
                                                     intersectionResult.relXY,
                                                     intersectionResult.scenePosition));
}

void QSSGRendererImpl::intersectRayWithSubsetRenderable(const QSSGRenderRay &inRay,
                                                        QSSGRenderableObject &inRenderableObject,
                                                        TPickResultArray &outIntersectionResultList)
{
    QSSGRenderRay::IntersectionResult intersectionResult = QSSGRenderRay::intersectWithAABB(inRenderableObject.globalTransform, inRenderableObject.bounds, inRay);
    if (!intersectionResult.intersects)
        return;

    // Leave the coordinates relative for right now.
    const QSSGRenderGraphObject *thePickObject = nullptr;
    if (inRenderableObject.renderableFlags.isDefaultMaterialMeshSubset())
        thePickObject = &static_cast<QSSGSubsetRenderable *>(&inRenderableObject)->modelContext.model;
    else if (inRenderableObject.renderableFlags.isCustomMaterialMeshSubset())
        thePickObject = &static_cast<QSSGCustomMaterialRenderable *>(&inRenderableObject)->modelContext.model;

    if (thePickObject != nullptr) {
        outIntersectionResultList.push_back(
                                    QSSGRenderPickResult(*thePickObject,
                                                         intersectionResult.rayLengthSquared,
                                                         intersectionResult.relXY,
                                                         intersectionResult.scenePosition));
    }
}

QSSGRef<QSSGRenderShaderProgram> QSSGRendererImpl::compileShader(const QByteArray &inName, const char *inVert, const char *inFrag)
{
    getProgramGenerator()->beginProgram();
    getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Vertex)->append(inVert);
    getProgramGenerator()->getStage(QSSGShaderGeneratorStage::Fragment)->append(inFrag);
    return getProgramGenerator()->compileGeneratedShader(inName);
}

QSSGRef<QSSGShaderGeneratorGeneratedShader> QSSGRendererImpl::getShader(QSSGSubsetRenderable &inRenderable,
                                                                              const ShaderFeatureSetList &inFeatureSet)
{
    if (Q_UNLIKELY(m_currentLayer == nullptr)) {
        Q_ASSERT(false);
        return nullptr;
    }
    auto shaderIt = m_shaders.constFind(inRenderable.shaderDescription);
    if (shaderIt == m_shaders.cend()) {
        // Generate the shader.
        const QSSGRef<QSSGRenderShaderProgram> &theShader(generateShader(inRenderable, inFeatureSet));
        if (theShader) {
            QSSGRef<QSSGShaderGeneratorGeneratedShader> theGeneratedShader = QSSGRef<QSSGShaderGeneratorGeneratedShader>(
                    new QSSGShaderGeneratorGeneratedShader(m_generatedShaderString, theShader));
            shaderIt = m_shaders.insert(inRenderable.shaderDescription, theGeneratedShader);
        } else {
            // We still insert something because we don't to attempt to generate the same bad shader
            // twice.
            shaderIt = m_shaders.insert(inRenderable.shaderDescription, nullptr);
        }
    }

    if (!shaderIt->isNull()) {
        if (m_currentLayer && m_currentLayer->camera) {
            QSSGRenderCamera &theCamera(*m_currentLayer->camera);
            if (!m_currentLayer->cameraDirection.hasValue())
                m_currentLayer->cameraDirection = theCamera.getScalingCorrectDirection();
        }
    }
    return *shaderIt;
}
static QVector3D g_fullScreenRectFace[] = {
    QVector3D(-1, -1, 0),
    QVector3D(-1, 1, 0),
    QVector3D(1, 1, 0),
    QVector3D(1, -1, 0),
};

static QVector2D g_fullScreenRectUVs[] = { QVector2D(0, 0), QVector2D(0, 1), QVector2D(1, 1), QVector2D(1, 0) };

void QSSGRendererImpl::generateXYQuad()
{
    if (m_quadInputAssembler)
        return;

    QSSGRenderVertexBufferEntry theEntries[] = {
        QSSGRenderVertexBufferEntry("attr_pos", QSSGRenderComponentType::Float32, 3),
        QSSGRenderVertexBufferEntry("attr_uv", QSSGRenderComponentType::Float32, 2, 12),
    };

    float tempBuf[20];
    float *bufPtr = tempBuf;
    QVector3D *facePtr(g_fullScreenRectFace);
    QVector2D *uvPtr(g_fullScreenRectUVs);
    for (int j = 0; j < 4; j++, ++facePtr, ++uvPtr, bufPtr += 5) {
        bufPtr[0] = facePtr->x();
        bufPtr[1] = facePtr->y();
        bufPtr[2] = facePtr->z();
        bufPtr[3] = uvPtr->x();
        bufPtr[4] = uvPtr->y();
    }
    m_quadVertexBuffer = new QSSGRenderVertexBuffer(m_context, QSSGRenderBufferUsageType::Static,
                                                       3 * sizeof(float) + 2 * sizeof(float),
                                                       toByteView(tempBuf, 20));

    quint8 indexData[] = {
        0, 1, 2, 0, 2, 3,
    };
    m_quadIndexBuffer = new QSSGRenderIndexBuffer(m_context, QSSGRenderBufferUsageType::Static,
                                                     QSSGRenderComponentType::UnsignedInteger8,
                                                     toByteView(indexData, sizeof(indexData)));

    // create our attribute layout
    m_quadAttribLayout = m_context->createAttributeLayout(toDataView(theEntries, 2));

    // create input assembler object
    quint32 strides = m_quadVertexBuffer->stride();
    quint32 offsets = 0;
    m_quadInputAssembler = m_context->createInputAssembler(m_quadAttribLayout,
                                                           toDataView(&m_quadVertexBuffer, 1),
                                                           m_quadIndexBuffer,
                                                           toDataView(&strides, 1),
                                                           toDataView(&offsets, 1));
}

void QSSGRendererImpl::generateXYZPoint()
{
    if (m_pointInputAssembler)
        return;

    QSSGRenderVertexBufferEntry theEntries[] = {
        QSSGRenderVertexBufferEntry("attr_pos", QSSGRenderComponentType::Float32, 3),
        QSSGRenderVertexBufferEntry("attr_uv", QSSGRenderComponentType::Float32, 2, 12),
    };

    float tempBuf[5] { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    m_pointVertexBuffer = new QSSGRenderVertexBuffer(m_context, QSSGRenderBufferUsageType::Static,
                                                        3 * sizeof(float) + 2 * sizeof(float),
                                                        toByteView(tempBuf, 5));

    // create our attribute layout
    m_pointAttribLayout = m_context->createAttributeLayout(toDataView(theEntries, 2));

    // create input assembler object
    quint32 strides = m_pointVertexBuffer->stride();
    quint32 offsets = 0;
    m_pointInputAssembler = m_context->createInputAssembler(m_pointAttribLayout,
                                                            toDataView(&m_pointVertexBuffer, 1),
                                                            nullptr,
                                                            toDataView(&strides, 1),
                                                            toDataView(&offsets, 1));
}

QPair<QSSGRef<QSSGRenderVertexBuffer>, QSSGRef<QSSGRenderIndexBuffer>> QSSGRendererImpl::getXYQuad()
{
    if (!m_quadInputAssembler)
        generateXYQuad();

    return QPair<QSSGRef<QSSGRenderVertexBuffer>, QSSGRef<QSSGRenderIndexBuffer>>(m_quadVertexBuffer, m_quadIndexBuffer);
}

QSSGLayerGlobalRenderProperties QSSGRendererImpl::getLayerGlobalRenderProperties()
{
    QSSGLayerRenderData &theData = *m_currentLayer;
    const QSSGRenderLayer &theLayer = theData.layer;
    if (!theData.cameraDirection.hasValue())
        theData.cameraDirection = theData.camera->getScalingCorrectDirection();

    return QSSGLayerGlobalRenderProperties{ theLayer,
                                              *theData.camera,
                                              *theData.cameraDirection,
                                              theData.globalLights,
                                              theData.lightDirections,
                                              theData.shadowMapManager,
                                              theData.m_layerDepthTexture,
                                              theData.m_layerSsaoTexture,
                                              theLayer.lightProbe,
                                              theLayer.lightProbe2,
                                              theLayer.probeHorizon,
                                              theLayer.probeBright,
                                              theLayer.probe2Window,
                                              theLayer.probe2Pos,
                                              theLayer.probe2Fade,
                                              theLayer.probeFov };
}

void QSSGRendererImpl::generateXYQuadStrip()
{
    if (m_quadStripInputAssembler)
        return;

    QSSGRenderVertexBufferEntry theEntries[] = {
        QSSGRenderVertexBufferEntry("attr_pos", QSSGRenderComponentType::Float32, 3),
        QSSGRenderVertexBufferEntry("attr_uv", QSSGRenderComponentType::Float32, 2, 12),
    };

    // this buffer is filled dynmically
    m_quadStripVertexBuffer = new QSSGRenderVertexBuffer(m_context, QSSGRenderBufferUsageType::Dynamic,
                                                            3 * sizeof(float) + 2 * sizeof(float), // stride
                                                            QSSGByteView());

    // create our attribute layout
    m_quadStripAttribLayout = m_context->createAttributeLayout(toDataView(theEntries, 2));

    // create input assembler object
    quint32 strides = m_quadStripVertexBuffer->stride();
    quint32 offsets = 0;
    m_quadStripInputAssembler = m_context->createInputAssembler(m_quadStripAttribLayout,
                                                                toDataView(&m_quadStripVertexBuffer, 1),
                                                                nullptr,
                                                                toDataView(&strides, 1),
                                                                toDataView(&offsets, 1));
}

void QSSGRendererImpl::updateCbAoShadow(const QSSGRenderLayer *pLayer, const QSSGRenderCamera *pCamera, QSSGResourceTexture2D &inDepthTexture)
{
    if (m_context->supportsConstantBuffer()) {
        const char *theName = "aoShadow";
        QSSGRef<QSSGRenderConstantBuffer> pCB = m_context->getConstantBuffer(theName);

        if (!pCB) {
            // the  size is determined automatically later on
            pCB = new QSSGRenderConstantBuffer(m_context, theName,
                                                  QSSGRenderBufferUsageType::Static,
                                                  QSSGByteView());
            if (!pCB) {
                Q_ASSERT(false);
                return;
            }
            m_constantBuffers.insert(theName, pCB);

            // Add paramters. Note should match the appearance in the shader program
            pCB->addParam(QSSGRenderConstantBuffer::ParamData<QSSGRenderConstantBuffer::Param::AoProperties>::handle(), QSSGRenderShaderDataType::Vec4, 1);
            pCB->addParam(QSSGRenderConstantBuffer::ParamData<QSSGRenderConstantBuffer::Param::AoProperties2>::handle(), QSSGRenderShaderDataType::Vec4, 1);
            pCB->addParam(QSSGRenderConstantBuffer::ParamData<QSSGRenderConstantBuffer::Param::ShadowProperties>::handle(), QSSGRenderShaderDataType::Vec4, 1);
            pCB->addParam(QSSGRenderConstantBuffer::ParamData<QSSGRenderConstantBuffer::Param::AoScreenConst>::handle(), QSSGRenderShaderDataType::Vec4, 1);
            pCB->addParam(QSSGRenderConstantBuffer::ParamData<QSSGRenderConstantBuffer::Param::UvToEyeConst>::handle(), QSSGRenderShaderDataType::Vec4, 1);
        }

        // update values
        QVector4D aoProps(pLayer->aoStrength * 0.01f, pLayer->aoDistance * 0.4f, pLayer->aoSoftness * 0.02f, pLayer->aoBias);
        pCB->updateParam(QSSGRenderConstantBuffer::ParamData<QSSGRenderConstantBuffer::Param::AoProperties>::handle(), toByteView(aoProps));
        QVector4D aoProps2(float(pLayer->aoSamplerate), (pLayer->aoDither) ? 1.0f : 0.0f, 0.0f, 0.0f);
        pCB->updateParam(QSSGRenderConstantBuffer::ParamData<QSSGRenderConstantBuffer::Param::AoProperties2>::handle(), toByteView(aoProps2));
        QVector4D shadowProps(pLayer->shadowStrength * 0.01f, pLayer->shadowDist, pLayer->shadowSoftness * 0.01f, pLayer->shadowBias);
        pCB->updateParam(QSSGRenderConstantBuffer::ParamData<QSSGRenderConstantBuffer::Param::ShadowProperties>::handle(), toByteView(shadowProps));

        float R2 = pLayer->aoDistance * pLayer->aoDistance * 0.16f;
        float rw = 100, rh = 100;

        if (inDepthTexture.getTexture()) {
            rw = float(inDepthTexture.getTexture()->textureDetails().width);
            rh = float(inDepthTexture.getTexture()->textureDetails().height);
        }
        float fov = (pCamera) ? pCamera->verticalFov(rw / rh) : 1.0f;
        float tanHalfFovY = tanf(0.5f * fov * (rh / rw));
        float invFocalLenX = tanHalfFovY * (rw / rh);

        QVector4D aoScreenConst(1.0f / R2, rh / (2.0f * tanHalfFovY), 1.0f / rw, 1.0f / rh);
        pCB->updateParam(QSSGRenderConstantBuffer::ParamData<QSSGRenderConstantBuffer::Param::AoScreenConst>::handle(), toByteView(aoScreenConst));
        QVector4D uvToEyeConst(2.0f * invFocalLenX, -2.0f * tanHalfFovY, -invFocalLenX, tanHalfFovY);
        pCB->updateParam(QSSGRenderConstantBuffer::ParamData<QSSGRenderConstantBuffer::Param::UvToEyeConst>::handle(), toByteView(uvToEyeConst));

        // update buffer to hardware
        pCB->update();
    }
}







const QSSGRef<QSSGShaderProgramGeneratorInterface> &QSSGRendererImpl::getProgramGenerator()
{
    return m_contextInterface->shaderProgramGenerator();
}

void QSSGRendererImpl::dumpGpuProfilerStats()
{
    if (!isLayerGpuProfilingEnabled())
        return;

    auto it = m_instanceRenderMap.cbegin();
    const auto end = m_instanceRenderMap.cend();
    for (; it != end; it++) {
        const QSSGRef<QSSGLayerRenderData> &theLayerRenderData = it.value();
        const QSSGRenderLayer *theLayer = &theLayerRenderData->layer;

        if (theLayer->flags.testFlag(QSSGRenderLayer::Flag::Active) && theLayerRenderData->m_layerProfilerGpu) {
            const QVector<QString> &idList = theLayerRenderData->m_layerProfilerGpu->timerIDs();
            if (!idList.empty()) {
#if QSSG_DEBUG_ID
                qDebug() << theLayer->id;
#endif
                auto theIdIter = idList.begin();
                for (; theIdIter != idList.end(); theIdIter++) {
                    char messageLine[1024];
                    sprintf(messageLine,
                            "%s: %.3f ms",
                            theIdIter->toLatin1().constData(),
                            theLayerRenderData->m_layerProfilerGpu->elapsed(*theIdIter));
                    qDebug() << "    " << messageLine;
                }
            }
        }
    }
}

QSSGOption<QVector2D> QSSGRendererImpl::getLayerMouseCoords(QSSGRenderLayer &inLayer,
                                                                const QVector2D &inMouseCoords,
                                                                const QVector2D &inViewportDimensions,
                                                                bool forceImageIntersect) const
{
    QSSGRef<QSSGLayerRenderData> theData = const_cast<QSSGRendererImpl &>(*this).getOrCreateLayerRenderDataForNode(inLayer);
    return getLayerMouseCoords(*theData, inMouseCoords, inViewportDimensions, forceImageIntersect);
}

bool QSSGRendererInterface::isGlEsContext(const QSSGRenderContextType &inContextType)
{
    QSSGRenderContextTypes esContextTypes(QSSGRenderContextType::GLES2 | QSSGRenderContextType::GLES3
                                           | QSSGRenderContextType::GLES3PLUS);

    return (esContextTypes & inContextType);
}

bool QSSGRendererInterface::isGlEs3Context(const QSSGRenderContextType &inContextType)
{
    return (inContextType == QSSGRenderContextType::GLES3 || inContextType == QSSGRenderContextType::GLES3PLUS);
}

bool QSSGRendererInterface::isGl2Context(const QSSGRenderContextType &inContextType)
{
    return (inContextType == QSSGRenderContextType::GL2);
}

QSSGRef<QSSGRendererInterface> QSSGRendererInterface::createRenderer(QSSGRenderContextInterface *inContext)
{
    return QSSGRef<QSSGRendererImpl>(new QSSGRendererImpl(inContext));
}

QSSGRenderPickResult::QSSGRenderPickResult(const QSSGRenderGraphObject &inHitObject,
                                           float inCameraDistance,
                                           const QVector2D &inLocalUVCoords,
                                           const QVector3D &scenePosition)
    : m_hitObject(&inHitObject)
    , m_cameraDistanceSq(inCameraDistance)
    , m_localUVCoords(inLocalUVCoords)
    , m_scenePosition(scenePosition)
{
}

QT_END_NAMESPACE
