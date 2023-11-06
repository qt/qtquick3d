// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "qssgrendercontextcore.h"
#include "qssgrenderhelpers.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderhelpers_p.h>
#include <QtQuick3DRuntimeRender/private/qssglayerrenderdata_p.h>

#include "graphobjects/qssgrendermodel_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>

#include <QtQuick3DUtils/private/qssgassert_p.h>

QT_BEGIN_NAMESPACE

 /*!
    \class QSSGRenderHelpers
    \inmodule QtQuick3D
    \since 6.7

    \brief Class containing helper functions for setting up and render QtQuick3D renderables.
*/

/*!
    \fn QSSGRenderablesId QSSGRenderHelpers::createRenderables(const QSSGFrameData &frameData,
                                                               QSSGPrepContextId prepId,
                                                               const NodeList &nodes,
                                                               CreateFlags flags)

    Takes a list of node ids and create renderables that can be further processed by the renderer.
    If there are no nodes, or no renderable nodes in the list, the returned id will be invalid.

    By default the function does not recurse down and included children of the \a nodes in the list.
    Enabling recursion can be achieved by passing in the \l{Recurse}{CreateFlags::Recurse} flag in
    the \a flags argument.

    \return an id to the created renderables.

    \sa CreateFlags, prepareForRender
 */


QSSGRenderablesId QSSGRenderHelpers::createRenderables(const QSSGFrameData &frameData,
                                                       QSSGPrepContextId prepId,
                                                       const NodeList &nodes,
                                                       CreateFlags flags)
{
    QSSGRenderablesId rid { QSSGRenderablesId::Uninitialized };
    if (nodes.size() > 0) {
        auto *layer = QSSGLayerRenderData::getCurrent(*frameData.renderer());
        QSSG_ASSERT_X(layer, "No active layer for renderer!", return rid);
        return layer->createRenderables(prepId, nodes, flags.testFlag(CreateFlag::Recurse));
    }

    return rid;
}

/*!
    \fn QSSGPrepContextId QSSGRenderHelpers::prepareForRender(const QSSGFrameData &frameData,
                                                              const QSSGRenderExtension &ext,
                                                              QSSGNodeId camera,
                                                              quint32 slot)

    prepareForRender() creates a context for collecting and storing information about the render-data
    associated with this render extension.

    If the same nodes are to be rendered more then once but with different properties, for example
    a different material or camera, then a new context will be needed. To create several contexts for
    one extension the \a slot argument can be used. The default context is created in slot \b 0.

    \return an id to the prep context.

    \sa commit
 */

QSSGPrepContextId QSSGRenderHelpers::prepareForRender(const QSSGFrameData &frameData,
                                                      const QSSGRenderExtension &ext,
                                                      QSSGNodeId camera,
                                                      quint32 slot)
{
    auto *cn = QSSGRenderGraphObjectUtils::getNode<QSSGRenderCamera>(camera);
    QSSG_ASSERT_X(cn && QSSGRenderGraphObject::isCamera(cn->type), "NodeId is not a camera!", return QSSGPrepContextId::Uninitialized);
    auto *layer = QSSGLayerRenderData::getCurrent(*frameData.renderer());
    QSSG_ASSERT_X(layer, "No active layer for renderer!", return QSSGPrepContextId::Uninitialized);
    return layer->getOrCreateExtensionContext(ext, cn, slot);
}

/*!
    \fn QSSGPrepResultId QSSGRenderHelpers::commit(const QSSGFrameData &frameData,
                                                   QSSGPrepContextId prepId,
                                                   QSSGRenderablesId renderablesId,
                                                   float lodThreshold)

    Once the required changes have been done to the renderables, the data can marked as ready for
    the renderer.

    \return an id to the preparation result.

    \sa prepareRenderables, renderRenderables
 */

QSSGPrepResultId QSSGRenderHelpers::commit(const QSSGFrameData &frameData,
                                           QSSGPrepContextId prepId,
                                           QSSGRenderablesId renderablesId,
                                           float lodThreshold)
{
    auto *layer = QSSGLayerRenderData::getCurrent(*frameData.renderer());
    QSSG_ASSERT_X(layer, "No active layer for renderer!", return QSSGPrepResultId::Uninitialized);
    return layer->prepareModelsForRender(*frameData.renderer()->contextInterface(), prepId, renderablesId, lodThreshold);
}

