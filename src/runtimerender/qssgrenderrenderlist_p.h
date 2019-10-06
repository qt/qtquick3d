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

#ifndef QSSG_RENDER_RENDER_LIST_H
#define QSSG_RENDER_RENDER_LIST_H

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

#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>

QT_BEGIN_NAMESPACE

class QSSGRenderTask
{
public:
    QAtomicInt ref;
    virtual ~QSSGRenderTask();
    virtual void run() = 0;
};

/**
 * The render list exists so that dependencies of the main render target can render themselves
 * completely before the main render target renders.  From Maxwell GPU's on, we have a tiled
 * architecture.  This tiling mechanism is sensitive to switching the render target, so we would
 * really like to render completely to a single render target and then switch.  With our layered
 * render architecture, this is not very feasible unless dependencies render themselves before
 * the main set of layers render themselves.  Furthermore it benefits the overall software
 * architecture to have a distinct split between the prepare for render step and the render step
 * and using this render list allows us to avoid some level of repeated tree traversal at the cost
 * of some minimal per frame allocation.  The rules for using the render list are that you need to
 * add yourself before your dependencies do; the list is iterated in reverse during RunRenderTasks.
 * So a layer adds itself (if it is going to render offscreen) before it runs through its renderable
 * list to prepare each object because it is during the renderable prepare traversale that
 * subpresentations will get added by the offscreen render manager.
 */
class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderList
{
public:
    QAtomicInt ref;

private:
    typedef QPair<quint32, QSSGRef<QSSGRenderTask>> TTaskIdTaskPair;
    typedef QVector<TTaskIdTaskPair> TTaskList;

    TTaskList m_tasks;
    quint32 m_nextTaskId = 1;
    bool m_scissorEnabled = false;
    QRect m_scissorRect;
    QRect m_viewport;

public:
    QSSGRenderList() = default;
    ~QSSGRenderList() {}
    // Called by the render context, do not call this.
    void beginFrame();

    // Next tell all sub render target rendering systems to add themselves to the render list.
    // At this point
    // we agree to *not* have rendered anything, no clears or anything so if you are caching
    // render state and you detect nothing has changed it may not be necessary to swap egl
    // buffers.
    quint32 addRenderTask(const QSSGRef<QSSGRenderTask> &inTask);
    void discardRenderTask(quint32 inTaskId);
    // This runs through the added tasks in reverse order.  This is used to render dependencies
    // before rendering to the main render target.
    void runRenderTasks();

    // We used to use GL state to pass information down the callstack.
    // I have replaced those calls with this state here because that information
    // controls how layers size themselves (which is quite a complicated process).
    //
    // 2nd parameter, bool unused, is for template compatibility of QSSGRenderListScopedProperty
    void setScissorTestEnabled(bool enabled, bool unused = false) { Q_UNUSED(unused) m_scissorEnabled = enabled; }
    void setScissorRect(QRect rect, bool unused = false) { Q_UNUSED(unused) m_scissorRect = rect; }
    void setViewport(QRect rect, bool unused = false) { Q_UNUSED(unused) m_viewport = rect; }
    bool isScissorTestEnabled() const { return m_scissorEnabled; }
    QRect getScissor() const { return m_scissorRect; }
    QRect getViewport() const { return m_viewport; }

    static QSSGRef<QSSGRenderList> createRenderList();
};

// Now for scoped property access.
template<typename TDataType>
struct QSSGRenderListScopedProperty : public QSSGRenderGenericScopedProperty<QSSGRenderList, TDataType>
{
    typedef QSSGRenderGenericScopedProperty<QSSGRenderList, TDataType> TBaseType;
    typedef typename TBaseType::TGetter TGetter;
    typedef typename TBaseType::TSetter TSetter;
    QSSGRenderListScopedProperty(QSSGRenderList &ctx, TGetter getter, TSetter setter)
        : TBaseType(ctx, getter, setter)
    {
    }
    QSSGRenderListScopedProperty(QSSGRenderList &ctx, TGetter getter, TSetter setter, const TDataType &inNewValue)
        : TBaseType(ctx, getter, setter, inNewValue)
    {
    }
};
QT_END_NAMESPACE

#endif
