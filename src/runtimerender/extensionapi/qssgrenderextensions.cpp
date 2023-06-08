// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgrenderextensions_p.h"
#include "private/qssgassert_p.h"
#include "private/qssglayerrenderdata_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QSSGFrameData
    \inmodule QtQuick3DRuntimeRender
    \since 6.6

    \brief Storage class containing data collected for a frame.
*/

/*!
    \brief QSSGFrameData::getRenderPassResult(QSSGFrameData::RenderResult id)
    \return The renderable texture result from \a id. Null if no matching \a id was found.

    \note Even if the function returns a non-null result, the returned \l QSSGRhiRenderableTexture
    might not be ready unless the pass rendering to the texture has been executed.

    \note The returned value is only valid within the current frame. On each new frame
    the renderable will be reset and should therefore be queried again.
*/
const QSSGRhiRenderableTexture *QSSGFrameData::getRenderResult(RenderResult id) const
{
    const QSSGRhiRenderableTexture *res = nullptr;
    auto *data = QSSGLayerRenderData::getCurrent(*m_renderer);
    if (QSSG_GUARD(data && (std::size(data->renderResults) > RenderResultT(id))))
        res = data->getRenderResult(id);

    return res;
}

/*!
    \fn QSSGFrameData::getPipelineState() const
    \return Base pipeline state for this frame
 */

QSSGRhiGraphicsPipelineState QSSGFrameData::getPipelineState() const
{
    auto *data = QSSGLayerRenderData::getCurrent(*m_renderer);
    QSSG_ASSERT(data, return {});
    return data->getPipelineState();
}

/*!
    \fn QSSGFrameData::getNode(QSSGNodeId id) const
    \a id a \c QSSGNodeId
    \return The \l {QSSGRenderableNodeEntry}{renderable entry} that corresponds to the \c QSSGNodeId.

    \note This function does not remove the renderable entry which means any other extension or internal
    code processed after this extension will be able to process it as well.

    \sa QSSGFrameData::takeNode(), QQuick3DExtensionHelpers::getNodeId(), QSSGRenderableNodeEntry
*/

QSSGRenderableNodeEntry QSSGFrameData::getNode(QSSGNodeId id) const
{
    auto *data = QSSGLayerRenderData::getCurrent(*m_renderer);
    QSSG_ASSERT(data, return {});
    return data->getNode(id);
}

/*!
    \fn QSSGFrameData::takeNode(QSSGNodeId id)
    \a id a \c QSSGNodeId
    \return The \l {QSSGRenderableNodeEntry}{renderable entry} that corresponds to the provided \c QSSGNodeId.

    \note This function \b removes the renderable entry which means it won't be available for any other
    extension or internal code executed after this extension.

    \sa QSSGFrameData::takeNode(), QQuick3DExtensionHelpers::getNodeId(), QSSGRenderableNodeEntry
*/
QSSGRenderableNodeEntry QSSGFrameData::takeNode(QSSGNodeId id)
{
    auto *data = QSSGLayerRenderData::getCurrent(*m_renderer);
    QSSG_ASSERT(data, return {});
    return data->takeNode(id);
}

/*!
    \fn QSSGFrameData::getResource(QSSGResourceId id) const
    \a id a \c QSSGNodeId
    \return The \l {QSSGRenderGraphObject}{resource object} that corresponds to the provided \c QSSGResourceId.

    \sa QQuick3DExtensionHelpers::getResourceId()
*/
QSSGRenderGraphObject *QSSGFrameData::getResource(QSSGResourceId id) const
{
    auto *data = QSSGLayerRenderData::getCurrent(*m_renderer);
    QSSG_ASSERT(data, return {});
    return data->getResource(id);
}

/*!
    \fn QSSGFrameData::camera() const

    \return The main camera for the scene, or null if non could be found.

    \sa QSSGRenderCamera
*/
QSSGRenderCamera *QSSGFrameData::camera() const
{
    auto *data = QSSGLayerRenderData::getCurrent(*m_renderer);
    QSSG_ASSERT(data, return {});
    return data->activeCamera();
}