/*!
    \fn QSSGPrepResultId QSSGRenderHelpers::prepareRenderables(const QSSGFrameData &frameData,
                                                               QRhiRenderPassDescriptor *renderPassDescriptor,
                                                               QSSGRhiGraphicsPipelineState &ps,
                                                               QSSGPrepResultId prepId)

    Prepare the draw call data needed for the renderables before calling \l {renderRenderables}.

    \return an id to the preparation result.

    \sa renderRenderables
 */

void QSSGRenderHelpers::prepareRenderables(const QSSGFrameData &frameData,
                                           QRhiRenderPassDescriptor *renderPassDescriptor,
                                           QSSGRhiGraphicsPipelineState &ps,
                                           QSSGPrepResultId prepId)
{
    auto *layer = QSSGLayerRenderData::getCurrent(*frameData.renderer());
    QSSG_ASSERT_X(layer, "No active layer for renderer!", return);
    layer->prepareRenderables(*frameData.renderer()->contextInterface(), renderPassDescriptor, ps, prepId);
}

/*!
    \fn void QSSGRenderHelpers::renderRenderables(QSSGRenderContextInterface &contextInterface,
                                                  QSSGPrepResultId prepId)

    Render the renderables.

    \sa prepareRenderables
 */

void QSSGRenderHelpers::renderRenderables(QSSGRenderContextInterface &contextInterface,
                                          QSSGPrepResultId prepId)
{
    auto *layer = QSSGLayerRenderData::getCurrent(*contextInterface.renderer());
    QSSG_ASSERT_X(layer, "No active layer for renderer!", return);
    layer->renderRenderables(contextInterface, prepId);
}

QSSGRenderHelpers::QSSGRenderHelpers()
{

}

 /*!
    \class QSSGModelHelpers
    \inmodule QtQuick3D
    \since 6.7

    \brief Class containing helper functions for modifying and setting data for model renderables.
*/

/*!
    \fn void QSSGModelHelpers::setModelMaterials(const QSSGFrameData &frameData,
                                                 QSSGRenderablesId renderablesId,
                                                 QSSGNodeId model,
                                                 MaterialList materials)

    Sets the \a materials to be used on \a model.

    \note As with the \l {QtQuick3D::Model::materials}{materials} on the \l {QtQuick3D::Model}{model} item,
    materials are applied in the same manner.

    \i "The sub-mesh uses a material from the \l{materials} list,
        corresponding to its index. If the number of materials is less than the
        sub-meshes, the last material in the list is used for subsequent sub-meshes."

    \sa createRenderables
 */

void QSSGModelHelpers::setModelMaterials(const QSSGFrameData &frameData,
                                         QSSGRenderablesId renderablesId,
                                         QSSGNodeId model,
                                         MaterialList materials)
{
    auto *layer = QSSGLayerRenderData::getCurrent(*frameData.renderer());
    QSSG_ASSERT_X(layer, "No active layer for renderer!", return);
    auto *renderModel = QSSGRenderGraphObjectUtils::getNode<QSSGRenderModel>(model);
    QSSG_ASSERT_X(renderModel && renderModel->type == QSSGRenderGraphObject::Type::Model, "Invalid model-id!", return);
    layer->setModelMaterials(renderablesId, *renderModel, materials);
}

/*!
    \fn void QSSGModelHelpers::setModelMaterials(const QSSGFrameData &frameData,
                                                 QSSGRenderablesId renderablesId,
                                                 MaterialList materials)

    Convenience function to apply \a materials to all models in the renderablesId set.

    \sa createRenderables
 */

void QSSGModelHelpers::setModelMaterials(const QSSGFrameData &frameData,
                                         QSSGRenderablesId renderablesId,
                                         MaterialList materials)
{
    auto *layer = QSSGLayerRenderData::getCurrent(*frameData.renderer());
    QSSG_ASSERT_X(layer, "No active layer for renderer!", return);
    layer->setModelMaterials(renderablesId, materials);
}

/*!
    \fn QMatrix4x4 QSSGModelHelpers::getGlobalTransformgetGlobalTransform(const QSSGFrameData &frameData,
                                                                          QSSGNodeId model,
                                                                          QSSGPrepContextId prepId = QSSGPrepContextId::Uninitialized)

    \return Returns the global transform for the \a model in context of the \a prepId. By default the prep context argument is
    \l {QSSGPrepContextId::Uninitialized}{Uninitialized} which returns the model's original global transform.


    \sa createRenderables
*/

