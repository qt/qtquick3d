// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgrenderextensions.h"
#include "private/qssgassert_p.h"
#include "private/qssglayerrenderdata_p.h"
#include "qssgrendercontextcore.h"

QT_BEGIN_NAMESPACE

/*!
    \class QSSGFrameData
    \inmodule QtQuick3D
    \since 6.7

    \brief Storage class containing data collected for a frame.
*/

/*!
    \return The renderable texture result from \a id. \nullptr if no matching \a id was found.

    \note Even if the function returns a non-null result, the returned QSSGRhiRenderableTexture
    might not be ready unless the pass rendering to the texture has been executed.

    \note The returned value is only valid within the current frame. On each new frame
    the renderable will be reset and should therefore be queried again.
*/
QSSGFrameData::Result QSSGFrameData::getRenderResult(RenderResult id) const
{
    using RenderResultT = std::underlying_type_t<RenderResult>;
    const QSSGRhiRenderableTexture *res = nullptr;
    auto *data = QSSGLayerRenderData::getCurrent(*m_ctx->renderer());
    if (QSSG_GUARD(data && (std::size(data->renderResults) > RenderResultT(id))))
        res = data->getRenderResult(id);

    return res ? Result{ res->texture, res->depthStencil } : Result{};
}

/*!
    \return Base pipeline state for this frame
 */
QSSGRhiGraphicsPipelineState QSSGFrameData::getPipelineState() const
{
    auto *data = QSSGLayerRenderData::getCurrent(*m_ctx->renderer());
    QSSG_ASSERT(data, return {});
    return data->getPipelineState();
}

/*!
    \return The active camera for the scene, or null if non could be found.
*/
QSSGCameraId QSSGFrameData::activeCamera() const
{
    QSSGCameraId ret { QSSGCameraId::Invalid };
    auto *data = QSSGLayerRenderData::getCurrent(*m_ctx->renderer());
    QSSG_ASSERT(data, return ret);
    if (auto *ac = data->activeCamera())
        ret = QSSGRenderGraphObjectUtils::getCameraId(*ac);

    return ret;
}

QSSGRenderContextInterface *QSSGFrameData::contextInterface() const
{
    return m_ctx;
}

void QSSGFrameData::clear()
{

}

QSSGLayerRenderData *QSSGFrameData::getCurrent() const
{
    return QSSGLayerRenderData::getCurrent(*m_ctx->renderer());
}

QSSGFrameData::QSSGFrameData(QSSGRenderContextInterface *ctx)
    : m_ctx(ctx)
{

}

/*!
    \class QSSGRenderExtension
    \inmodule QtQuick3D
    \since 6.7

    \brief Base class for extension backend node implementations.

    \sa QQuick3DRenderExtension
*/

QSSGRenderExtension::QSSGRenderExtension()
    : QSSGRenderGraphObject(QSSGRenderGraphObject::Type::RenderExtension, FlagT(Flags::HasGraphicsResources))
{

}

QSSGRenderExtension::~QSSGRenderExtension()
{

}

/*!
    \enum QSSGRenderExtension::RenderMode

    Specifies the render extension mode.

    \value Standalone The rendering code is recorded in full during the render prepare phase.
    This will usually imply that there are some output crated for a preceding render extension(s).
    When this mode is used the \l prepareRender() and \l render() functions are both called during
    the frame's prepare phase.

    \value Main The rendering code is recorded within the main render pass. In this mode the
    \l prepareRender() is called in the frame's prepare phase while \l render() is called the frame's render phase.

*/

/*!
    \enum QSSGRenderExtension::RenderStage

    Specifies the order the extension will be called.

    \value PreColor The rendering code is recorded and executed before the main (color) pass.
    \value PostColor The rendering code is recorded and executed after the main (color) pass.
*/


/*!
    Called after scene \a data is collected, but before any render data or rendering in the current
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
    Prepare data for rendering. Build and collect \a data needed for rendering. Any render extension
    scheduled before this one has been processed. In addition; any render extension of
    mode \l RenderMode::Standalone will, if successful, have been completed in full.

    \note Much of the data created/collected from the engine during the prepare and render phases
    is per-frame and should be released or assumed released at the start of the next frame

    \sa QSSGFrameData
*/
void QSSGRenderExtension::prepareRender(QSSGFrameData &data)
{
    Q_UNUSED(data);
}

/*!
    Record the render pass. Depending on the extensions \l {RenderMode}{mode} this function will be called
    during the frame's prepare or render phase.

    Use \a data to gain access to the render context from which the active QRhi object can be queried.

    \sa QSSGRenderExtension::RenderMode
*/
void QSSGRenderExtension::render(QSSGFrameData &data)
{
    Q_UNUSED(data);
}

/*!
    Called each time a new frame starts. Any data from the previous frame should be cleared at
    this point.
*/
void QSSGRenderExtension::resetForFrame()
{

}

/*!
    \return The render mode used for this extension.
 */
QSSGRenderExtension::RenderMode QSSGRenderExtension::mode() const
{
    return RenderMode::Main;
}

/*!
    \return The stage in which this render extension will be used.
*/
QSSGRenderExtension::RenderStage QSSGRenderExtension::stage() const
{
    return RenderStage::PostColor;
}

QT_END_NAMESPACE
