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

#include "qssgrenderrenderlist_p.h"
#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>

QT_BEGIN_NAMESPACE

QSSGRef<QSSGRenderList> QSSGRenderList::createRenderList()
{
    return QSSGRef<QSSGRenderList>(new QSSGRenderList());
}

QSSGRenderTask::~QSSGRenderTask() = default;

void QSSGRenderList::beginFrame()
{
    m_nextTaskId = 1;
    m_tasks.clear();
}

quint32 QSSGRenderList::addRenderTask(const QSSGRef<QSSGRenderTask> &inTask)
{
    quint32 taskId = m_nextTaskId;
    ++m_nextTaskId;
    m_tasks.push_back(QPair<quint32, QSSGRef<QSSGRenderTask>>(taskId, inTask));
    return taskId;
}

void QSSGRenderList::discardRenderTask(quint32 inTaskId)
{
    auto iter = m_tasks.begin();
    const auto end = m_tasks.end();
    while (iter != end && iter->first != inTaskId)
        ++iter;

    if (iter != end)
        m_tasks.erase(iter);
}

void QSSGRenderList::runRenderTasks()
{
    auto iter = m_tasks.crbegin();
    const auto end = m_tasks.crend();
    while (iter != end) {
        iter->second->run();
        ++iter;
    }
    beginFrame();
}

QT_END_NAMESPACE