QMatrix4x4 QSSGModelHelpers::getGlobalTransform(const QSSGFrameData &frameData,
                                                QSSGNodeId model,
                                                QSSGPrepContextId prepId)
{
    auto *layer = QSSGLayerRenderData::getCurrent(*frameData.renderer());
    QSSG_ASSERT_X(layer, "No active layer for renderer!", return {});
    auto *renderModel = QSSGRenderGraphObjectUtils::getNode<QSSGRenderModel>(model);
    QSSG_ASSERT_X(renderModel && renderModel->type == QSSGRenderGraphObject::Type::Model, "Invalid model-id!", return {});
    return (prepId != QSSGPrepContextId::Uninitialized) ? layer->getGlobalTransform(prepId, *renderModel)
                                                        : renderModel->globalTransform;
}

/*!
    \fn QMatrix4x4 QSSGModelHelpers::getLocalTransform(const QSSGFrameData &frameData, QSSGNodeId model)

    \return Returns the local transform for the \a model.
*/

QMatrix4x4 QSSGModelHelpers::getLocalTransform(const QSSGFrameData &frameData, QSSGNodeId model)
{
    Q_UNUSED(frameData);
    auto *renderModel = QSSGRenderGraphObjectUtils::getNode<QSSGRenderModel>(model);
    QSSG_ASSERT_X(renderModel && renderModel->type == QSSGRenderGraphObject::Type::Model, "Invalid model-id!", return {});
    return renderModel->localTransform;
}

/*!
    \fn float QSSGModelHelpers::getGlobalOpacity(const QSSGFrameData &frameData, QSSGNodeId model)

    \return Returns the global opacity for the \a model.
*/

float QSSGModelHelpers::getGlobalOpacity(const QSSGFrameData &frameData, QSSGNodeId model)
{
    Q_UNUSED(frameData);
    auto *renderModel = QSSGRenderGraphObjectUtils::getNode<QSSGRenderModel>(model);
    QSSG_ASSERT_X(renderModel && renderModel->type == QSSGRenderGraphObject::Type::Model, "Invalid model-id!", return {});
    return renderModel->globalOpacity;
}

/*!
    \fn float QSSGModelHelpers::getGlobalOpacity(const QSSGFrameData &frameData, QSSGNodeId model, QSSGPrepContextId prepId = QSSGPrepContextId::Uninitialized)

    \return Returns the global opacity for the \a model in context of the \a prepId. By default the prep context argument is
    \l {QSSGPrepContextId::Uninitialized}{Uninitialized} which returns the model's original global opacity.


    \sa createRenderables
*/

float QSSGModelHelpers::getGlobalOpacity(const QSSGFrameData &frameData, QSSGNodeId model, QSSGPrepContextId prepId = QSSGPrepContextId::Uninitialized)
{
    auto *layer = QSSGLayerRenderData::getCurrent(*frameData.renderer());
    QSSG_ASSERT_X(layer, "No active layer for renderer!", return {});
    auto *renderModel = QSSGRenderGraphObjectUtils::getNode<QSSGRenderModel>(model);
    QSSG_ASSERT_X(renderModel && renderModel->type == QSSGRenderGraphObject::Type::Model, "Invalid model-id!", return {});
    return (prepId != QSSGPrepContextId::Uninitialized) ? layer->getGlobalOpacity(prepId, *renderModel)
                                                        : renderModel->globalOpacity;
}

/*!
    \fn float QSSGModelHelpers::getLocalOpacity(const QSSGFrameData &frameData, QSSGNodeId model)

    \return Returns the local opacity for the \a model.
*/

float QSSGModelHelpers::getLocalOpacity(const QSSGFrameData &frameData, QSSGNodeId model)
{
    Q_UNUSED(frameData);
    auto *renderModel = QSSGRenderGraphObjectUtils::getNode<QSSGRenderModel>(model);
    QSSG_ASSERT_X(renderModel && renderModel->type == QSSGRenderGraphObject::Type::Model, "Invalid model-id!", return {});
    return renderModel->localOpacity;
}

/*!
    \fn void QSSGModelHelpers::setGlobalTransform(const QSSGFrameData &frameData,
                                                  QSSGRenderablesId renderablesId,
                                                  QSSGNodeId model,
                                                  const QMatrix4x4 &transform)

    Sets the global transform for \a model in the context of the \a renderablesId.

    \sa createRenderables
 */