void QSSGFrameData::clear()
{

}

QSSGLayerRenderData *QSSGFrameData::getCurrent() const
{
    return QSSGLayerRenderData::getCurrent(*m_renderer);
}

QSSGFrameData::QSSGFrameData(QSSGRenderer *renderer)
    : m_renderer(renderer)
{

}

QSSGRenderExtension::QSSGRenderExtension()
    : QSSGRenderGraphObject(QSSGRenderGraphObject::Type::RenderExtension)
{

}

QSSGRenderExtension::~QSSGRenderExtension()
{

}

/*!
    \enum QSSGRenderExtension::Type

    Specifies the render extension type.

    \value Standalone The rendering code is recorded in full during the render prepare phase.
    This will usually imply that there are some output crated for a preceding render extension(s).
    When this type is used the \l prepareRender() and \l render() functions are both called during
    the frame's prepare phase.

    \value Main The rendering code is recorded within the main render pass. In this mode the
    \l prepareRender() is called in the frame's prepare phase while \l render() is called the frame's render phase.

*/

/*!
    \enum QSSGRenderExtension::Mode

    Specifies the order the extension will be called.

    \value Underlay The rendering code is recorded and executed before the main (color) pass.
    \value Overlay The rendering code is recorded and executed after the main (color) pass.
*/


/*!
    \fn bool QSSGRenderExtension::prepareData(QSSGFrameData &data)

    Called after scene data is collected, but before any render data or rendering in the current
    frame has been done.

    \return Dirty state. Return \c true if the there are dirty data
    that needs to be rendered.

    \note Much of the data created/collected from the engine during the prepare and render phases
    is per-frame and should be released or assumed released at the start of the next frame

    \sa QSSGFrameData
*/
bool QSSGRenderExtension::prepareData(QSSGFrameData &data)
{
    Q_UNUSED(data);
    return false;
}

/*!
    \fn void QSSGRenderExtension::prepareRender(const QSSGRenderer &renderer, QSSGFrameData &data)

    Prepare data for rendering. Build and collect data needed for rendering. Any render extension
    scheduled before this one has been processed. In addition; any render extension of
    type \l Type::Standalon will, if successful, have been completed in full.

    \note Much of the data created/collected from the engine during the prepare and render phases
    is per-frame and should be released or assumed released at the start of the next frame

    \sa QSSGFrameData, QSSGRenderer
*/
void QSSGRenderExtension::prepareRender(const QSSGRenderer &renderer, QSSGFrameData &data)
{
    Q_UNUSED(renderer);
    Q_UNUSED(data);
}

/*!
    \fn QSSGRenderExtension::render(const QSSGRenderer &renderer)
    \a renderer handle to the QtQuick3D's render object.

    Record the render pass. Depending on the extensions \l {Type}{type} this function will be called
    during the frame's prepare or render phase.

    \sa QSSGRenderExtension::Type, QSSGRenderer
*/
void QSSGRenderExtension::render(const QSSGRenderer &renderer)
{
    Q_UNUSED(renderer);
}

/*!
    \fn QSSGRenderExtension::release()

    Called each time a new frame starts. Any data from the previous frame should be cleared at
    this point.
*/
void QSSGRenderExtension::release()
{

}

/*!
    \fn QSSGRenderExtension::type() const
    \return The render extension type.

    \sa QSSGRenderExtension::Type
*/
QSSGRenderExtension::Type QSSGRenderExtension::type() const
{
    return Type::Main;
}

/*!
    \fn QSSGRenderExtension::mode() const
    \return The mode that should used for this render extension.

    \sa QSSGRenderExtension::RenderMode
*/
QSSGRenderExtension::RenderMode QSSGRenderExtension::mode() const
{
    return RenderMode::Overlay;
}

QT_END_NAMESPACE