void QSSGModelHelpers::setGlobalTransform(const QSSGFrameData &frameData,
                                          QSSGRenderablesId renderablesId,
                                          QSSGNodeId model,
                                          const QMatrix4x4 &transform)
{
    auto *layer = QSSGLayerRenderData::getCurrent(*frameData.renderer());
    QSSG_ASSERT_X(layer, "No active layer for renderer!", return);
    auto *node = QSSGRenderGraphObjectUtils::getNode(model);
    QSSG_ASSERT_X(node && node->type == QSSGRenderGraphObject::Type::Model, "Invalid model-id!", return);
    const auto &renderModel = static_cast<const QSSGRenderModel &>(*node);
    layer->setGlobalTransform(renderablesId, renderModel, transform);
}

/*!
    \fn void QSSGModelHelpers::setGlobalOpacity(const QSSGFrameData &frameData, QSSGRenderablesId renderablesId, QSSGNodeId model, float opacity)

    Sets the global opacity for \a model in the context of the \a renderablesId.

    \sa createRenderables
 */

void QSSGModelHelpers::setGlobalOpacity(const QSSGFrameData &frameData, QSSGRenderablesId renderablesId, QSSGNodeId model, float opacity)
{
    auto *layer = QSSGLayerRenderData::getCurrent(*frameData.renderer());
    QSSG_ASSERT_X(layer, "No active layer for renderer!", return);
    auto *node = QSSGRenderGraphObjectUtils::getNode(model);
    QSSG_ASSERT_X(node && node->type == QSSGRenderGraphObject::Type::Model, "Invalid model-id!", return);
    const auto &renderModel = static_cast<const QSSGRenderModel &>(*node);
    layer->setGlobalOpacity(renderablesId, renderModel, opacity);
}

 /*!
    \class QSSGCameraHelpers
    \inmodule QtQuick3D
    \since 6.7

    \brief Class containing helper functions for getting camera data used for rendering.
*/

/*!
    \fn QMatrix4x4 QSSGCameraHelpers::getViewProjectionMatrix(const QSSGNodeId cameraId,
                                                        const QMatrix4x4 *globalTransform = nullptr)

    Get the projection matrix for \a cameraId. An optional transform argument can be given to be used
    instead of the cameras global transform when calculating the projection matrix.

    \return projection matrix for \a cameraId

    \sa createRenderables
 */

QMatrix4x4 QSSGCameraHelpers::getViewProjectionMatrix(const QSSGNodeId cameraId,
                                                      const QMatrix4x4 *globalTransform)
{
    auto *renderCamera = QSSGRenderGraphObjectUtils::getNode<QSSGRenderCamera>(cameraId);
    QSSG_ASSERT(renderCamera && QSSGRenderGraphObject::isCamera(renderCamera->type), return {});

    QMatrix4x4 mat44{Qt::Uninitialized};
    const auto &projection = renderCamera->projection;
    const auto &transform = (globalTransform != 0) ? *globalTransform : renderCamera->globalTransform;
    QSSGRenderCamera::calculateViewProjectionMatrix(projection, transform, mat44);
    return mat44;
}

 /*!
    \class QSSGRenderExtensionHelpers
    \inmodule QtQuick3D
    \since 6.7

    \brief Class containing helper functions for the extensions.
*/

/*!
    \fn void QSSGRenderExtensionHelpers::registerRenderResult(const QSSGRenderContextInterface &contextInterface,
                                                              QSSGExtensionId extension,
                                                              QRhiTexture *texture)

    Register a render result, in form of a texture, for this \a extension. Once a texture is registered,
    the extension can be uses as a {QtQuick3D::Texture::textureProvider}{texture provider} in QML.

    \note To ensure that the \a texture is available for renderables, for example to be used by a {QtQuick3D::Texture} item,
    textures should be registered during the \l QSSGRenderExtension::prepareData call of the extension.

    \note Calling this function with a new texture will any previously registered texture.
    \note A texture can be unregistered by registering a nullptr for this extension.

    \sa {QtQuick3D::Texture::textureProvider}{textureProvider}
 */

void QSSGRenderExtensionHelpers::registerRenderResult(const QSSGRenderContextInterface &contextInterface,
                                                      QSSGExtensionId extension,
                                                      QRhiTexture *texture)
{
    if (auto *ext = QSSGRenderGraphObjectUtils::getExtension<QSSGRenderExtension>(extension))
        contextInterface.bufferManager()->registerExtensionResult(*ext, texture);
}

QT_END_NAMESPACE
